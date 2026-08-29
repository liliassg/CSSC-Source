# Modules

A module is a reusable scope you reach with `::`. CSSC has three kinds:

1. Built-in modules, loaded with `#include('name')` from a fixed in-runtime registry
   (`video`, `def`, `sizes`, `os`, and so on).
2. Loaded CSSC files, where `#load["file.cssc"] alias;` runs another `.cssc` file as
   a child runtime and exposes its globals.
3. `.obj` packages, where `#depend["pkg.obj"] alias;` loads a prebuilt package.

## `#include` for built-in modules

```
#include('video') vid;      // load 'video', alias it 'vid'
#include('os') os;
#include('def');            // no alias: the module name is the key
```

`#include('name') alias;` loads a built-in module and binds the alias. Without an
alias, the module name itself is the storage key. An unknown module name is a runtime
error. The name can use single or double quotes.

You may have read that every `#include` needs a matching `#free[alias]`. For a
built-in module, `#free` does nothing at runtime (there's nothing to release), so
forgetting it in the interpreter is harmless. It's only `#load` and `#depend`
modules, and native or `.obj` builds, where teardown actually matters.

## A few built-in modules

| Module | What it provides |
|--------|------------------|
| `def` | Extended callables: `#cdefine`, `#fvar`, `#param`, `#qvar`, `#scanp_opt` (see [Callables](04-callables.md)) |
| `sizes` | Recommended bit-size constants like `sz::normal_int` (see [Types & Values](01-types-and-values.md)) |
| `sys` | CLI arguments: `sys::args`, `sys::argc`, `sys::arg(i)` |
| `stdio`, `cssc.io` | File I/O |
| `cssc.math` | Math: trig, logs |
| `console`, `sys.console` | A native console window and sync |
| `devdebug` | `#debug`, `#trace`, `dev::stdout` |
| `stdgrace` | Graceful errors: `grace::catch`, `#catch` |
| `asyncthreads` | `#daemon`, `#killdaemon`, `#thread`, `#await` |
| `video` (plus `video.sprite`, `.tilemap`, `.font`) | Native windowing and 2-D graphics |
| `keyboard`, `mouse`, `sound` | Input and audio |
| `gipeo` | GPIO, I2C, SPI, UART, ADC, PWM, timers for embedded targets |
| `tft` | TFT and OLED displays |
| `network.http` | An HTTP/HTTPS client |
| `matrix`, `serialcommunication`, `os` | Pixel matrix, IPC, OS access |
| `cssc` | Toolchain access from a script: `csscmod::cli("build x.cssc")` runs a CSSC or shell command, streams the output live, and returns the exit code |
| `colors` | 24-bit terminal output: `col::c_outln(text, 0xRRGGBB)` |

The registry is bigger than this; these are the common ones. Directives that a
module gates are listed per group in [Directives](08-directives.md).

Don't confuse the always-available `cssc::` builtins with the `cssc` module. The
`cssc::` scope (`cssc::outln`, `cssc::typeof`, `cssc::run("file.cssc")`, the bit and
float intrinsics) is the runtime core and needs no `#include`. `#include("cssc")
csscmod;` is a separate module whose `cli(...)` shells out to the toolchain.

## `alias::member` dispatch

How a missing member behaves depends on what the target is, and this catches people,
so here it is exactly (see also [Access Operators](10-access-operators.md)):

| Target | `alias::member` (read) | `alias::method(args)` (call) |
|--------|------------------------|------------------------------|
| built-in module | missing property gives `null` | missing method is a runtime error, `"Module 'alias' has no function 'method'"` |
| sector | private or missing gives `0x0` | private or missing gives `0x0` |
| loaded module / `.obj` | missing gives `null` | resolves against the file's public members |

The asymmetry to remember: calling a missing built-in-module function is a hard
error, but reaching a private sector member is a silent `null`. So don't lean on a
`null` to tell you a built-in-module call was mistyped; it throws instead.

A loaded CSSC file (`#load`) exposes all of its globals as public members. Loaded-file
modules have no private members.

## `#load`, running another `.cssc` file

```
#load["helpers.cssc"] hlp;
hlp::doThing();
#unload[hlp];
```

This parses and runs `helpers.cssc` in a child runtime, wrapped as a module. Every
global in that file becomes a public member of `hlp`. Path resolution is below.

## `#depend`, loading a `.obj` package

A `.obj` is a self-contained, distributable CSSC package: a compiled DLL plus
optional source and assets. You build them with `cssc install`.

```
#depend['./math-helpers.obj'] mh;
#stack[int, 32] x = mh::mathlib::square(7);   // nested: alias::sector::func
cssc::outln(x);
#delete[x];
#free[mh];      // required for .obj; the loader holds the package open
```

`#depend['path.obj'] alias;` loads the package and exposes its sectors as
`alias::sector::member`. Note that `#free[alias]` is required for a `.obj`: the
package stays open until you free it, and omitting it is a real leak in compiled
builds.

## Teardown: `#unload` versus `#free`

| Directive | What it frees |
|-----------|---------------|
| `#unload[alias]` | Unloads a loaded module (`#load` or `#depend`), running each child sector's `free { }` and then removing it. This is the main way to free a loaded module. |
| `#free[alias]` | Runs teardown on a sector, object, or loaded module, cascading to child sectors. For a built-in `#include` module it does nothing; on anything unexpected it errors. |

The rule of thumb: `#unload` for `#load` and `#depend` modules, `#free` for sectors,
objects, and `.obj` aliases. For a built-in `#include` module, teardown is a no-op in
the interpreter, but keep `#free` for portability to native builds.

None of this is enforced. Nothing errors if you forget, the same as with sectors.
But forgetting `#free` on a `.obj` leaks in compiled builds.

## `#REQUIRE` is not `#req`

Uppercase `#REQUIRE` has nothing to do with `#req`
([Scopes & `#req`](03-scopes-and-req.md)) or with modules by name. It loads an
external resource by path, detecting the kind from the extension (`.dll`, `.ini`,
`.csl`, `.cssl`).

```
#REQUIRE["plugin.dll"] plug;
```

Keep the three straight: `#req` imports a variable across a wall, `#REQUIRE` loads a
resource file, and `#include`/`#load`/`#depend` load modules. There is no `#require`
directive at all.

## Where modules are found

- Built-in `#include` modules come from a hard-coded in-runtime registry. They aren't
  searched on the filesystem.
- `#load` and file paths resolve in order: absolute path, then relative to the running
  script's directory, then each configured search path, then the active version's
  installed-module directory (populated by `cssc module install`).
- `#depend` `.obj` files resolve in order: literal or absolute path, then the current
  working directory, then the script directory, then the per-user installed-package
  store under `%APPDATA%/CSSC/<version>/objects/`.

A `cssc.cproject` file can set the module directory centrally, so `#depend['./x.obj']`
finds packages without hard-coded paths.

## Writing a module: `#MODULE;`

A `.cssc` file meant to be `#load`ed by others should declare itself a module with
`#MODULE;` on its first line:

```
#MODULE;
#auto[string] gGreeting = "hello";     // a global the importer will use

#define(greet) {
    #req[gGreeting] *gGreeting;
    cssc::outln(gGreeting);
}
```

Here's the golden rule for module globals: a module must never `#delete` (or `#free`,
or `#unload`) its own globals. They belong to whoever `#load`ed the file, and freeing
them here would destroy a value the importer still needs. The importer's
`#unload[alias]` frees all of the module's globals. That's exactly what `#unload` is
for.

