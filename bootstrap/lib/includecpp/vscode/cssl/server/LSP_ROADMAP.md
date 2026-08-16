# CSSC LSP — Roadmap to Next-Next-Level

The current LSP works but loses overview under several edge cases. CSSC's design philosophy (manual lifetime management, no GC, brutal-by-default deletes) puts heavy weight on the LSP — it's the only place where the user gets a friendly heads-up before the compiler smacks them.

This plan takes the LSP from "good with rough edges" to "the chip's compiler is angry but my editor caught it 30 seconds ago." Phases are ordered so each one stands alone and unblocks the next.

## Audit baseline — the 12 known failure modes

Captured in detail in the audit (file:line refs inline). Short list:

| # | Class | Severity | Repro |
|--:|-------|:--------:|-------|
| 1 | Parser crash on incomplete input (`#stack[int, `) | HIGH | unhandled `IndexError`/`AttributeError` past the `CSSLSyntaxError` catch |
| 2 | Stale-diagnostics race under rapid edits | MED | three `didChange` events within 200 ms |
| 3 | `#include('module')` hover/goto silently empty | MED | module not in LSP's Python env |
| 4 | Completion scope-unaware (offers `free`-block locals in sector body) | MED | brace-counting only, no AST scope walk |
| 5 | No cross-file go-to-def | HIGH | `main.cssc` calls helper.cssc's `#define`d fn |
| 6 | AST cache never evicted → memory grows linearly | MED | edit a 10 MB file 100 times |
| 7 | Diagnostics published for already-closed document | MED | fast open/close cycle |
| 8 | `#delete[` without name → no diagnostic | LOW | regex requires the name |
| 9 | New AST node types silently skipped by analyzer | MED | adding a node type stops emitting lints for it |
| 10 | Single syntax error blanks the whole file | HIGH | line 5 broken → lines 7-100 lose all symbols |
| 11 | Cancelled debounce tasks accumulate in event loop | LOW | progressive sluggishness on long typing sessions |
| 12 | No recursion-depth limit in semantic analyzer | HIGH | deeply-nested expr → `RecursionError` crash |

## Phase 1 — Stability (Bugs 1, 7, 10, 11, 12)

**Goal:** the LSP never dies, never goes silent, never leaks tasks. Pure defense, zero new features.

- Wrap `CsscParser.parse()` in a broad `except Exception` in `document_manager.py:_parse_with_timeout` and convert any non-`CsscError` exception into a synthetic `CsscError` with `line=1, code=PARSER_INTERNAL`. Surface in `analysis.syntax_errors` so the user sees "parser crashed at … — please file a repro" instead of going dark.
- Error-recovery parsing: after a `CsscError` on line N, scan forward to the next top-level statement (line starts with `#`, `object`, `sector`, `method`, `while`, `for`, identifier-followed-by-`(`/`=`) and resume parsing from there. Collect ALL syntax errors per file; produce a partial AST. Bug 10's fix.
- Recursion guard: `sys.setrecursionlimit(10000)` in the parser context + an explicit depth counter on the `_lower_expr` walker that raises `CsscError` at depth > 256 ("expression too deeply nested — split into intermediate slots"). Bug 12.
- `did_close` must `await` the in-flight parse before publishing nothing for the URI; track open task per URI in `_active_parse_tasks` and `await` it under `asyncio.shield()`. Bug 7.
- Replace per-edit `asyncio.ensure_future` with a single per-URI dedicated worker task that consumes a `queue.Queue` of pending edits. Cancellation is O(1) and the queue is bounded. Bug 11.

## Phase 2 — Correctness (Bugs 2, 8, 9)

**Goal:** every diagnostic the LSP shows reflects the current document state. No silent skips.

- Version-stamp guard: in `_publish_diagnostics`, recheck `documents[uri].version == analyzed_version` *under the lock*; if not, drop the diagnostics rather than publish stale ones. Bug 2.
- Extend the `#delete` regex to also match `#delete[\s*\]|\s*$` and emit `INCOMPLETE_DELETE` warning. Apply the same fix to `#adress`, `#free`, `#scanp` regexes. Bug 8.
- Replace `_visit_node`'s silent generic-fallback with an explicit `_visit_node_unknown(node)` that logs at INFO level once per node-type-per-session AND surfaces an `info`-level diagnostic on the file: "LSP doesn't yet lint this node type — please file an issue". Bug 9.
- Add the `#delete[<missing>]` AND `#adress[<missing>] → 0x0` lint NOW. The static check is trivial (walk decls, walk deletes, diff); shipping it stops the BRUTAL compile error from ever surprising the user.

## Phase 3 — Scope-aware lifetime tracker (THE big-ticket item)

**Goal:** make "no GC, you manage lifetimes" pleasant. The LSP becomes the safety net so the runtime stays mean.

This is its own subsystem. New file: `server/analysis/lifetime_tracker.py`.

The tracker maintains, per function/label body, a **lexical scope tree**: each `{ … }` block, each `if`/`else`/`while`/`for` body is a node with:
- `declared`: set of names introduced via `#stack`/`#heap`/`#auto`/`#cdefine` param/`#scanp` capture in this block
- `deleted`: set of names that were `#delete`d in this block
- `referenced`: list of (name, line) references inside this block
- `escape_to_outer`: set of names whose ownership leaves the block (e.g., assigned to an outer-scope slot)

From that tree we can emit these diagnostics — all SHOULD be warnings at first (not errors), so the user can opt to ignore noisy ones:

