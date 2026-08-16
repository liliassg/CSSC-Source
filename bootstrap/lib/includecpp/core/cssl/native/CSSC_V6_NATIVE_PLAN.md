# CSSC v6 — Native Backend (LLVM IR + Transembly Runtime)

**Status**: Phase 1 in progress. Phases 2–5 queued.

## Context

CSSC v5 produced firmware via `cssc compile → cssc_compiler.py → .c → gcc → .exe/.elf`. The C-codegen path is now retired. CSSC v6 emits LLVM IR directly from a domain-specific intermediate representation (CIR), and pairs it with Transembly — a CSSC-tailored assembler that emits raw x86_64 / xtensa assembly for the runtime support library. No more `cssc_runtime.c`, no more gcc dependency for plain host builds.

The design goal: maximum control over what the chip executes, minimum overhead, full integration. Take advantage of the fact that we own the language — we tune the runtime calling-conventions and the value layout for CSSC specifically.

## Architecture

```
.cssc source
   │  CsscLexer + CsscParser (unchanged from v5)
   ▼
CSSC AST  (Python dicts, the v5 shape)
   │  cir_lower.py        ← NEW
   ▼
CIR  (CSSC Intermediate Representation; SSA-ish, refcount-aware)
   │  cir_to_llvm.py      ← NEW
   ▼
LLVM IR (.ll text)
   │  llc (LLVM)          ← external dependency, single binary
   ▼
ELF/PE object
   │  lld-link / ld       ← LLVM linker
   ▼
Native executable (.exe / .elf)


                ┌────────────────────────────────┐
                │  Transembly runtime emitter    │  ← parallel, called by `cssc build`
                │  transembly.py                 │     once per target. Output is a
                │   ├─ x86_64 backend            │     .o (PE/ELF object) containing
                │   └─ xtensa-lx106 backend      │     all cssc_*  symbols. Cached.
                └────────────────────────────────┘
```

## CIR — CSSC Intermediate Representation

A minimal SSA IR sized for CSSC's semantics. Each function is a list of basic blocks; each block is a list of operations; each operation has a tagged type and produces a single value with a unique SSA id.

### Value types (`cir_types.py`)

```python
@dataclass
class CIRType:
    kind: str       # 'i64', 'i32', 'f64', 'i1', 'ptr', 'csscval', 'void'
    bits: int = 0   # for stack-allocated heap-types: how big the slot is
```

`csscval` is the canonical 16-byte tagged-union representation:
```
struct CsscVal {
    uint64_t tag;      // 4 bits type, 28 bits flags, 32 bits size hint
    union { int64_t i; double f; void* ptr; } data;
};
```

### Operations (`cir_ops.py`)

