# Types & Values

CSSC is strict about types when you declare something, raw about them at runtime,
and predictable about sizes. This chapter is the reference for what the primitive
types are, how big they are, what `null` means, and how the containers behave.

## Primitive types

There are four primitives, plus the untyped escape hatch.

| Type | What it is | Size |
|------|------------|------|
| `int` | a signed integer, arbitrary precision | as many bytes as the value needs (minimal signed little-endian) |
| `float` | an IEEE-754 double | always 64-bit, 8 bytes |
| `string` | UTF-8 text | the byte length of the encoding |
| `bool` | true or false | 1 byte |
| `auto` / `var` / `void` | untyped | whatever the initializer produced; `null` if none |

```
#stack[int, 32]    n = 42;      // also 0xFF, 0b1010
#stack[float, 64]  f = 3.14;    // must be at least 64 bits, see below
#stack[string, 128] s = "hi";   // double or 'single' quotes
#stack[bool, 8]    b = true;    // true / false
```

## Floats are always 64-bit

A float always encodes to 8 bytes. If you write `#stack[float, N]` with `N` below
64, CSSC doesn't quietly round it up. It rejects the declaration with an overflow
error, even for a plain `0.0`.

```
#stack[float, 64] ok  = 3.14;   // fine, this is the minimum
#stack[float, 32] bad = 1.0;    // error: 64 bits won't fit in 32
```

I did this on purpose. A silent float32-to-float64 mismatch is exactly the kind of
bug that costs you an afternoon, so the language refuses it instead of hiding it.
The smallest valid float slot is `#stack[float, 64]`.

Watch out for the `sizes` module here: `sizes::small_float` is 32, so using it for
a float slot triggers the same rejection. Use `sizes::normal_float` (64) or larger.

## Only a few names are real types

`int`, `float`, `string`, `bool`, the containers, and `auto`/`var`/`void` are the
real types. Anything else you might type out of habit, like `char`, `byte`, `i32`,
`i64`, `f32`, `f64`, or `double`, is not one of them. CSSC accepts the name but
treats the slot as untyped:

- no coercion, so the value keeps whatever type its initializer gave it,
- no default, so it starts as `null`,
- no type checking, so it accepts anything.

```
#stack[i32, 32] x = 5;    // 'i32' is not int; x is just an untyped slot holding 5
#stack[char, 8] c = "A";  // 'char' is not a type; c is just the string "A"
```

If you reach for `i32` or `char` expecting C semantics, you'll get an untyped slot
instead of a sized integer or a character type. Use `int`, and control the width
with the bit capacity.

## `null` and `0x0`

They're the same thing. `null` (and its alias `none`) is the null value, `0x0` is
the integer zero, and CSSC treats `null`, `0x0`, and `0` as equal under `==` and
`!=`. A deleted or dangling slot reads back as `0x0`.

```
if (maybe == 0x0)  { /* maybe is null, unbound, or zero */ }
if (maybe == null) { /* the exact same test */ }
```

`0x0` is the idiomatic "is this here?" check. You'll also see it with `#adress`,
covered in [Memory & Ownership](02-memory-and-ownership.md).

## Literals

| Kind | How you write it |
|------|------------------|
| int | `42`, `0xFF`, `0b1010` |
| float | `3.14`, `2.0` |
| string | `"hello"`, `'world'` |
| bool | `true`, `false` |
| null | `null`, `none`, `0x0` |
| array | `{1, 2, 3}` or `[1, 2, 3]` |
| map | `{}`, `{key: value}`, `{key = value}` |
| bind | `{a, b; c, d}` (note the `;`) |

## Capacities and the `sizes` module

The number after the type in `#stack[type, bits]` is a capacity in bits. When you
leave it off, you get a default:

| Region | Default |
|--------|---------|
| `#stack[type]` | 256 bits |
| `#heap[type]` | 1024 bits |
| `#auto[type]` | at least 32 bytes (256 bits), grows as needed |

For containers, that second number is an element capacity, not a scalar bit limit.

Rather than sprinkle raw bit counts through your code, you can use the `sizes`
module, which gives them readable names:

```
#include("sizes") sz;
#stack[string, sz::normal_string] name = "Ada";   // 256 bits
#stack[int, sz::large_int] big = 0;                // 64 bits
```

The full set, in bits:

| group | `small_` | `normal_` | `large_` |
|-------|----------|-----------|----------|
| `int` | 16 | 32 | 64 |
| `float` | 32 | 64 | 128 |
| `string` | 128 | 256 | 1024 |
| `bool` | 8 | 8 | 8 |
| `array` / `vector` / `list` | 256 | 1024 | 4096 |
| `map` / `dict` | 512 | 2048 | 8192 |
| `auto` / `var` | 64 | 256 | 1024 |

