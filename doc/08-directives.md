# Directive Reference

This is the full list of `#…` directives, grouped by what they do. It's the place to
check a directive's spelling and one-line meaning; the deeper behavior lives in the
chapters each row links to.

A few conventions. Most directives take arguments in brackets, `#name[…]`, or parens,
`#name(…)`. Under the hood the lexer captures both into one argument field, so the
bracket-versus-paren choice is mostly a documentation convention, and this reference
uses the conventional punctuation for each. (`#include(...)` is the one that's
genuinely special-cased.) A directive may also take a trailing name it declares or
targets, as in `#stack[int, 32] x = 5;`. A `&` or `*` prefix on that trailing name
matters for `#req` and `#scanp` (reference, copy, or a deprecated spelling); see
[Scopes & `#req`](03-scopes-and-req.md) and [Callables](04-callables.md).

The "Gate" column is which `#include('…')` module has to be loaded first. The
"Backend" column notes interpreter-only or native-only where the runtime makes a
distinction; a dash means no special restriction.

## Memory

| Directive | Syntax | Meaning | Gate | Backend |
|-----------|--------|---------|------|---------|
| `#stack` | `#stack[type, bits] name = init;` | Fixed stack buffer. Default 256 bits. Overflow errors. A hex name goes in the hex-var store. | — | — |
| `#heap` | `#heap[type, bits] name = init;` | Heap buffer. Default 1024 bits. Freed at program end. | — | — |
| `#auto` | `#auto[type] name = init;` | Auto-sized buffer. At least 32 bytes, grows. Manual `#delete`. | — | — |
| `#delete` | `#delete[name]` / `#delete[0xADDR]` / `#delete[Sec->m]` | Free a slot, cascade over its members, and follow the reference chain up (up to 16 deep). | — | — |
| `#delmember` | `#delmember[c]` / `#delmember[c[i]]` | Clear contents, keep size and capacity. Idempotent and null-safe. | — | — |
| `#free` | `#free[sector]` / `#free[obj]` / `#free[alias]` / `#free[0xADDR]` | Run `free { }`, then drop a sector, object, or loaded module. No-op for a built-in module. | — | — |
| `#reallocate` | `#reallocate[var, type, stack\|heap, size?] newvar;` | A real region move, type-strict, default grow of 32 bits. | — | — |
| `#resize` | `#resize[var, ±bits];` | Grow or shrink an existing alloc, following the reference chain. | — | — |
| `#reserve` | `#reserve[label];` / `#reserve[mod.label];` | Construct a deferred `sector … ?label`. | — | — |
| `#set` | `#set[0xADDR, bits] = value;` | Write a coerced value at a known address, bit-checked. | — | interpreter |
| `#peekstack` / `#peekheap` | `#peekstack[src, type, index, amount] name;` | Slice a collection into a new stack or heap var. | — | — |
| `#cast` | `#cast[source, target] result;` | Coerce `source` into an existing `#heap` target. | — | — |
| `#adress` | `#adress[var] name;` (get) · `#adress[var] = 0xABC;` (alias) · `#adress[var]` (expression) | Address introspection. Missing var gives `0x0`. One `d`. | — | alias form interpreter-only |

`#address` and `#memory` aren't directives (`#memory` is explicitly rejected). The
only spelling is `#adress`.

## Callables

