# Scopes & `#req`

CSSC scoping isn't C scoping, and there's one rule that catches everyone at least
once: some blocks are walls and some are windows. Inside a wall, your code can't see
outer names at all, and it has to import what it needs with `#req`. Inside a window,
it sees the enclosing scope directly.

Get this backwards and you hit the classic "why is my variable `0x0` inside this
block?" bug. So here's the full list.

## The scope model

The top level is one flat scope. Every top-level `#stack`, `#heap`, and `#auto`
declaration, and every top-level `#define`, lives together in it.

A wall (a barrier) installs a fresh frame when you enter and restores the previous
one when you leave. Names declared inside a wall don't leak out, and names outside
it aren't visible in, except through the resolution rules below.

A window doesn't install a frame. It reads and writes the enclosing scope directly.

## Walls and windows

| Construct | Wall? | Sees the enclosing scope? | Inner names gone after `}`? |
|-----------|-------|---------------------------|-----------------------------|
| `#define(f) { … }` body | wall | no, import with `#req` | yes |
| `#cdefine(f, …) { … }` body | wall | no, import with `#req` | yes |
| bare `{ … }` block | wall | no, import with `#req` | yes |
| object body | wall | no (it has its own members) | yes (members live until `#free`) |
| label body | wall | object members via implicit `this`, otherwise `#req` | yes |
| sector body | wall | no (its own members plus `<deps>`) | members live until `#free` |
| `if` / `else` block | window | yes | its own locals released at `}` |
| `for ( … ) { … }` | window | yes | loop var and locals released at `}` |
| `while ( … ) { … }` | window | yes | its locals released at `}` |
| `select ( … ) ?n { … }` | window | yes | the `?n` cursor released at `}` |

The way I remember it: `#define`, `{}`, object, label, and sector are walls; `if`,
`for`, `while`, `select`, and `else` are windows.

One thing to keep separate: this table is about whether a name is visible, not about
when the memory is freed. Only a bare `{}` block discards the `#heap` it made at its
`}`. A `#define` body restores its names but leaves `#stack` and `#auto` buffers for
you to `#delete`, and `#heap` lives until the program ends. See
[Memory & Ownership](02-memory-and-ownership.md).

```
#stack[int, 32] x = 50;

if (x < 100) {
    cssc::outln(x);      // 50, an 'if' is a window and sees the outer x
}

{
    cssc::outln(x);      // 0x0, a bare {} is a wall, so x is invisible here
}

{
    #req[x] xr;          // import x across the wall (a live reference by default)
    cssc::outln(xr);     // 50
}
```

The trap is treating a bare `{ … }` like a C block. In C it sees the enclosing
scope; in CSSC it's a wall, and you bring names in with `#req`.

## How names resolve inside a wall

Inside a wall, an unqualified identifier resolves in this order:

1. declarations local to this frame,
2. `#req` imports declared in this body,
3. parameters (`#scanp`, label params) and `<deps>` injected into a sector or object,
4. `cssc::` built-ins,
5. the wall's own members (sector or object members, via `->` or implicitly).

A bare name that matches none of these does not fall back to the global scope. In a
sector function, an unknown bare name resolves to `0x0`. There's no accidental global
read or write.

This is deliberate. Without an implicit outer-scope fallback, a function can't
silently read or clobber some unrelated top-level slot. If you need an outer name,
import it with `#req`, or for sectors and objects, inject it through `<deps>`.

## `#req`: importing across a wall

`#req` is the only way to pull an outer name into a `#define` or `{}` body. It has
two forms.

| Form | Meaning |
|------|---------|
| `#req[X] Y;` | A live reference, the default. `Y` reads and writes through to `X`, so `Y += 1` really increments `X`. |
| `#req[X] &Y;` | A snapshot, a deep copy. `Y` is an independent frozen copy of `X` taken at import time, and writes to `Y` don't touch `X`. For a scalar this is indistinguishable from a reference; for a string, container, object, or sector it's a recursive deep copy. |

There's also `#req[X] *Y;`, a deprecated older spelling of the live-reference form
with identical meaning. New code leaves off the `*`.

```
#stack[int, 32] hits = 0;

#define(beat) {
    #req[hits] h;      // a live reference, so mutating h mutates hits
    h += 1;
}

beat();
cssc::outln(hits);     // 1, the write went through the reference
```

The snapshot form:

```
#stack[int, 32] cfg = 10;

#define(readonly) {
    #req[cfg] &c;      // a snapshot: c is a private copy
    c = 999;           // doesn't touch cfg
}

readonly();
cssc::outln(cfg);      // 10
```

`#req` is reference-by-default, which matches the argument-passing default where
`f(x)` is a reference (see [Callables](04-callables.md)). The write-back is real, so
`#delete[Y]` inside the body invalidates the outer `X` too.

If you're porting older code, note that this flipped at some point. It used to be
that `#req[X] Y;` meant snapshot and `#req[X] *Y;` meant reference. Now bare `Y` is
the reference, `&Y` is the snapshot, and `*Y` is a deprecated alias of the
reference. If you relied on `#req[X] Y;` being a copy, change it to `#req[X] &Y;`.

## Importing other functions

Top-level `#define` functions are just variables in the flat top scope, so you
import them the same way and call through the alias.

```
#stack[int, 32] helper;
#define(helper) { return 7; }

#stack[int, 32] caller;
#define(caller) {
    #req[helper] h;          // import the function
    #stack[int, 32] r = h(); // call it, giving 7
    return r;
}
```

## `#DEFINE` is not `#define`

Uppercase `#DEFINE` is a different thing from lowercase `#define`. Lowercase
`#define` declares a callable (see [Callables](04-callables.md)). Uppercase `#DEFINE`
is a compiler and transpiler directive with three uses:

```
#DEFINE __main__ '__main__';   // opt in as the entry point
#DEFINE somename;              // foreign-scope passthrough
#DEFINE MAX <expr>;            // a compile-time constant (native/transpiler path)
```

In the interpreter, `#DEFINE` does nothing at runtime. It doesn't create a constant
you can read while running. Its compile-time-constant meaning is for the native and
transpiler paths, not `cssc run`. So don't count on `#DEFINE NAME value;` expanding
inside interpreted code; use a real `#stack` or `#heap` slot for a runtime value.

Case is the whole difference: `#DEFINE` is a compiler construct, `#define` is a
function. Both are in [Directives](08-directives.md), along with `#redefine`, which
mutates a function body and is interpreter-only.

## The short version

- A bare `{}` is a wall, not a window. Code inside can't see outer names; import
  them with `#req`.
- `if`, `for`, `while`, and `select` are windows. They see the enclosing scope, and
  you don't need `#req` inside them.
- There's no global fallback across a wall. An unknown bare name in a sector
  function is `0x0`, not the top-level variable with the same name.
- `#req` defaults to a live reference, not a copy. Use `&` for a snapshot. This is
  the reverse of the old semantics.
- `#DEFINE` and `#define` are different. Uppercase is a constant, lowercase is a
  function.

## See also

- [Memory & Ownership](02-memory-and-ownership.md) for aliasing, the delete cascade,
  and how a bare block discards its heap.
- [Callables](04-callables.md) for `#define`, parameters, and call-site reference or
  copy.
- [Sectors](06-sectors.md) for sector bodies as walls and `<deps>` injection.
- [Objects](05-objects.md) for object and label bodies as walls.