## Containers

CSSC has four container shapes. One thing to get straight early: the literal you
write and the runtime type you get aren't always the same class you'd expect.

| You write | It's a | At runtime |
|-----------|--------|------------|
| `{1, 2, 3}` (no `;`, no `:`/`=`) | array literal | a plain list |
| `[1, 2, 3]` | array literal | a plain list |
| `{k: v}` or `{k = v}` or `{}` | map literal | a plain dict |
| `{a, b; c, d}` (has a `;`) | bind literal | a bind |

Both `{…}` and `[…]` give you a plain list. The brace-versus-bracket choice doesn't
change the type. The richer container classes, with their fuller method sets, show
up when a value lands in a typed slot (`array<T>`, `vector<T>`, `map<K,V>`), goes
through coercion, or is built by the `#array`/`#vector`/`#map` directives. So it's
the slot type that decides what you get, not the bracket.

The methods you have follow from that:

- `array<T>`: `push_back`, `pop_back`, `push_front`, `pop_front`, `size`, `length`,
  `at`, `get`, `set`, `contains`, `indexOf`, `insert`, `erase`, `slice`, `join`,
  `sort`, `reverse`, `unique`, `first`, `last`, `front`, `back`, `clear`, `resize`,
  `capacity`, and more. A typed `array<T>` adds `map`, `filter`, `reduce`, `sortBy`,
  and friends.
- `vector<T>`, which is the same as `list`: the full STL-flavored surface, including
  `push_back`, `pop_back`, `front`, `back`, `at`, `insert_at`, `erase`, `resize`,
  `reserve`, `capacity`, `map`, `filter`, `reduce`, `slice`, and `sort_inplace`.
- `map<K,V>`: `get`, `set`, `has`, `contains`, `keys`, `values`, `items`, `remove`,
  `size`, `length`, `clear`, `merge`, `update`, `at`. A typed `map<K,V>` adds
  `emplace`, `get_or_default`, `lower_bound`, and so on.

How assignment works with containers (they alias, scalars copy) is in
[Memory & Ownership](02-memory-and-ownership.md).

## `bind`, the structured key/cell type

A `bind` holds a flat list of cells plus a `pair_width` that says how many cells
make one row. That lets you read the same data flat with `b[i]` or two-dimensional
with `b[r][c]`.

- A structured literal `{a, b; c, d}` sets `pair_width` to the number of cells in
  the first row: `{a, b; c, d}` gives width 2, `{a, b, c; d, e, f}` gives width 3.
  Every row has to have the same cell count.
- A flat `{a, b, c}` is an array literal, not a bind. It only becomes a bind when
  you assign or coerce it into a bind slot, and that coercion pairs adjacent cells,
  giving `pair_width` 2. A leftover unpaired cell becomes `(cell, null)`.
- `pair_width` is 0 only for an empty or default bind.

Access and size:

- `b[i]` reads the i-th cell, flat.
- `b[r][c]` is the structured read, equal to `b[r * pair_width + c]`.
- `b.size()` and `b.length()` count rows, not cells.
- `b.addmap(m)` appends a map's, pair's, or list's entries as rows. The list
  methods (`push_back`, `pop_back`, `at`, and the rest) work too, since a bind is a
  list underneath.

```
#heap[bind, 328] frame = {yPos, text; durationMs, 0x0};  // pair_width = 2
cssc::outln(frame[0]);       // yPos        (flat cell 0)
cssc::outln(frame[2]);       // durationMs  (flat cell 2)
cssc::outln(frame[1][0]);    // durationMs  (row 1, col 0)
cssc::outln(frame.size());   // 2           (rows, not cells)
```

## Strings

`string` is UTF-8, and it has a big method surface:

`length`, `size`, `append(x)`, `upper`, `lower`, `trim`, `split(sep=' ')`,
`replace(old, new)`, `contains`, `startsWith`, `endsWith`, `indexOf`, `charAt`,
`substr(start, len=-1)`, `substring(start, end=-1)`, `reverse`, `repeat`,
`padStart(n, ch)`, `padEnd(n, ch)`, `isEmpty`, `isDigit`, `isAlpha`, `toInt`,
`toFloat`, `front`, `back`, `data`, `capacity`, `exists`.

Indexing and single-character writes:

```
#stack[string, 128] s = "Hello";
cssc::outln(s[1]);        // "e", a one-character string, not a code point
s[0] = "h";               // in-place single-char write, now "hello"
cssc::outln(s.length());  // 5
```

- `s[i]` gives you a one-character string. Out of range gives `""`.
- `s[i] = "x"` rebuilds the string (strings are immutable underneath). Writing past
  the end zero-pads with `\x00`. The target has to be a named variable.
