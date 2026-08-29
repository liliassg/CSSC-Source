# Sectors

A `sector` is CSSC's named scope with real, enforced privacy. Unlike objects, which
have no access control, a sector actually gates `private:` members: read one from
outside through `::` and you get `0x0`. Sectors are also isolated. A sector body runs
in its own variable space and can't see top-level globals unless you inject them.

## Structure

```
sector Engine {
private:
    #stack[int, 32] Engine->fps = 60;
public:
    #stack[string, 128] Engine->title = "MyEngine";
    #define(Engine->start) {
        cssc::outln("Engine starting");
    }
} free {
    #delete[Engine->fps];
    #delete[Engine->title];
};
```

The header grammar is:

```
sector NAME <injection-list>? ?reserveLabel? { private: … public: … } free { … }?
```

`private:` and `public:` split the members, and the default is `private` if you
write members before any section label. Inside the sector you declare and reach
members with `->` (`Engine->fps`); from outside you reach them with `::`
(`Engine::title`, `Engine::start()`). The `free { }` block is optional (see below),
though you should still write it to release your members.

## Public and private

External `Sector::member` access is checked. A `public:` member comes back with its
value; a `private:` member read comes back as `0x0`, not an error. But
`Sector->member` is not checked, so a private member is fully readable and writable
through `->`. That's the deliberate `::`/`->` asymmetry, covered in
[Access Operators](10-access-operators.md). A `Sector::member = v` write isn't
checked either, and in the interpreter it persists.

```
cssc::outln(Engine::title);   // public, so the value
cssc::outln(Engine::fps);     // private through ::, so 0x0
cssc::outln(Engine->fps);     // private through ->, so 60, not gated
```

One note if you've read older material: an external `Sector::member = v` write does
persist in the interpreter. Some docs describe it as a silent no-op, but that's the
native compiler's model, not the interpreter's.

## Isolation

A sector body runs in its own variable space, so:

- Top-level globals aren't visible inside the sector unless you inject them.
- An unknown bare identifier inside a sector resolves to `0x0`. There's no silent
  fallback to a same-named global.
- Inside a sector method, a `ns::func(...)` call is only allowed when `ns` is `cssc`,
  the sector itself, one of its members, or an injected dependency. Anything else is
  a `Sector-Isolation` error.

```
#stack[int, 32] globalX = 99;

sector Iso {
public:
    #define(Iso->show) {
        cssc::outln(globalX);   // 0x0, because globalX isn't injected
    }
} free {};
```

To use `globalX`, inject it, which is the next section.

## Dependency injection

The `<…>` header list injects outer variables into the sector's scope. Each entry is
one of these:

| Form | Meaning |
|------|---------|
| `<name>` | inject outer variable `name` by reference (zero-copy, shared) |
| `<&name>` | inject `name` as a deep-copy snapshot |
| `<outerVar: localName>` | inject `outerVar` under the local name `localName` (reference by default; `&`/`*` modifiers allowed on either side) |
| `<*name>` | a deprecated reference spelling, the same as bare `<name>` |

```
#stack[int, 32] globalConfig = 42;

sector App<globalConfig> {          // injected by reference, shared
    #define(App->run) {
        cssc::outln(globalConfig);  // 42
    }
} free {};

sector Snapshot<&globalConfig> { … } free {};    // an independent copy
sector Aliased<globalConfig: cfg> { … } free {}; // injected under the name 'cfg'
```

One thing to be clear about: for a sector, `<A: B>` means "inject outer variable `A`
under the local name `B`." It's not a typed constructor parameter. Typed constructor
parameters like `<int: width>` are an object feature, not a sector one. See
[Objects](05-objects.md).

## Deferred construction

A sector marked with `?label` in its header isn't built where it's defined. It's set
aside and constructed later by `#reserve[label]`.

```
sector Config ?app {
    #define(Config->run) { cssc::outln("running"); }
} free {};

#reserve[app];      // now the sector is constructed
app::run();
#free[app];
```

Use this when a sector needs to be built at a specific moment, after some setup,
rather than where it textually appears.

## Teardown

`#free[Sector]` runs the sector's `free { }` block and drops it. `#unload[alias]` is
the equivalent for a loaded module, and it cascades to that module's child sectors
(see [Modules](07-modules.md)).

```
#free[Engine];
```

The `free { }` block is optional and unenforced. The parser accepts a sector with no
`free` block, an empty block does nothing, and nothing errors if you never `#free` a
sector, since there's no leak check. So "mandatory" is good practice, not a rule.
Write and call them anyway, because it matters for compiled builds.

## Nested and self-referencing sectors

An inner sector can inject its enclosing sector to reach it:

```
sector Outer {
    sector Inner<Outer> {          // Inner captures Outer by reference
        // Inner can reach Outer's injected members
    } free {};
} free {};
```

During construction a live placeholder for the parent shares the same variable
space, so an inner sector sees the parent as a real reference. (`<*Outer>` is the
deprecated spelling.)

## The short version

- Sectors are the real privacy tool. `::` reads enforce `private:`; objects enforce
  nothing.
- `->` bypasses sector privacy for both reads and writes. Only `::` reads are gated.
- `Sector::member = v` persists in the interpreter; it's not a no-op.
- `<A: B>` on a sector injects a variable, not a typed parameter. Typed constructor
  parameters are objects-only.
- There's no global fallback inside a sector. Inject with `<…>` or the name is `0x0`.
- `free` and `#free` are optional and unenforced, with no leak error, but you should
  still use them.

## See also

- [Access Operators](10-access-operators.md) for the `::`/`->`/`.` rules and the
  asymmetry.
- [Scopes & `#req`](03-scopes-and-req.md) for the sector body as a wall.
- [Modules](07-modules.md) for modules versus sectors, `::` dispatch, and
  `#free`/`#unload`.
- [Objects](05-objects.md) for objects, which have typed constructor parameters and
  no privacy.