| Directive | Syntax | Meaning | Gate |
|-----------|--------|---------|------|
| `#define` | `#define(name) { … }` | Bind a callback to a slot. | — |
| `#cdefine` | `#cdefine(func, p1, p2) { … }` | A callback with named parameters. | def |
| `#redefine` | `#redefine(fn) { … }` · `#redefine(fn) +<pos> { … }` | Overwrite or inject into a function body. Interpreter-only, since native lowers statically. | — |
| `#fvar` | `#fvar(type) name;` | Declare a typed function-variable, no init. | def |
| `#param` | `#param(type) name;` | Declare a typed parameter, filled by `#scanp`. | def |
| `#scanp` | `#scanp(src, type, pos) name;` · `… &name;` · `… name = default;` | Read a positional call argument. Reference or copy is decided by the call site; missing gives null. | — |
| `#scanp_opt` | `#scanp_opt(src, type, pos) name;` | Like `#scanp`, but a missing arg silently gives null. | described as def, but the interpreter doesn't enforce it |
| `#qvar` | `#qvar(type, expr) name;` | Materialize `expr` into a typed local. | def |
| `#daemon` | `#daemon[funcVar];` | Run a `#define` func repeatedly on a background thread. | asyncthreads |
| `#killdaemon` | `#killdaemon[funcVar];` | Cooperatively stop a daemon. | asyncthreads |
| `#thread` | `#thread[type, bits] name = init;` | Declare a thread-callable var. | asyncthreads |
| `#await` | `#await[handle] result;` | Join a daemon and capture its result. | asyncthreads |
| `#raii` | `#raii 0xADDR method(args);` · `#raii name method(args);` | Call a method on a hex or named scope (no brackets on the directive). | — |
| `#interrupt` | `#interrupt(name) { … }` | A hardware ISR. Native only; the interpreter registers it but never auto-invokes. | — |

`call` is an object keyword (`call label<args> capture;`, see [Objects](05-objects.md)),
not a directive. There is no `#call`.

## Scope and modules

| Directive | Syntax | Meaning | Backend |
|-----------|--------|---------|---------|
| `#include` | `#include("mod")` / `#include("mod") alias;` | Load a built-in module. Unknown errors. | — |
| `#load` | `#load["path.cssc"] alias;` | Load an external `.cssc` file as a child-runtime module (globals become public). | interpreter |
| `#depend` | `#depend["path.obj"] alias;` | Load an isolated `.obj` package; exposes `alias::sector::member`. | — |
| `#unload` | `#unload[alias];` | Unload a loaded module, running child sectors' `free { }`. | — |
| `#req` | `#req[X] Y;` (reference) · `#req[X] &Y;` (copy) · `#req[Sec->m] Y;` | Import an outer or sector var into a wall. Reference by default; `&` is a deep-copy snapshot. | — |
| `#REQUIRE` | `#REQUIRE["path"] var;` | Load a resource by extension (`.dll`, `.ini`, `.csl`, `.cssl`). Uppercase, and not `#req`. | — |
| `#MODULE` | `#MODULE;` (top of file) | Marks this file as a module: its globals are freed by the importer's `#unload`, so the analyzer never adds a global `#delete` here. No-op at runtime. See [Modules](07-modules.md). | analyzer |
| `#tlisten` | `#tlisten[var] { body }` | Run `body` if the watched var is non-null. | interpreter |

There's no `#require`. The real ones are `#req`, `#REQUIRE`, `#include`, `#load`,
`#depend`, and `#unload`.

## Introspection

| Directive | Syntax | Meaning |
|-----------|--------|---------|
| `#size` | `#size[var] out;` | Used size in bits. |
| `#capacity` | `#capacity[var] out;` | Allocated capacity in bits, following the reference chain. |
| `#exists` | `#exists[0xADDR] out;` | 1 or 0: is this address a known allocation? |
| `#reflect` | `#reflect[address] out;` | Resolve an address back to its value. |
| `#adress` | (see Memory) | Address of a slot; `0x0` if unbound. |
| `#iterator` | `#iterator[type, source] name;` | Create an STL-style iterator over a container. |

## IO and diagnostics

