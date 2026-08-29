# Control Flow

`if`, `for`, and `while` are what you'd expect. `select` isn't. It's a cursor loop
driven by explicit `jump` statements, and it's the one construct people misuse, so
most of this chapter is about it.

A quick scope reminder: `if`, `else`, `for`, `while`, and `select` bodies are
windows, not walls. They see the enclosing scope directly, so you don't need `#req`
inside them. Only `#define`, `{}`, object, label, and sector bodies are walls (see
[Scopes & `#req`](03-scopes-and-req.md)).

## `if`, `else if`, `else`

```
if (x > 10) {
    cssc::outln("big");
} else if (x > 5) {
    cssc::outln("medium");
} else {
    cssc::outln("small");
}
```

The condition is truthy or falsy in the usual way. Remember that `0`, `0x0`, and
`null` all compare equal (see [Types & Values](01-types-and-values.md)).

## `for`

There are three forms:

```
// C-style
for (int i = 0; i < 10; i = i + 1) {
    cssc::outln(i);
}

// for-in over values
for (val in myList) {
    cssc::outln(val);
}

// for-in with an index
for (i, val in myList) {
    cssc::outln(i + ": " + val);
}
```

The loop variable is local to the loop and released at the closing `}`. The body
still sees the enclosing scope, since it isn't a wall.

## `while`

```
while (running) {
    cssc::outln("tick");
}
```

## `break` and `continue`

`break;` exits the nearest `for` or `while` loop, and `continue;` skips to the next
iteration of it. There's one special case in object labels: a `break;` in a label
body but outside any loop acts as an early return from the label (see
[Objects](05-objects.md)).

## `select`, cursor iteration

`select` walks an iterable with an explicit cursor. Each pass binds the current
element to `?name`, and then you decide how far to move the cursor with a `jump`. If
you never `jump`, the cursor never advances, so the body runs at most once and the
loop ends. That's almost always a bug, and the analyzer flags it
(`SELECT_WITHOUT_JUMP`).

```
#stack[array<int>, 1024] bytecode = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0x0};

select (bytecode) ?i {
    if (i == 0x0) {          // sentinel, so stop
        cssc::outln("end");
        return;              // 'return' exits the whole function, not just select
    }
    if (i == 1) {
        jump++;              // skip the next element (cursor +2)
    }
    cssc::outln(i);
    jump;                    // advance to the next element (cursor +1)
}
```

The cursor controls:

| Statement | Cursor move |
|-----------|-------------|
| `jump;` | +1, the next element |
| `jump++;` | +2, skip one |
| `!jump;` | -1, back one |
| `!jump++;` | -2, back two |

If a path has no `jump`, the cursor doesn't move on that path, and when control
reaches the end of the body without a jump, the loop ends. The loop also ends when
the cursor goes out of bounds. And `return;` inside a `select` exits the enclosing
function or label, not just the loop; to leave only the loop, arrange your jumps so
the cursor runs out, or use a sentinel like `0x0`.

The backward form is `!jump` (and `!jump++`). There's no `jump_back` keyword. `jump`
is the only cursor keyword; the leading `!` flips the direction and `++` doubles the
step.

### `?i` is a reference; copy it with `&` to keep it

The cursor `?i` is a borrowed reference into the container you're iterating. CSSC
binds by reference by default, and cursors are no exception. It's valid only during
the current pass: the next `jump` rebinds it to the next element, and when the
`select` exits it goes dead. Storing the bare cursor somewhere that outlives the loop
leaves a stale reference that reads `0x0`:

```
#heap[vector<int>, 1024] kept = [];
select (nums) ?i {
    kept.push(i);      // wrong: stores a borrowed reference, dead after the loop
    jump;
}
// kept[0] now reads 0x0; the cursor it pointed at is gone
```

Hand over a copy with `&` so the value outlives the cursor:

```
select (nums) ?i {
    kept.push(&i);     // right: &i is an independent copy that survives the loop
    jump;
}
```

This is the same rule as deleting a reference versus a copy
([Memory & Ownership](02-memory-and-ownership.md)): by default you hold the real
thing, not a copy, so anything you want to keep past the cursor's lifetime has to be
copied with `&`. The analyzer flags the bare-reference case as
`SELECT_ALIAS_BORROW_NO_COPY`, and it flags `#delete[?i]` or `#free[?i]` (freeing the
borrowed cursor) as `DELETE_SELECT_ALIAS`.

### `?label.pos()`, the current index

Inside the body, `?label.pos()` gives the current 0-based cursor position.

```
#stack[list, 256] n = [10, 20, 30];
select (n) ?i {
    cssc::out("pos=");
    cssc::out(i.pos());     // 0, 1, 2
    cssc::out(" val=");
    cssc::outln(i);
    jump;
}
```

One backend note: the native compiler currently lowers `select` only for
`vector<int>` and `array<bind>`. For embedded builds, prefer those container kinds;
other iterables (`array<float>`, `map`, `bind`) are interpreter-only for now. The
semantics are identical across backends where both support it.

## The short version

- A `select` with no `jump` runs once and stops. Every path that should keep
  iterating has to end in a `jump` (or `jump++`, `!jump`). The analyzer catches the
  missing-jump case as `SELECT_WITHOUT_JUMP`.
- `return;` in a `select` exits the function, not just the loop. Use a sentinel or
  let the cursor run out to end only the loop.
- `if`, `for`, `while`, and `select` are windows, not walls. They read and write the
  enclosing scope, so don't wrap outer names in `#req` inside them.
- Backward jump is `!jump`, not `jump_back`.

## See also

- [Scopes & `#req`](03-scopes-and-req.md) for why these blocks are transparent.
- [Types & Values](01-types-and-values.md) for the `0x0`/`null` truthiness used in
  conditions.
- [Objects](05-objects.md) for `break` as a label early-return.
