# CSSC Language Reference

CSSC stands for Control Specified Source Compiling. It's a small, strongly typed
language I've been building for a couple of years, aimed at people who want to know
exactly what their code does to the machine underneath.

I started it because I was frustrated. I wanted to write software for the ESP32 and
ESP8266, and MicroPython kept letting me down. Heap fragmentation ruined several
projects I actually cared about. C was the obvious answer, but it felt awkward for
the way I wanted to write embedded code. So I started making my own language:
something with roughly the readability of Python, but with small binaries and much
tighter control over where every byte goes. That's still the core of what CSSC is
for. It also has an x86_64 backend now, so you can write ordinary host programs
with it too.

It's honest work-in-progress. The Xtensa target (ESP32, ESP8266) is tested and
working. The host backend runs on Windows. Linux isn't tested yet and ARM isn't
supported yet. I'll say so in the chapters wherever it matters, rather than pretend
everything is finished.

This is the reference. Each chapter covers one area and stands on its own, so you
can read it front to back or jump straight to what you need.

If you're new, start with [Getting Started](00-getting-started.md). It installs
CSSC, gets a program running, and sends you to the chapters that matter for what
you're building.

## Chapters

| # | Chapter | What's in it |
|---|---------|--------------|
| 00 | [Getting Started](00-getting-started.md) | Install, write and run your first program, and the few ideas you need up front. |
| 01 | [Types & Values](01-types-and-values.md) | The primitive types and their sizes, `null` and `0x0`, literals, and the container types. |
| 02 | [Memory & Ownership](02-memory-and-ownership.md) | `#stack`, `#heap`, `#auto`, when things get freed, `#delete` and `#free`, and how aliasing works. Read this one slowly. |
| 03 | [Scopes & `#req`](03-scopes-and-req.md) | Which constructs hide outer names and which don't, how names resolve, and importing with `#req`. |
| 04 | [Callables](04-callables.md) | `#define`, parameters, passing by reference or by copy, `mirror` versus `return`, and how a variable can also be a function. |
| 05 | [Objects](05-objects.md) | `object` layout, members with `->`, labels with `.`, overloading, and constructor parameters. |
| 06 | [Sectors](06-sectors.md) | `sector` namespaces, real `public`/`private` enforcement, isolation, and dependency injection. |
| 07 | [Modules](07-modules.md) | `#include`, `#load`, `#depend`, `#unload`, and where CSSC looks for modules. |
| 08 | [Directives](08-directives.md) | The full `#…` directive reference, grouped by what they do. |
| 09 | [Control Flow](09-control-flow.md) | `if`, `for`, `while`, and `select` with `jump` for walking containers. |
| 10 | [Access Operators](10-access-operators.md) | When to use `::`, `->`, or `.`. They aren't interchangeable, and mixing them up is the first thing that trips everyone up. |

## What I built it around

A few goals shaped almost every decision in the language.

I wanted it to be fast, and I mean measured fast, not assumed fast. Hot paths
compile to native code and the compiler throws away anything you don't use. When
speed matters, I benchmark it rather than guess.

I wanted it to be raw. There's no garbage collector and nothing gets cleaned up
behind your back, apart from `#heap` memory at the end of the program. You decide
where each value lives and when it goes away. On a chip with a few kilobytes of
RAM, that kind of control is the entire reason the language exists.

I wanted the binaries to stay small. Dead-code elimination and a minimal runtime
keep the output tight, which is what makes it usable on flash-constrained boards.

And I wanted one source to reach many places. The same `.cssc` file runs on the
interpreter, compiles to native code for your machine, and builds for the Xtensa
and AVR chips.

The tradeoff is real and I won't hide it: CSSC does exactly what you wrote,
including leaking memory you forgot to free. The rules in these chapters are the
contract that makes that tradeoff pay off.

## The toolchain

| Command | What it does |
|---------|--------------|
| `cssc run <file>` | Runs the file on the interpreter. This is the reference behavior. When something is ambiguous, whatever `cssc run` does is what CSSC means. |
| `cssc build <file> [-o out]` | Compiles to a native executable. Host builds use LLVM. Embedded targets: `--esp32`, `--esp8266`, `--arduino`. |
| `cssc workspace` | Opens the native CSSC IDE. |
| `cssc analyze <file>` | Static analysis: diagnostics and lints, without running anything. |
| `cssc module install …` | Builds and installs distributable modules. |
| `cssc uninstall` | Removes CSSC from your machine. |

The interpreter, the native compiler, and the analyzer are supposed to agree on
everything written here. Where a feature only works on one of them, the chapter
says so.

## The things people get wrong first

A short list, each linked to the chapter that explains it properly.

- Plain `b = a` doesn't always create a live link. Scalars and strings are copied
  by value; only containers share their storage. To link two slots, use `#req` or
  pass through a function argument. See [Memory & Ownership](02-memory-and-ownership.md).
- `#heap` is freed at the end of the program, not when a block ends. The one
  exception is a bare `{ }` block, which drops the heap it made when it closes. See
  [Memory & Ownership](02-memory-and-ownership.md).
- The call site decides reference or copy: `f(x)` passes a reference, `f(&x)`
  passes a copy. See [Callables](04-callables.md).
- `::`, `->`, and `.` mean different things and can't be swapped. See
  [Access Operators](10-access-operators.md).
- `select` needs a `jump`. Without it the cursor never moves and you loop on the
  first element forever. See [Control Flow](09-control-flow.md).
- `float` is always 64-bit. A narrower float slot is rejected outright. See
  [Types & Values](01-types-and-values.md).

## One note on precedence

These docs describe how CSSC actually runs. If the text and the language ever
disagree, trust the language and let me know about the doc bug so I can fix it.