`#MODULE;` tells the tooling to respect that:

- The analyzer stops flagging the module's un-freed globals and un-freed
  `#include`/`#load` imports, so you don't get leak warnings for file-scope
  allocations. Leaks inside a `#define` are still reported, since a module shouldn't
  leak its locals either.
- `cssc cleanup` and F10 autoclean never insert a global `#delete`, `#free`, or
  `#unload` into a module. They only add the frees that belong inside function
  bodies.

`#MODULE;` does nothing at runtime, in both the interpreter and the native compiler.
It exists purely so the analyzer and cleanup know the file's globals are the
importer's responsibility. It has to sit at the top level, conventionally the first
line.

## The short version

- There's no `#require`. Use `#include`/`#load`/`#depend` for modules, `#req` for
  variable imports, `#REQUIRE` for resource files.
- A `#MODULE;` file never frees its own globals; the importer's `#unload` does. Add
  `#MODULE;` so the analyzer stops asking you to.
- Built-in `#free` does nothing, but `.obj` `#free` is required, since it holds the
  package open.
- A missing built-in-module call errors; a missing sector member is null. Don't
  conflate them.
- `#load` files expose everything as public. No private members in a loaded-file
  module.

## See also

- [Access Operators](10-access-operators.md) for `::` dispatch and the missing-member
  asymmetry.
- [Sectors](06-sectors.md) for sectors versus modules and `#reserve`/`#free`.
- [Directives](08-directives.md) for the full syntax of `#include`, `#load`,
  `#depend`, `#unload`, and `#REQUIRE`.
- [Callables](04-callables.md) for the `def` module directives.