| Op | Result | Operands | Semantics |
|---|---|---|---|
| `const_int` | i64 | imm | Integer immediate |
| `const_float` | f64 | imm | Float immediate |
| `const_bool` | i1 | imm | Bool immediate |
| `const_str_ptr` | ptr | imm-name | Pointer to a global string blob |
| `wrap_int` | csscval | i64 | Build CsscVal from int (tag=INT, data.i=...) |
| `wrap_float` | csscval | f64 | Build CsscVal float |
| `wrap_bool` | csscval | i1 | Build CsscVal bool |
| `string_from_lit` | csscval | ptr, i64-len | Calls `cssc_string_lit(ptr,len)` (interned, no copy) |
| `slot_alloc` | ptr | type-kind, bits | Stack-allocate a CsscVal-sized slot |
| `slot_store` | void | ptr, csscval | Store + refcount-bump if heap |
| `slot_load` | csscval | ptr | Load + retain (ref-by-default for callers) |
| `slot_release` | void | ptr | Release + zero |
| `scope_push` | scope-frame | — | New scope frame |
| `scope_pop` | void | scope-frame | Drop + cascade release |
| `scope_bind` | void | scope-frame, name-imm, ptr | Bind named slot in current scope |
| `scope_lookup` | ptr | scope-frame, name-imm | Resolve name → slot ptr |
| `scope_alias` | void | scope-frame, name-imm, name-imm | Install alias entry (#req ref) |
| `call` | csscval | fn-name-imm, [csscval...] | Direct call to a runtime symbol or another CSSC fn |
| `cssc_outln` | void | csscval | Lowered to `cssc_print_val` runtime call |
| `eq`, `ne`, `lt`, `gt`, `le`, `ge` | i1 | csscval, csscval | Comparison via runtime helpers |
| `add`, `sub`, `mul`, `div`, `mod` | csscval | csscval, csscval | Arithmetic, polymorphic int/float |
| `br_cond` | terminator | i1, target-bb, target-bb | Conditional branch |
| `br` | terminator | target-bb | Unconditional branch |
| `ret` | terminator | csscval? | Return |

Refcount is **inline at CIR level** — every `slot_store` of a heap CsscVal emits a refcount-bump, every `slot_release` emits the matching dec + cascade. The LLVM-IR emitter is dumb: just a textual transcription.

## Files to create

| File | Lines (est.) | Purpose |
|---|---|---|
| [native/cir/types.py](includecpp/core/cssl/native/cir/types.py) | 80 | CIRType + value tag constants |
| [native/cir/ops.py](includecpp/core/cssl/native/cir/ops.py) | 200 | Op dataclasses + Block + Function |
| [native/cir/builder.py](includecpp/core/cssl/native/cir/builder.py) | 250 | Fluent CIR builder (`b.const_int(5)`, `b.scope_push()`, etc.) |
| [native/cir/cir_lower.py](includecpp/core/cssl/native/cir/cir_lower.py) | 900 | Walks the CSSC AST, emits CIR via the builder |
| [native/cir/cir_to_llvm.py](includecpp/core/cssl/native/cir/cir_to_llvm.py) | 600 | CIR → `.ll` text |
| [native/transembly/x86_64.py](includecpp/core/cssl/native/transembly/x86_64.py) | 700 | Emits an .o for cssc_* runtime symbols on x86_64 |
| [native/transembly/xtensa.py](includecpp/core/cssl/native/transembly/xtensa.py) | 800 | Same for xtensa-lx106 (deferred until ESP target) |
| [native/v6_driver.py](includecpp/core/cssl/native/v6_driver.py) | 200 | CLI glue: AST→CIR→LLVM→llc→ld |
| Tests | 200 | Hello-world end-to-end |

Total ≈ 4000 lines new code, replaces ≈ 5500 lines of `cssc_compiler.py` + `cssc_runtime.c`.

## Runtime symbols emitted by Transembly (x86_64)

Minimum set for Phase 1 (Hello + int outln):
- `cssc_runtime_init` — initialise global heap, intern table; `_start` calls this first.
- `cssc_runtime_shutdown` — flush, exit syscall.
- `cssc_alloc(size)` — bump-allocator on a 1MiB linear arena. No free() in Phase 1.
- `cssc_print_int(i64)` — formats and writes to stdout via syscall `write`.
- `cssc_print_str(ptr, len)` — write syscall.
- `cssc_panic(msg, len)` — write + exit(1).

Phase 2 expands with: refcount cascade, vector/map allocators, string interning, file syscalls.

## Phases

### Phase 1 — Hello-World End-to-End (THIS RUN)
**Deliverable**: `cssc native examples/v6_hello.cssc -o hello.exe && ./hello.exe` prints `5` and exits cleanly. No gcc, no `cssc_compiler.py`. The example source:
```cssc
#stack[int, 32] x = 5;
cssc::outln(x);
#delete[x];
```
**Components produced**:
- `cir/types.py`, `cir/ops.py`, `cir/builder.py`
- `cir/cir_lower.py` — handles: `#stack[int,N]` decl, int literal, arrow_assign, `cssc::outln(i)`, `#delete[x]`
- `cir/cir_to_llvm.py` — emits `.ll` for the above ops
- `transembly/x86_64.py` — `_start`, `cssc_runtime_init`, `cssc_alloc`, `cssc_print_int`, `cssc_runtime_shutdown` as inline-NASM-style emitter, packaged into a system-V ELF / PE object via `pyelftools` or hand-rolled writer
- `v6_driver.py` — invokes `llc` (LLVM ≥ 17) and `lld-link` (Windows) / `ld` (Linux)
- `cssc native` subcommand in `cli/commands.py`
- One test `.cssc` + Python smoke script
**Out of scope this run**: anything in Phase 2+.

### Phase 2 — Full Value Surface
- `wrap_float`, `wrap_bool`, `string_from_lit`, comparison + arithmetic, branch + loop CIR ops
- Transembly: refcount header struct, `cssc_retain`, `cssc_release`, `cssc_release_internal`, `cssc_copy`, `cssc_string_lit`, `cssc_string_concat`
- `for`, `while`, `if/else`, expressions all the way through

### Phase 3 — Containers
- `cssc_vector`, `cssc_vector_push`, `cssc_vector_get`, `cssc_vector_pop`, `cssc_vector_size`, `cssc_vector_clear`
- `cssc_map`, `cssc_map_set`, `cssc_map_get`
- Transembly: hash table implementation in assembly. Bump-allocator extended to handle realloc for grow.

### Phase 4 — Objects, Sectors, Labels
- Object/sector creation in CIR (`object_create` op, `label_call` op)
- Free-block cascade in destructors
- The full v5 ownership model on the native side, mirroring `cssc_object_free`, `cssc_sector_free`

### Phase 5 — Xtensa LX106 (ESP8266)
- `transembly/xtensa.py` parallels the x86_64 emitter
- ELF emitter targets Espressif's bootloader format
- Drop-in replacement for `cssc build --esp8266` flow

## CLI Surface

```
cssc native source.cssc -o out                # current target (x86_64 host)
cssc native source.cssc -o out --emit ll      # stop at .ll, don't link
cssc native source.cssc -o out --emit asm     # stop at .s
cssc native source.cssc -o out --emit obj     # stop at .o
cssc native source.cssc -o out --target esp8266   # Phase 5
```

`cssc build` keeps existing semantics (delegates to v5 path) until Phase 4 is complete; then `cssc build` becomes alias for `cssc native`.

## External Dependencies (Phase 1)

- LLVM ≥ 17 (`llc` on PATH)
- `lld-link.exe` on Windows OR `link.exe` from MSVC OR `ld` on Linux
- No Python deps beyond stdlib

We do NOT depend on `gcc`, `cl`, or PlatformIO.

## Verification (Phase 1)

```powershell
cssc native examples/v6_hello.cssc -o hello.exe
./hello.exe   # prints: 5
$LASTEXITCODE  # 0
```

Plus: `cssc native examples/v6_hello.cssc -o hello --emit ll` writes `hello.ll` we can inspect. `--emit asm` writes `hello.s` from llc. `--emit obj` writes `hello.o`.

## Notes on the Transembly philosophy

The runtime support library is the only place where we DON'T go through LLVM IR. There, the user wants raw machine code tailored for CSSC's value layout. So Transembly emits NASM-style assembly (we keep it textual for review), assembles it via a tiny hand-rolled assembler module (parses mnemonics, emits opcode bytes), and packages into a system-V / PE object file we hand to lld.

The reason: a Refcount-Cascade in C compiles to ~30 instructions per release call. In hand-tuned assembly with CSSC-specific knowledge (we know the layout, the alignment, the hot-path tag values), we can do it in 8–12. Multiplied across every release in a hot loop on an 80 MHz ESP8266, this is the difference between "smooth" and "WDT-resetting".

The trade-off: every new runtime function requires hand-tuning for each target. That's why we delay xtensa Transembly to Phase 5.
