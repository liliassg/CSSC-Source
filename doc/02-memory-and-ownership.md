# Memory & Ownership

This is the chapter I'd ask you to read slowly. Most of the "CSSC did something I
didn't expect" moments come down to misreading one of the rules here, so it's worth
the time.

CSSC has no garbage collector and makes no copies behind your back. You allocate,
and you free. The only thing that gets cleaned up automatically is `#heap` memory
at the end of the program (plus one scoped-block case, below). Everything else
leaks until you `#delete` or `#free` it. That's deliberate. The whole point of the
language is that nothing happens to your memory that you didn't write.

Here's the one rule everything else grows out of: **a value lives only while a slot
owns it.** A value with no owner is transient. That means a bare heap literal like
`"hi"` or `[1, 2, 3]`, a throwaway call argument, or a borrowed `select` cursor
(`?i`) exists for the current call or expression and is freed the moment it
returns. Bare literals are perfectly fine as transient arguments, which is why
`cssc::outln("hi")` just works. But if you want to keep such a value, you have to
copy it into a slot that owns it, with `&`. Give it an owner or it dies. This one
rule is why references are the default (you're holding the real thing), why
`#delete` on a reference frees the referent, and why a `select` cursor has to be
copied to survive its loop.

## The three regions

Every variable lives in exactly one region, chosen by the directive that declared
it.

| Directive | Syntax | Default capacity | Freed when |
|-----------|--------|------------------|------------|
| `#stack` | `#stack[type, bits] name = init;` | 256 bits | only by `#delete[name]`. Leaks otherwise. |
| `#heap` | `#heap[type, bits] name = init;` | 1024 bits | at program end, or earlier with `#delete`, or at a bare block's `}` if declared inside one (see below). |
| `#auto` | `#auto[type] name = init;` | at least 32 bytes, grows as needed | only by `#delete[name]`. Leaks otherwise. |

The `bits` number is a capacity in bits, not bytes and not an element count.
`#stack[int, 32] x = 5;` reserves 32 bits, and writing a value that doesn't fit is a
hard runtime error, not a silent wraparound.

```
#stack[string, 256] name = "Hello";          // 256-bit buffer, you must #delete it
#heap[vector<int>, 1024] data = [1, 2, 3];   // freed at program end
#auto[int] counter = 0;                      // grows as needed, you must #delete it

#delete[name];
#delete[counter];
```

Floats are the exception to picking a width: `#stack[float, N]` with `N` below 64 is
rejected, because a float is always 64-bit. The minimum is `#stack[float, 64]`. See
[Types & Values](01-types-and-values.md).

Choosing between them is usually simple. Use `#stack` for locals and members whose
lifetime you'll manage yourself. Use `#heap` for large or long-lived data that
should survive ordinary scope exit and is guaranteed gone at program end even if you
forget. Use `#auto` when you don't want to think about the bit count, keeping in
mind you still have to `#delete` it.

## When things actually get freed

This is the part people misread most, so let me be exact about it.

`#heap` is not freed when a block ends. It's freed at the end of the program.
Ordinary control-flow blocks, `if`, `else`, `for`, `while`, and `select`, do not
free heap allocations you declare inside them.

There are two separate ideas here that are easy to confuse: whether a name is still
visible, and whether the buffer is still alive. A barrier scopes the first; the
region rule governs the second. Concretely:

- A bare `{ … }` block is a scoped heap arena. It sets up a fresh heap store for its
  body and throws it away at the closing `}`. `#heap` allocations declared directly
  inside a bare block die at that `}`.
- A `#define`, `#cdefine`, or label body restores name bindings at `}`, so its
  locals stop being visible, but it does not free the underlying `#stack`, `#heap`,
  or `#auto` buffers on return. Those follow the normal region rule: stack and auto
  have to be deleted or they leak, and heap lives until the program ends. That's
  exactly why every `#define` that allocates a local also deletes it.
- Object and sector members get handed to the object or sector and are released by
  `#free`. See [Objects](05-objects.md) and [Sectors](06-sectors.md).

```
#heap[vector<int>, 1024] g = [];   // top level: lives until program end

if (cond) {
    #heap[vector<int>, 1024] a = []; // 'if' is not a barrier, so 'a' survives the
}                                    // block and is freed only at program end

{
    #heap[vector<int>, 1024] b = []; // a bare {} is a scoped heap arena,
}                                    // so 'b' is thrown away right here, at the }
```

So the mental model is: a bare `{}` block discards the heap it made, everything else
follows its region rule (stack and auto manual, heap at program end), and a
barrier's `}` only takes away name visibility.

The trap is thinking heap frees at the end of a block the way a scoped C++ object
does. Only the bare `{}` arena behaves that way. An `if` or `for` body doesn't free
its heap, and a `#define` body doesn't free its stack or auto buffers on return.
[Scopes & `#req`](03-scopes-and-req.md) has the full list of which constructs are
barriers.

## Hex-keyed variables

A variable name is usually an identifier, but it can also be a hex literal, which
puts the variable in a global hex-keyed store (a uint64 key lookup instead of a
string one). It's a small performance convenience, handy on embedded targets.

```
#stack[int, 32] 0x0AA = 42;
cssc::outln(0x0AA);     // 42, resolved as a variable lookup
#delete[0x0AA];
```

The same hex literal used in arithmetic stays an ordinary integer constant as long
as no variable with that key exists. And hex-keyed variables are checked before raw
memory addresses, so `#delete[0x0AA]` deletes the variable, not "address 0x0AA".

## `#delete`

```
#delete[name];
```

This frees the slot and runs a teardown cascade over its contents. Releasing a
container walks its members and drops a reference on each, and when a referenced
heap object hits zero references, its own teardown runs (for an object, that's its
`free { }` block). You don't have to track references by hand; the cascade does it.
Your job is to put `#delete` where a slot needs to die before its default
end-of-life.

### Deleting a reference frees the whole chain

When you delete a slot that's a live reference to a caller's slot, whether it's a
parameter bound by `f(x)` or a `#req` reference import, the delete travels up the
entire chain of links, not just one level. It'll follow a chain up to 16 deep.

```
#stack[int, 32] f;
#define(f) {
    #scanp(f, int, 0) p;   // p is a live reference to the caller's argument slot
    #delete[p];            // frees p and every source slot p links to
}

#stack[int, 32] x = 10;
f(x);                      // passed by reference, which is the default
cssc::outln(x);            // 0x0, because x was released through the cascade
```

If `p` refers to `q` which refers to `y`, then `#delete[p]` releases `p`, `q`, and
`y`.

A copy, on the other hand, is yours alone. If the call site passes a copy with
`h(&y)`, the parameter is an independent slot, so `#delete[p]` frees only that copy
and leaves the original alone:

```
#stack[int, 32] h;
#define(h) {
    #scanp(h, int, 0) p;   // a parameter, but bound to a copy at the call site below
    #delete[p];
}

#stack[int, 32] y = 10;
h(&y);                     // pass a copy
cssc::outln(y);            // 10, y survives; only the copy was freed
```

In one line: deleting a reference frees the referent and its whole chain; deleting a
copy frees only the copy. Since CSSC binds by reference by default (parameters,
`#req`, `select` cursors), reach for `&` whenever you want an independent lifetime.
It's the same reason a `select` cursor has to be copied to outlive its loop, in
[Control Flow](09-control-flow.md).

There's a related behavior worth separating out. When you assign to a reference
parameter and the function returns, the new value is written back to the immediate
caller's slot only, one level up. So the two behaviors differ in depth: deleting a
reference walks the full chain, while a parameter's mutation on return writes back
just one level.

### Guarding a maybe-freed slot

`#adress[var]` as an expression gives you the variable's address as an int, or `0x0`
when it isn't bound. That's the guard to use before a possibly redundant delete
(more on `#adress` below).

## `#delmember`: empty a container, keep it

Use `#delmember` when you want to clear a container's contents without freeing the
container itself. A render buffer you reuse every frame is the classic case.

| Form | What it does |
|------|--------------|
| `#delmember[container];` | walk every entry and release its heap content. Size and capacity stay the same. |
| `#delmember[container[idx]];` | wipe only entry `idx`. Its slot stays in the container as `0x0`; the others are untouched. |
| `#delmember[container[i][j]];` | wipe only cell `j` inside nested element `i`. Everything else stays; that one cell becomes `0x0`. |

What "wipe" means per container:

| Container | `#delmember[c[i]]` | `#delmember[c]` |
|-----------|--------------------|-----------------|
| `vector`/`array` of `int` or `float` | `c[i] = 0` | all slots to 0 |
| `map<string, int>` | that bucket: key released, value to 0 | all buckets wiped |
| `bind<string, string>` | that pair: both strings released | all pairs wiped |
| `array<bind>` or nested arrays | that entry: heap content released, entry to `0x0` | all entries wiped |

`#delmember` is idempotent (wiping an already-wiped slot does nothing) and safe on
null or empty containers (it just returns).

```
#stack[array<bind>, 1024] Buffer;
tick:
    Buffer.push_back({0, 19, "hello"});
    render(Buffer);
    #delmember[Buffer];        // clear the contents, keep the allocation
```

`#delete[c]` already releases the members through its cascade, so
`#delmember[c]; #delete[c];` is redundant. Just `#delete[c]`.

### A worked example with references

`#delmember` wipes the slot inside the container, not whatever the slot points at.
If the wiped slot held a reference, the thing it referred to is untouched; only the
container's copy of the reference goes to `0x0`. A copy element (stored with `&`) is
simply zeroed. Here's the whole picture in one program. If you can trace it, you
understand CSSC's value, reference, and lifetime model:

```
#stack[int, 32] x = 3;
#stack[array<int>, 1028] f = {&x, *x};  // f[0] = &x (frozen copy = 3), f[1] = *x (live ref)
x = 32;
cssc::outln(f);                          // {3, 32}: copy frozen at 3, ref followed to 32
x = 100;                                 // f[1] (the ref) now reads 100

#stack[array<int>, 2048] n = {&f, *f};  // n[0] = &f (deep copy of f right now = {3, 100}, frozen)
                                         // n[1] = *f (live ref to the whole f container)
f.push_back(&x);                         // f gains a third cell: &x = frozen copy = 100
x = 500;                                 // f[1] (the ref to x) now reads 500
cssc::outln(n);                          // {{3, 100}, {3, 500, 100}}
                                         // n[0] is the frozen snapshot; n[1] tracks live f

#delete[x];                              // every live reference to x now dangles to 0x0
cssc::outln(n);                          // {{3, 100}, {3, 0x0, 100}}
                                         // n[1][1] was *x, now 0x0; the &-copies survive

#delmember[n[0]];                        // wipe n's first element to 0x0 (n stays size 2)
cssc::outln(n);                          // {0x0, {3, 0x0, 100}}

#delmember[n[1][0]];                     // wipe cell 0 inside n[1] to 0x0
cssc::outln(n);                          // {0x0, {0x0, 0x0, 100}}

#delete[f];
#delete[n];
```

Two things to take from it. `&` freezes at the moment you write it, so `&f` captured
`{3, 100}`, the value `f` had then, not its earlier `{3, 32}`. And `#delmember`
edits the container in place, one cell or the whole thing, while `#delete` on a
referent is what makes the references to it read `0x0`.

## `#free`: objects, sectors, module aliases

`#free[X]` runs `X`'s `free { }` teardown and then tears the entity down. It's the
right teardown for objects, sectors, and loaded module or `.obj` aliases, not
`#delete`.

```
#free[Engine];   // runs Engine's free { } then destroys the sector
#free[mh];       // releases a #depend'd .obj alias (see Modules)
```

The three teardown operators side by side:

| Operation | The container/entity | Its contents |
|-----------|----------------------|--------------|
| `#delete[c]` | freed | freed (cascade) |
| `#free[c]` | freed (object/sector/module) | freed (`free {}` block plus cascade) |
| `#delmember[c]` | kept | freed |

## References and copies: what points at what

This is the section people most often mis-remember, so here's the whole thing.
There are two separate ideas, and keeping them apart is what makes this click.

First, what a plain `b = a` does depends on the type of `a`. Second, a live
cross-slot reference, where writing `a` is seen through `b` even for a scalar, comes
only from a small set of explicit places, never from a plain `b = a`. And wherever
you genuinely want an independent clone, `&a` gives you a deep copy.

### Plain assignment `b = a`

- A scalar (`int`, `float`, `bool`) is value-copied. `b` is its own slot, and a
  later change to `a` doesn't touch `b`.
- A string is value-copied. Independent.
- A container (`array`, `vector`, `map`, `bind`, object, sector) is aliased. `b` and
  `a` refer to the same underlying container, and a change through either name shows
  up through the other.

```
#stack[int, 32] a = 42;
#stack[int, 32] b = a;     // scalar, so a value copy
a = 100;
cssc::outln(b);            // 42, b never moved

#stack[vector<int>, 256] xs = {1, 2, 3};
#stack[vector<int>, 256] ys = xs;   // container, so an alias
xs.push_back(4);
cssc::outln(ys.size());    // 4, it's the same container
```

The asymmetry is on purpose. A scalar or string is its value, so copying it is cheap
and almost always what you meant. A container is a heap object you reach through a
handle, and copying the whole thing on every `=` would be both surprising and
expensive, so a plain assignment shares the handle. When you want the other
behavior, say so: `&xs` deep-copies a container, and the reference sites below give
you a live link to a scalar.

### Deep copy with `&a`

`&a` is a recursive deep copy. It makes `b` fully independent, even for containers.
For a scalar, `&a` reads the same as `a`, since a scalar is already a value.

```
#stack[vector<int>, 256] xs = {1, 2, 3};
#stack[vector<int>, 256] ys = &xs;   // an independent clone
xs.push_back(4);
cssc::outln(ys.size());              // 3, ys never saw the push
```

### Where live references actually come from

A live cross-slot reference, where a write through one name is seen through the
other for every type including scalars, comes only from these four places. A plain
scalar declaration like `#stack[int] b = a;` is not one of them; that's the value
copy above.

1. **Argument passing.** `f(x)` passes a live reference, so the callee can write back
   to your slot; `f(&x)` passes a deep copy. The call site decides. A callee
   `&param` hint is only an analyzer note, never an override. See
   [Callables](04-callables.md).
2. **Storing into a container.** `c.push_back(x)` stores a live reference to `x`;
   `c.push_back(&x)` stores a frozen copy. Every store method follows this
   (`push_back`, `push_front`, `insert`, `append`, `set`, the `emplace_*` family,
   `push`, `pushafter`).
3. **`#req` import.** `#req[X] Y;` binds `Y` as a live reference to `X`; `#req[X] &Y;`
   is a deep-copy snapshot. See [Scopes & `#req`](03-scopes-and-req.md).
4. **An explicit `*x`.** Writing `*x` as an argument or a container element takes a
   reference on purpose, spelled out. `{&x, *x}` stores a frozen copy of `x` and a
   live reference to `x`, side by side.

```
#stack[array<int>, 64] live = {};
#stack[array<int>, 64] frozen = {};
#stack[int, 32] x = 3;
live.push_back(x);      // bare, so a live reference
frozen.push_back(&x);   // &x, so a frozen copy
x = 500;
cssc::outln({live});    // {500}, it followed x
cssc::outln({frozen});  // {3}, independent
```

Because bare means reference, a loop like `while (i < n) { c.push_back(i); i++; }`
stores `n` references to the same `i`, and every element ends up equal to `i`'s
final value. To collect distinct values, store copies with `c.push_back(&i)`. The
analyzer flags the related dangling case too: a bare or `*x` store into a container
that's never freed, followed by `#delete[x]`, leaves the stored reference reading
`0x0`. Fix it with a copy, or free the container before the referent.

### Indexing `list[i]`

Indexing in place is a live view: `list[i] = v` writes through, and
`list[i].method()` mutates the real element. But binding `list[i]` into a new scalar
value-copies it, like any scalar assignment:

```
#stack[vector<int>, 256] list = {10, 20, 30};
list[1] = 99;                     // in place, list is now {10, 99, 30}

#stack[int, 32] elem = list[1];   // a copy of 99, its own slot
list[1] = 50;
cssc::outln(elem);                // 99, unchanged
```

### Containers keep your values alive

When you add a variable to a container, the container holds a reference to that
value. Deleting your local slot drops one reference, but the value survives as long
as the container still holds it.

```
#heap[array<auto>, 1024] queue;
{
    #stack[int, 32] a = 5;
    queue.add(a);      // queue now references a's value
}                      // a's local slot is gone (bare {} barrier)
cssc::outln(queue[0]); // 5, the value lives on through queue
#delete[queue];        // the cascade releases queue[0]'s reference, freeing it
```

## Addresses: `#adress` and `#reflect`

```
#adress[var] addr;              // read the real memory address into addr
#reflect[addr] var;             // resolve an address back to its value

if (#adress[maybe] != 0x0) {    // guard: 0x0 means not bound
    #delete[maybe];
}
```

As a statement, `#adress[var] a;` stores the address. As an expression,
`#adress[var]` is the address as an int, or `0x0` if unbound. `null` and `0x0` are
the same null sentinel, as in [Types & Values](01-types-and-values.md).

One spelling note: the directive is `#adress`, with one `d`. `#address` doesn't
exist, and neither does `#memory` (the parser rejects it as a removed name). Don't
"correct" the spelling.

## Reshaping allocations

```
#reallocate[var, type, stack, 512] newvar;   // move var into a new region/size
#resize[buf, 64];                             // grow the existing alloc by 64 bits
#resize[buf, -32];                            // shrink it by 32 bits
```

- `#reallocate[var, type, stack|heap, size?] newvar;` is a real region move into a
  fresh stack or heap buffer. It's type-strict, so the `type` has to match, with no
  coercion. Leave `size` off and it grows by the default 32 bits.
- `#resize[var, ±bits];` grows or shrinks an existing allocation in place, following
  the reference chain to the real buffer.
- `#cast[source, target] result;` is an explicit coercion into an existing `#heap`
  target.
- `#set[0xADDR, bits] = value;` writes a coerced value at a known address, with the
  bit limit checked.

These are all manual, deliberate operations. No hidden copies beyond what you ask
for, and no automatic growth for `#stack` or `#heap` (only `#auto` grows on its
own). The full syntax for each is in [Directives](08-directives.md).

## The short version

- Heap frees at the end of the program, not the end of a block. Only a bare `{}`
  block frees its heap early. `if`, `for`, `while`, and `select` don't.
- `b = a` makes `b` track `a` only for containers, which alias. Scalars and strings
  are value-copied. A live reference to a scalar comes only from argument passing, a
  container store, `#req`, or an explicit `*x`, never from a plain `b = a`.
- `#delete` on a reference cascades up the entire chain and frees every source slot
  the reference links to. If you meant to keep the caller's slot, pass a copy with
  `f(&x)`, or don't delete.
- `#stack` and `#auto` never clean themselves up. You `#delete` them, or they leak.
  Only `#heap` self-cleans, at program end.
- `#delmember` keeps the container and clears its contents; `#delete` frees the
  container too.
- Objects, sectors, and modules use `#free`, not `#delete`, so their `free { }` block
  runs.

## See also

- [Scopes & `#req`](03-scopes-and-req.md) for isolation barriers and `#req` reference
  versus snapshot.
- [Callables](04-callables.md) for call-site reference or copy, and `mirror` versus
  `return`.
- [Types & Values](01-types-and-values.md) for what scalar versus container means,
  and `null`/`0x0`.
- [Directives](08-directives.md) for the exact syntax of every `#…` directive.