| Diagnostic | Trigger |
|------------|---------|
| `LIFETIME_LEAK` | Block declares `x` with `#stack`/`#heap`, never `#delete`s it AND never escapes it; warn at block close |
| `USE_AFTER_DELETE` | Reference to `x` after `#delete[x]` in same block (or any parent block) |
| `DOUBLE_DELETE` | Two `#delete[x]` in the same block without a re-declare |
| `CTOR_LOCAL_IN_FREE` | Object's `free {}` block references a `#stack` name declared inside the ctor body (the exact bug that hit `main.cssc`'s `#delete[final]`) |
| `IF_BRANCH_LIFETIME_ASYMMETRY` | `if` deletes `x` but `else` doesn't (or vice versa) |
| `LOOP_BODY_LEAK` | `while`/`for` body declares `x` with `#stack`/`#heap` and never deletes it → arena fills per iteration |

Also drives:
- Scope-aware completion (Bug 4): completion offers only names live at the cursor.
- Quick-fix code actions: "Insert `#delete[x];` before end of block", "Add `if (#adress[x] != 0x0) { #delete[x]; }` guard".

## Phase 4 — Cross-file project model (Bugs 3, 5, 6)

**Goal:** the LSP knows about every `.cssc` file in the project, not just the open ones.

- New `WorkspaceIndex` keyed by file path. On first `initialize`, walk every `.cssc` under the workspace root, parse each lazily (only when first queried). Re-parse on file change.
- `#include('./other.cssc')` resolves through the index → cross-file go-to-def, find-refs, hover (Bug 5).
- `#include('module-name')` resolves via:
  1. Workspace index (project-local override)
  2. Per-package symbol stubs shipped with the LSP (the `diagnostic_provider.py:22-93` table — moved to `stubs/<module>.cssc-stub` so they're reviewable). Bug 3 fallback.
  3. Live Python `importlib` against the LSP's env (best-effort; failure is silent today, becomes an info-level "module symbols unavailable" diagnostic).
- LRU eviction on `WorkspaceIndex` AST cache: cap at 200 files / 100 MB of AST. Bug 6.

## Phase 5 — Performance & polish

- **Incremental reparse:** when the user types on line N, re-lex from line N's start, re-parse only the smallest enclosing top-level construct. Plug into the existing AST cache via subtree replacement. Huge win for 5 KB+ files.
- **Inlay hints:** `#stack[int, 32] x;` → render `   // 4 B` next to it. `#stack[string, 256] s;` → `// 32 B header + 256 B data`. Helps the user feel the arena budget.
- **Workspace symbol search:** Ctrl+T → all `#define`s, all objects, all sectors across the project.
- **Rename refactor:** rename `#stack[int, 32] foo` to `bar` and every `#delete[foo]`, every reference, every `#adress[foo]` gets updated atomically.
- **Code actions:**
  - "Add #delete[x] for unfreed slot"
  - "Convert #stack to #heap" (when size > stack budget)
  - "Wrap with #adress guard" (when `#delete` is in a possibly-not-reached path)
  - "Move local to object member" (the ctor-body-in-free-block fix)

## Phase 6 — Embedded-specific

- **Per-target lint filter:** `cssc.target = "esp8266"` in `.vscode/settings.json` → suppress diagnostics for features the target supports but native doesn't (e.g., `#daemon` warnings only fire when target is host).
- **Arena-budget estimator:** sum `#stack`/`#heap` sizes per function. If sum exceeds the target's arena (`64 KB` for esp8266) — error. If exceeds 80% — warn. Visible as a CodeLens at the function header.
- **Trace-event sidebar:** when the user is running `cssc diagnostics --console --trace`, mirror the call-chain in the editor's sidebar with click-to-jump. Folds the gap between "I flashed it" and "where did it crash".
- **`#adress` constant-fold hook:** for `if (#adress[x] != 0x0) { … }`, the LSP can statically prove `x` is in scope and offer a code action "this guard is redundant; just `#delete[x];`". Drives users toward minimal source.

## Sequencing

Phase 1 is two days of defensive coding — ship first; it stops the LSP from going dark and unblocks every later phase. Phase 2 is one day. Phase 3 is the big one (~one to two weeks) — but it's the ENTIRE selling point of "CSSC is minimal because the LSP carries the safety net". Without Phase 3, the brutal-by-default delete semantics are user-hostile; with it, they're a feature.

Phase 4 unblocks anyone with a multi-file project (i.e. any real project). Phases 5 and 6 are polish that compounds — they each take ~one week — but the LSP is usable and pleasant after Phase 3.

## Architecture changes

The current server (`server.py`, 757 lines) is monolithic. Phase 1 introduces a small split:

```
server/
├── server.py                       (entry, pygls dispatch)
├── document_manager.py             (existing — AST cache, parse-on-debounce)
├── workspace_index.py              [Phase 4]
├── analysis/
│   ├── lifetime_tracker.py         [Phase 3]
│   ├── arena_budget.py             [Phase 6]
│   └── semantic_analyzer.py        (existing)
├── providers/
│   ├── diagnostic_provider.py      (existing — split into static + lifetime + budget)
│   ├── completion_provider.py      (existing — wire scope from lifetime_tracker)
│   ├── hover_provider.py           (existing — query workspace_index)
│   ├── definition_provider.py      (existing — query workspace_index)
│   ├── code_action_provider.py     [Phase 5]
│   ├── inlay_hint_provider.py      [Phase 5]
│   └── rename_provider.py          [Phase 5]
└── stubs/
    └── <module>.cssc-stub          [Phase 4]
```

Each provider gets a `lifetime_tracker` injected in `__init__` so Phase 3's data flows to everywhere it's needed.

## Non-goals

- Full type inference (CSSC's typing is declarative; declared types are authoritative).
- LSP-driven refactoring engine beyond rename / extract-slot (over-engineering for the user base).
- Built-in profiler / debugger (`cssc diagnostics --console --trace` already covers that surface; the LSP just renders its output).