- `indexOf` returns `-1` when it finds nothing; `toInt` and `toFloat` return `0`
  and `0.0` on input that isn't a number.

### The in-place string methods

Some of the whole-string transforms behave differently depending on where you use
them, and this catches people, so it's worth being clear.

Used as a statement, `s.reverse();` or `s.append(x);` rewrites `s` in place. The
method changes the variable's own slot. This is how you grow or transform a string,
because `str += x` re-evaluates a heap literal in expression position, which the
analyzer rejects as E020 inside a `select` or another barrier.

Used as an expression, `x = s.upper()` or `outln(s.replace(a, b))` is a pure
transform. It returns the new string and leaves `s` alone. So chaining or repeating
the expression form is safe: two `s.replace(…)` calls in a row both work on the
original `s`.

The methods that behave this way are `append(x)`, `reverse`, `upper`, `lower`,
`trim`, and `replace(old, new)`. The interpreter adds a few more (`repeat`,
`padStart`, `padEnd`, `capitalize`, `title`); the native compiler covers the first
six.

```
#stack[string, 64] path = "myproject";
path.reverse();                 // statement: path is now "tcejorpym"
#stack[string, 16] slash = "/";
path.append(slash);             // statement: path is now "tcejorpym/"
#delete[slash];                 // the argument is only read, so free it yourself
path.upper();                   // statement: path is now "TCEJORPYM/"
cssc::outln(path);

#stack[string, 64] name = "Ada";
#auto[string] shout = name.upper();   // expression: shout is "ADA", name stays "Ada"
cssc::outln(name);                    // "Ada", unchanged
```

A couple of details:

- `append(x)` reads `x`'s bytes and tacks them onto the receiver. It borrows `x`,
  it doesn't consume it, so you still `#delete` `x` yourself.
- The in-place (statement) form only works when the receiver is a named string
  variable, because there has to be a slot to write back into. On an rvalue like
  `(a + b).reverse();` the transform runs but the result goes nowhere.
- The methods that read or extract a different value (`length`, `size`, `at`,
  `indexOf`, `contains`, `charAt`, `startsWith`, `substr`, `split`, `toInt`) never
  change the receiver, wherever you use them.

## Introspection: `declared()` and `cssc::typeof`

Two things work on a variable of any type.

`x.declared()` gives you an owned string holding the variable's source-code name,
the identifier exactly as you wrote it. The name is resolved at compile time, so it
works for everything: scalars, strings, containers, objects, sectors. A receiver
that isn't an identifier gives `""`. You own the returned string, so `#delete` it.

```
#stack[int, 32] local = 5;
cssc::outln(local.declared());     // "local"

local.declared() nm;               // capture the owned name
cssc::outln(nm);                   // "local"
#delete[nm];
#delete[local];
```

One catch: `declared()` gives you the name of the receiver expression, not of
whatever produced its value. Inside `select (arr) ?el { … el.declared() … }` the
receiver is the cursor, so you get `"el"`, never the name of the element's original
variable. Once a value is inside a container, its source name is gone.

`cssc::typeof(x)` gives you a lowercase string naming the runtime type: `"int"`,
`"float"`, `"string"`, `"bool"`, `"array"`, `"map"`, `"bind"`, `"object"`,
`"sector"`, and so on. It's handy for `auto` parameters that accept more than one
shape. To compare two kinds, compare their `typeof`:

```
#define(dump) {
    #scanp(dump, auto, 0) value;
    #stack[string, 8] probe;                       // a known string to compare against
    if (cssc::typeof(value) == cssc::typeof(probe)) {
        cssc::outln("got a string: ", value);
    } else if (cssc::typeof(value) == "array") {
        cssc::outln("got an array of ", value.size());
    }
    #delete[probe];
}
```

## The short version

- A float under 64 bits is an error, not a rounding. The minimum is `#stack[float, 64]`.
- `i32`, `char`, `byte`, and `double` aren't real types; they become untyped slots.
  Use `int` or `float` with a bit capacity.
- `0x0`, `null`, and `0` all compare equal. `0x0` is the standard "is it there?"
  guard.
- `{…}` and `[…]` are both lists at runtime. The typed slot decides whether you get
  the richer container class.
- A flat bind has `pair_width` 2, and a flat `{a, b, c}` is an array until you
  coerce it into a bind slot.
- `s[i]` is a one-character string, not an integer character code.

## See also

- [Memory & Ownership](02-memory-and-ownership.md) for capacities, container
  aliasing, and the `0x0` guard.
- [Directives](08-directives.md) for the `#string`, `#int`, `#array`, `#vector`,
  and `#map` typed-declaration directives.
- [Modules](07-modules.md) for `#include("sizes")` and the rest.
