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

The reference lives in [`doc/`](doc/) and is also readable at
[cssclang.com/docs](https://cssclang.com/docs/). Each chapter stands on its own.

If the docs and the language ever disagree, trust the language and tell me about
the doc bug so I can fix it.

## License, and the name

The code is under the [Apache License 2.0](LICENSE). Use it, ship it, build
things with it, commercially or not. You do not owe me anything for that.

The name is a separate matter, and I want to be clear about it rather than leave
it to be guessed. **CSSC**, **Control Specified Source Compiling** and the star
are mine and are not part of the license. Section 6 of Apache 2.0 says so
explicitly: it grants no trademark rights.

In practice that means: if you fork this and take it somewhere I did not, that
is allowed and I would rather you did that than nothing. But give it your own
name. CSSC is the thing I have been building for three years and I would like
that name to keep meaning this project.

If you want to contribute back instead of forking, that is what I would prefer.
Open an issue.
