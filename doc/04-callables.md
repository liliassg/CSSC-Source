# Callables

CSSC has an unusual model, but a consistent one: a function is a variable. There's
no separate "function" thing. You declare a slot, bind a body to it, and calling the
slot runs the body.

## A variable that's also a function

```
#stack[int, 32] add;      // declare a slot
#define(add) {            // bind a body to that slot
    #req[a] a;
    #req[b] b;
    return a + b;
}
```

Behind the scenes the slot carries two facets: `add["value"]`, the last return value
(or `null` if it's never been called), and `add["address"]`, the address of the body.
Calling `add(…)` runs the body and stores the result in `add["value"]`. This duality
is why you can `#req` a function by name and then call it (see
[Scopes & `#req`](03-scopes-and-req.md)), and why one slot can hold both a value and
behavior.

The `#define` body is a wall. It doesn't see outer names, so you import them with
`#req`. See [Scopes & `#req`](03-scopes-and-req.md).

## `#define`, the core form

```
#stack[int, 32] greet;
#define(greet) {
    #req[name] who;                 // import an outer slot by reference
    #stack[string, 64] m = "Hi " + who;
    cssc::outln(m);
    #delete[m];
}
```

`#define(slot) { body }` binds the body to the slot. It's core, so it needs no
module. The body runs in its own isolated scope, and `return` exits it with a value.

## `#cdefine`, named parameters

`#cdefine` lets you name parameters in the signature instead of reading them by
position.

```
#include('def');

#stack[int, 64] multiply;
#cdefine(multiply, a, b) {
    return a * b;
}
cssc::outln(multiply(3, 4));   // 12
```

`#cdefine`, `#fvar`, `#param`, and `#qvar` live behind the `def` module, so put
`#include('def');` once at the top or you'll get a clear "missing module" error.
`#define` and `#redefine` are core and need nothing.

One wrinkle worth knowing: `#scanp` and `#scanp_opt` are described as part of the
`def` family, but the interpreter doesn't actually require the module for them, so
they work without `#include('def')` under `cssc run`. Keep the include anyway if you
use the rest of the family, so your code ports cleanly to the native path.

## Reading parameters with `#scanp`

```
#include('def');

#stack[int, 32] sum;
#define(sum) {
    #scanp(sum, int, 0) x;             // required argument at position 0
    #scanp(sum, int, 1) y;             // required argument at position 1
    #scanp_opt(sum, int, 2) bonus = 0; // optional, missing gives the default 0
    return x + y + bonus;
}

sum(10, 20)    a;   // 30
sum(10, 20, 5) b;   // 35
```

`#scanp(source, type, pos) name;` reads the required argument at `pos`, where
`source` is the callable's own name. `#scanp_opt(source, type, pos) name;` is the
optional version: if there's no argument and no default, `name` resolves to `null`
instead of raising an error. Both take a default clause, so
`#scanp_opt(sum, int, 2) bonus = 100;` uses `100` when the argument is missing.
`#scanp_opt` gives you `null` only when neither an argument nor a default was
supplied, which is what makes it good for variadic-style trailing arguments.

Whether the parameter is a reference or a copy is decided by the call site, not by
the parameter. `#scanp(f, int, 0) name;` binds `name` as a live reference when the
caller passed `f(x)`, or as an independent copy when the caller passed `f(&x)`. A `&`
or `*` on the parameter name doesn't decide this: `#scanp(f, int, 0) &name;` is only
a hint that the callee prefers a copy, and the runtime ignores it; `#scanp(f, int, 0)
*name;` is a deprecated reference spelling, the same as bare `name`.

## `#fvar`, `#param`, `#qvar`

```
#fvar(int) counter;          // a typed function variable (a return-typed slot)
#param(string) inputStr;     // a typed parameter for the next #cdefine
#qvar(int, x + y) result;    // a quick typed local from an expression
```

`#fvar(type) name;` declares a typed callable-variable up front. `#param(type)
name;` declares a typed parameter, written only through `#scanp`. `#qvar(type, expr)
name;` materializes an expression into a typed local in one line. All three need the
`def` module.

## Calling and capturing results

A call can be a bare statement, an expression, or a capture that binds the result
into a new slot:

```
add(3, 4);                       // call and discard (result still lands in add["value"])
#stack[int, 32] r = add(3, 4);   // capture through assignment
add(3, 4) r2;                    // capture form: bind the result into a new slot r2
```

The `f(args) name;` capture form is the idiomatic way to name a call's result.

## Reference or copy at the call site

This is the same rule as the ownership chapter, from the calling side. The call site
decides whether an argument goes by reference or by copy, and the callee can't
override it.

```
#stack[int, 32] f;
#define(f) {
    #scanp(f, int, 0) p;   // reference or copy, depending on how f was called
    #delete[p];            // deletes p; if p was a reference, deletes the caller's slot too
}

#stack[int, 32] x = 10;
f(x);                      // by reference, the default
cssc::outln(x);            // 0x0, p was a live link and #delete[p] took x with it

#stack[int, 32] y = 10;
f(&y);                     // a copy, with an explicit &
cssc::outln(y);            // 10, p was independent and only the copy died
```

`f(x)` passes a reference by default, so mutations and a `#delete` inside `f` reach
the caller's slot one level up. `f(&x)` passes a deep copy, and the callee gets an
independent slot. `f(*x)` is a deprecated spelling of the reference form, the same as
`f(x)`.

And to say it once more, because it surprises people: a callee-side `&param` hint
like `#scanp(f, int, 0) &p;` is ignored. Only the call-site syntax matters.

## `mirror`, `return`, and `destruct`

All three end or pause a body, but they differ in what happens afterward.

| Form | What happens after | The return value |
|------|--------------------|------------------|
| `return value;` | Hard stop. The body ends immediately, and trailing cleanup does not run. | value |
| `mirror value;` | The body keeps running, so trailing `#delete` and `#free` still execute. By default a live reference to the source slot. | value (live reference) |
| `mirror &value;` | Like `mirror`, but a deep-copy snapshot taken at the `mirror` point, so later cleanup can't disturb it. | value (snapshot) |
| `mirror *value;` | A deprecated alias of `mirror value;`. | value (live reference) |
| `destruct;` | Objects only: runs the object's `free { }` block, marks it dead, and returns control to the caller. It's not `exit()`. | — |

`mirror` exists because `return` can't both hand back a value and clean up
afterward. `mirror` can. It sets the return value but lets the rest of the body run,
so you can free copy parameters and transient allocations before the call actually
returns.

The catch is the live-reference default. If you `mirror` a slot and then delete that
slot, the outer capture is invalidated:

```
#stack[int, 32] f;
#define(f) {
    #stack[int, 32] inner = 42;
    mirror inner;          // a live reference to inner
    #delete[inner];        // inner freed, so the outer capture is now 0x0
}
f() out;
cssc::outln(out);          // 0, not 42; the reference was invalidated
```

A snapshot fixes it:

```
#define(f) {
    #stack[int, 32] inner = 42;
    mirror &inner;         // a snapshot copy
    #delete[inner];        // inner freed, but the snapshot survives
}
f() out;
cssc::outln(out);          // 42
```

Mirroring an expression instead of a slot is automatically a snapshot, since there's
no slot for it to reference:

```
#define(f) {
    #scanp(f, int, 0) n;
    #scanp(f, int, 1) t;
    mirror n + t;          // an expression, so an automatic snapshot
    #delete[t];
    #delete[n];
}
f(&a, &b) r;               // r holds the value safely
```

The rule of thumb: if the body cleans up the very thing it returns, use `mirror &x;`.
If cleanup touches a different slot, plain `mirror x;` is fine. If there's nothing to
clean up, `return x;` is simplest.

`destruct` and the object-label mechanics are covered in [Objects](05-objects.md).

## `#redefine`, changing a body at runtime

```
#redefine(myFunc) { cssc::outln("overwritten"); }    // replace the whole body
#redefine(myFunc) +<0> { cssc::outln("prepended"); } // inject at position 0
```

Without a position it replaces the entire body. With `+<pos>` it injects code at
statement position `pos`.

`#redefine` is interpreter-only. The native compiler lowers function bodies
statically, so a runtime AST mutation has no effect there. For native builds, reach
for `if`/`select` branching or parameterize with `#cdefine`.

## The short version

- A callee `&param` hint does nothing. Reference versus copy is a call-site
  decision: `f(x)` versus `f(&x)`.
- `return` skips cleanup. If you have copy parameters or transient allocations to
  free, use `mirror` so the trailing `#delete`s still run.
- `mirror slot;` is a live reference, so deleting that slot afterward zeroes the
  caller's capture. Use `mirror &slot;` to snapshot.
- `#cdefine`, `#fvar`, `#param`, `#qvar`, and `#scanp_opt` want `#include('def')`.
  `#define`, `#redefine`, and `#scanp` are core.
- `#define` bodies can't see outer names. Import with `#req`.

## See also

- [Scopes & `#req`](03-scopes-and-req.md): the `#define` body is a wall, and `#req`
  imports across it.
- [Memory & Ownership](02-memory-and-ownership.md): call-site reference or copy, and
  the delete cascade.
- [Objects](05-objects.md): labels, `call`, `destruct`, and label overloading.
- [Directives](08-directives.md): the exact syntax for every `#…` form.
