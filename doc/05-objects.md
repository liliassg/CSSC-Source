# Objects

An `object` bundles data members, which you reach with `->`, and labels, which are
named sections of code you call with `.`. Objects are instantiated, can be cloned,
support label overloading, and carry a `free { }` teardown block.

The two access operators aren't interchangeable, and [Access Operators](10-access-operators.md)
has the full rules. The short version: `obj->member` reads or writes a data member,
and `inst.label(args)` calls a label.

## Structure

```
object Player {
    #auto[int] Player->hp = 100;    // a data member, declared with ->
    Player->init();                 // top-level object code runs at instantiation

init:                               // a label, a named section of code
    cssc::outln("Player created");

takeDamage<int: dmg>:               // a label with a typed parameter
    Player->hp = Player->hp - dmg;
    if (Player->hp <= 0) {
        call die;
    }

die:
    cssc::outln("Player died");
    destruct;                       // run free {}, mark dead, return to the caller
} free {
    #delete[Player->hp];            // teardown
};
```

Data members are declared and reached as `Player->member`. Labels are `name:`
sections, and the object's top-level statements, the ones outside any label, run
once at instantiation. The `free { }` block runs on `#free[instance]` or on
`destruct`; it's where you release what the object allocated, and it's good practice
to release every member you own there. The object body is a wall (see
[Scopes & `#req`](03-scopes-and-req.md)); the members are the object's own scope.

## Instantiating and calling labels

```
Player() myPlayer;          // run the object's top-level code and bind the instance
myPlayer.takeDamage(30);    // call a label with '.'
```

`Type() name;` instantiates, with constructor arguments in the parentheses (below).
`instance.label(args)` calls a label. Labels are the object's callable surface.

## Data members with `->`

Member access uses `->`, and here's the important part: `->` does no access check. A
member reached with `->` is always reachable, even from outside the object, even if
you thought of it as private.

```
Player() p;
cssc::outln(p->hp);     // works from outside; -> isn't gated
```

That's not an accident. Objects have no access control at all, so every member (via
`->`) and every label (via `.`) is always reachable. Access control is a sector
feature: sector members reached with `::` are gated by `public:` and `private:`. So
`::` on a sector enforces privacy and `->` never does. More on that below, and in
[Access Operators](10-access-operators.md).

## Constructor parameters and capture

The `<…>` header on an object is its capture and injection list. Each entry is one
of these:

| Form | Meaning |
|------|---------|
| `<type: name>` | a constructor parameter, typed, supplied at instantiation |
| `<name>` | a zero-copy reference captured from the enclosing scope (the default) |
| `<&name>` | a deep copy captured from the enclosing scope |
| `<*name>` | a deprecated reference spelling, the same as bare `<name>` |

Constructor parameters:

```
object Widget<int: width, int: height> {
    #stack[int, 32] Widget->w = width;
    #stack[int, 32] Widget->h = height;
} free {
    #delete[Widget->w];
    #delete[Widget->h];
};

Widget(800, 600) myWidget;
```

Capturing an outer variable is how CSSC does composition without a class-inheritance
keyword. Inject an outer object or sector into an object's header and it becomes a
member, live or copied:

```
#stack[int, 32] globalConfig = 42;

object App<globalConfig> {          // captured by reference, the default
    // globalConfig is available here as a zero-copy reference
} free {};

object Snapshot<&array<int>: buf> { // captured as an independent deep copy
    // changing the original after construction isn't visible here
} free {};
```

The old `object Foo<*a, *b>` reference spelling is deprecated but still accepted.
Write `object Foo<a, b>` for references, or `object Foo<auto: a, auto: b>` for
constructor parameters. Bare capture is reference-by-default, matching `#req` and
argument passing.

## Labels: parameters, transfers, overloading

```
object Handler {
    process<string: data>:
        cssc::outln("String: " + data);
    process<int: data>:                 // same name, a different parameter type
        cssc::outln("Int: " + data);
} free {};

Handler() h;
h.process("hello");   // "String: hello"
h.process(42);        // "Int: 42"
```

A simple label is `name:`. Typed parameters go in the header, `name<int: x, string:
msg>:`. A transfer capture, `name<outerVar>:`, pulls an outer variable zero-copy into
the label (the default; `<*outerVar>` is the deprecated spelling). And labels
overload: several can share a name with different parameter lists, and CSSC picks
one at runtime by argument type, with an exact type match winning.

## `call`, `mirror`, `return`, `destruct`

Inside an object, labels call each other with `call` and hand back values with
`mirror` or `return`, the same as in [Callables](04-callables.md).

```
object Calculator {
    add<int: a, int: b>:
        mirror a + b;       // return the value but keep running for cleanup

    snapshot<data>:
        mirror &data;       // an explicit deep-copy return

    shutdown:
        destruct;           // run free {}, mark dead; the host keeps running
} free {};

Calculator() calc;
call add<3, 4> result;      // call a label with transfer args and capture, giving 7
```

`call label<args> capture;` calls a label and binds its result. `call label<*name,
42>;` uses the `*name` transfer form to force an explicit transfer reference instead
of the automatic scope pull. (Here `*` is a transfer override, not the deprecated
reference marker.) `destruct;` runs `free { }`, marks the instance dead, and returns
to the caller. It's not `exit()`, so the host script keeps going. A `break;` inside a
label but outside any loop acts as an early return from the label and doesn't
destroy the object; inside a `for` or `while` it's the normal loop break.

The full `mirror` reference-versus-snapshot rules are in [Callables](04-callables.md).

## Objects have no privacy: use a sector

Objects don't have `public` or `private`. Access control is a sector feature, and
this is by design. Every object member (via `->`) and every label (via `.`) is
reachable from anywhere, and there's no modifier that turns privacy on.

When you need enforced privacy, put the state in a sector. `::` access honors
`public:` and `private:`, and a private read returns `0x0`:

```
sector Config {
private:
    secret = "internal only";
public:
    show:
        cssc::outln(Config::secret);   // an internal read, allowed
} free {};

cssc::outln(Config::secret);   // an external read of a private member, giving 0x0
```

If you've seen a `secure !` object modifier or object `private:`/`public:` sections
in older material, they don't do anything on an object. Privacy lives in sectors.
See [Sectors](06-sectors.md).

## The short version

- Objects have no privacy. Every member (`->`) and label (`.`) is reachable. `::` on
  a sector is what enforces `public:`/`private:`.
- `.` calls a label; `->` reads or writes a data member. They aren't interchangeable
  (see [Access Operators](10-access-operators.md)).
- Put teardown in `free { }` and release every member you allocated.
- `destruct` isn't exit. It tears down the instance and returns to the host.
- Overloading resolves at runtime by argument type, and an exact match wins.
- Captures are reference-by-default: `<name>` is a live reference, `<&name>`
  deep-copies, `<*name>` is deprecated.

## See also

- [Access Operators](10-access-operators.md) for the `::` versus `->` versus `.`
  rules.
- [Sectors](06-sectors.md), where `::` privacy is enforced.
- [Callables](04-callables.md) for `mirror`/`return`/`destruct` and capture syntax.
- [Memory & Ownership](02-memory-and-ownership.md) for `#free` teardown and the
  member cascade.
