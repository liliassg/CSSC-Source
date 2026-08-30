# Getting Started

This takes you from a fresh install to a running program and the handful of ideas
you need before the rest of the reference makes sense. Give it ten minutes.

## Install

Download the release zip and extract it. Windows and Linux both get a visual
installer, so it's the same on either one: run it, accept the license, click
Install, and open a new terminal when it's done.

Then check it worked:

```
cssc --version
```

You should see the version print. If the command isn't found, close the terminal
and open a fresh one. The installer puts `cssc` on your PATH, and only new
terminals pick that change up.

One thing to know up front: the first time you compile something, CSSC downloads
its LLVM backend. That's a one-time thing and it can take a few minutes. Everything
after that is quick.

## Your first program

Put this in a file called `hello.cssc`:

```
cssc::outln("Hello from CSSC");
```

Run it on the interpreter:

```
cssc run hello.cssc
```

`cssc::outln` prints a line. `cssc::` is the built-in namespace, always there, no
import needed.

Now compile the same file to a real executable:

```
cssc build hello.cssc -o hello
```

That gives you `hello` (or `hello.exe` on Windows) that runs on its own. Same
source, interpreted and compiled, no changes in between. That's the whole idea
behind CSSC: you write it once and it reaches a lot of places.

## Every variable has a type and a size

You declare a variable with its type and how many bits it gets:

```
#stack[int, 32] count = 10;
#stack[int, 8]  small = 200;
#stack[float, 64] ratio = 1.5;
#stack[string, 64] name = "Ada";
cssc::outln(count);
cssc::outln(name);
```

`#stack[int, 32]` means a 32-bit integer that lives on the stack. The size is part
of the deal. You're telling the machine exactly how much room to set aside. Floats
are the one exception to picking a width: they're always 64-bit, and a narrower
float slot gets rejected.

The full story is in [Types & Values](01-types-and-values.md).

## You free what you allocate

CSSC doesn't tidy up after you. When you're done with something, delete it:

```
#stack[int, 32] x = 42;
cssc::outln(x);
#delete[x];
```

Stack values are cheap and short-lived. When you need memory that outlives the
current block, you reach for `#heap`, and heap memory sticks around until the
program ends unless you free it yourself.

This is strict, and it's meant to be. It's the reason CSSC can run with no garbage
collector on a chip that has almost no memory to spare. The rules for what lives
how long are in [Memory & Ownership](02-memory-and-ownership.md), and it's the
chapter I'd read twice.

## Functions

A function is a `#define`. It reads its arguments with `#scanp` and hands back a
result with `return`:

```
#stack[int, 32] square;
#define(square) {
    #scanp(square, int, 0) n;
    return n * n;
}
square(7) result;
cssc::outln(result);
```

Two things worth pointing out. You declare the name `square` as a normal variable
first, then define it as a function. In CSSC a function is a value that lives in a
slot, same as any other. And you call it with `square(7) result;`, which runs it
and captures the answer into `result`.

There's more on arguments and passing by reference or copy in
[Callables](04-callables.md).

## Loops and containers

A `vector` grows as you push to it. To walk one, use `select` with a `jump`:

```
#stack[vector<int>, 64] nums;
nums.push_back(3);
nums.push_back(5);
nums.push_back(8);

#stack[int, 32] total = 0;
select (nums) ?n {
    total = total + n;
    jump;
}
cssc::outln(total);
#delete[nums];
```

The `jump` is doing real work here. `select` puts a cursor on the container, and
`jump` moves it to the next element. Leave it out and the cursor never advances, so
you loop on the first element forever. It's a common slip, and the analyzer will
warn you about it.

[Control Flow](09-control-flow.md) covers `if`, `for`, `while`, and `select` in
full.

## Where to go next

That's enough to write real programs. From here, follow whatever you're building:

- Structuring something bigger? [Objects](05-objects.md) for data with behavior,
  [Sectors](06-sectors.md) for namespaces with real privacy.
- Pulling in library code? [Modules](07-modules.md).
- Hit a memory error, or something got freed too early?
  [Memory & Ownership](02-memory-and-ownership.md) and
  [Scopes & `#req`](03-scopes-and-req.md).
- Tangled up in `::` versus `->` versus `.`? [Access Operators](10-access-operators.md).
  This one catches everyone once.

And if you'd rather work in an editor with a file tree, autocomplete, and a
built-in terminal, run `cssc workspace` for the CSSC IDE.
