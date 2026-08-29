# CSSC

Control Specified Source Compiling. A small, strongly typed language for people
who want to know exactly what their code does to the machine underneath.

I started it because MicroPython kept letting me down on the ESP32 and ESP8266,
heap fragmentation ruined projects I cared about, and C felt wrong for the way I
wanted to write embedded code. So I built something with roughly the readability
of Python, small binaries, and tight control over where every byte goes. It has
an x86_64 backend now too, so it runs ordinary host programs as well.

There is no garbage collector and nothing is cleaned up behind your back. You
decide where each value lives and when it goes away.

**[cssclang.com](https://cssclang.com)**

## What is in here

| Path | What it is |
|------|------------|
| `bytecodeinterp/` | The interpreter, written in CSSC. This is the reference behavior. |
| `transembly/` | The self-hosted compiler backend, written in CSSC. |
| `semantic_analyisis/` | The analyzer behind the diagnostics and the language server. |
| `c/` | The C runtime the compiled output links against. |
| `modules/`, `user/` | Built-in modules and example programs. |
| `doc/` | The language reference. Start at [`doc/00-getting-started.md`](doc/00-getting-started.md). |
| `web/` | The source of cssclang.com. |
| `webtools/` | Build scripts for the site. |

## The toolchain

| Command | What it does |
|---------|--------------|
| `cssc run <file>` | Runs the file on the interpreter. When something is ambiguous, whatever this does is what CSSC means. |
| `cssc build <file> [-o out]` | Compiles to a native executable. Embedded targets: `--esp32`, `--esp8266`, `--arduino`. |
| `cssc workspace` | Opens the CSSC IDE. |
| `cssc analyze <file>` | Diagnostics and lints, without running anything. |

## Where it stands

Xtensa, so ESP32 and ESP8266, is tested and working. The host backend runs on
Windows and on Linux. ARM is not supported yet. Parts of the toolchain, the CLI
among them, are still stage 0 and run on Python.

This is honest work in progress. Where something only works on one backend, the
documentation says so rather than pretending everything is finished.

## Documentation

The reference lives in [`doc/`](doc/). Each chapter stands on its own.

If the docs and the language ever disagree, trust the language and tell me about
the doc bug so I can fix it.