| Directive | Syntax | Meaning | Gate |
|-----------|--------|---------|------|
| `#stdout` | `#stdout(text)` | Write to the stdout module and output buffer. | stdout |
| `#debug` | `#debug(msg);` | Always append to `dev::stdout`; print to stderr only with `--debug`. | devdebug |
| `#trace` | `#trace(funcName);` | Log each call of a function, only under `--debug`. | devdebug |
| `#catch` | `#catch (callable) ?caller { body }` | Catch a runtime error and bind the message to `?caller`. | stdgrace |
| `#panic` | `#panic("message");` | Raise a runtime error deliberately. | — |
| `#sysarg` | `#sysarg[type, index] var;` | A CLI argument into a stack var (needs `#delete`). | sys |
| `#sysout` | `#sysout(expr);` | Return a value to the `cssc.run()` host caller. | — |
| `#clock` | `#clock[ms];` | Sleep `ms` milliseconds. | — |
| `#OUTPUT` | `#OUTPUT["path"]` | Set the output path. | — |
| `#EXIT` | `#EXIT[code]` | Set the exit code and stop. | — |
| `#DEFINE` | `#DEFINE name;` · `#DEFINE __main__ '__main__';` · `#DEFINE name <expr>;` | A compiler/transpiler construct (entry point, passthrough, compile-time constant). No-op at runtime in the interpreter. Different from lowercase `#define`. | transpiler/native |

## Typed-declaration directives

| Directive | Syntax | Gate |
|-----------|--------|------|
| `#VARIABLE` | `#VARIABLE[type] name = value;` | — |
| `#string` | `#string[bits] name = init;` | string |
| `#int` | `#int[bits] name = init;` | int |
| `#array` | `#array[type, size] name = init;` | array |
| `#vector` | `#vector[type, bits] name = init;` | vector |
| `#map` | `#map[keytype, valtype, bits] name = init;` | map |
| `#matrix` | `#matrix[width, height] name = fill;` | matrix |

For containers, the second `#stack`/`#heap` argument is an element capacity, not a
scalar bit limit (see [Types & Values](01-types-and-values.md)).

One known issue: the `#array[type, size]` directive form passes two arguments to a
one-argument constructor and raises a `TypeError`. Use an `array<T>` typed
declaration or a `{…}` literal instead.

## Hardware and peripherals

These are documented with `#include`, but the interpreter doesn't gate them (the
helpers construct directly). The toolchain docs cover target mapping.

| Directive | Syntax | Meaning |
|-----------|--------|---------|
| `#pin` | `#pin[N] var;` | A GPIO line |
| `#i2c` | `#i2c[bus, sda, scl] var;` | An I2C master |
| `#spi` | `#spi[bus, sck, miso, mosi] var;` | An SPI master |
| `#uart` | `#uart[bus, tx, rx] var;` | A UART/serial port |
| `#adc` | `#adc[pin] var;` | An ADC input |
| `#pwm` | `#pwm[pin, freq, bits] var;` | A PWM output |
| `#timer` | `#timer[slot, hz] var;` | A periodic timer |
| `#tft` | `#tft[ctrl, w, h] var;` | A TFT/OLED panel |
| `#oled` | `#oled[w, h] var;` | Shorthand for `#tft[ssd1306, w, h]` |
| `#video` | `#video[w, h, fps] var;` | A video context (gate: video) |
| `#framebuffer` | `#framebuffer[w, h] var;` | A pixel buffer (gate: video) |
| `#console` | `#console[w, h] var;` | A native console window |
| `#get` / `#post` / `#send` | `#get[url] var;` · `#post[url] var = body;` · `#send[url] body;` | HTTP (from `network.http`) |

## Directives that look real but aren't

| Looks plausible | Reality |
|-----------------|---------|
| `#require` | Not a directive. Use `#req` (var import), `#include`/`#load`/`#depend` (modules), or `#REQUIRE` (resources). |
| `#address` | Not a directive. The spelling is `#adress`, one `d`. |
| `#memory` | Not a directive; it's explicitly rejected. |
| `#call` | Not a directive. `call` is an object keyword. |

## See also

- [Memory & Ownership](02-memory-and-ownership.md) for the memory directives in depth.
- [Callables](04-callables.md) for `#define`, `#scanp`, and `mirror`.
- [Scopes & `#req`](03-scopes-and-req.md) for `#req` and `#DEFINE`.
- [Modules](07-modules.md) for `#include`, `#load`, `#depend`, `#unload`, and
  `#REQUIRE`.
