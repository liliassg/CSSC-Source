# Access Operators: `::`, `->`, and `.`

This is the thing that trips people up more than any other. The three access
operators aren't interchangeable, and they each have exactly one job.

If you remember one line: `::` is sector or module access with privacy enforced,
`->` is a data member on objects and sectors with privacy not enforced, and `.` is an
object label or method call.

## The decision table

| Operator | Use it for | Access-controlled? | A private member | A missing member |
|----------|------------|--------------------|------------------|------------------|
| `::` | a sector or module member: `alias::member`, `alias::func(args)` | yes, on reads | a read gives `0x0`; a `::` write bypasses the check and persists | sector gives `null`; a built-in-module method call errors; a module property read gives `null` |
| `->` | an object or sector data member: `obj->field`, `Sector->field` | no | fully readable and writable | a read gives `null` |
| `.` | an object label or method call: `instance.label(args)` | no (objects have no privacy) | reachable and callable | a method call errors; a property read (no parens) gives `null` |

## `::`, sector and module access

Use `::` to reach a public member of a sector, or a member of a loaded module.

```
App::run();                 // call a public sector function
cssc::outln(App::title);    // read a public sector member
mh::mathlib::square(7);     // reach into a #depend'd .obj (nested sectors/modules)
```

`::` enforces public and private on reads: reading a `private:` sector member through
`::` gives `0x0`, not an error.

Here's the asymmetry to keep in mind, though: a `::` write to a sector member is not
checked, and in the interpreter it persists. `App::secret = 5;` actually stores. Only
the read side of `::` is gated.

Missing-member behavior depends on the target, which is the module-versus-sector
asymmetry from [Modules](07-modules.md): a missing or private sector member gives
`null`; a missing method call on a built-in module errors with `"Module 'mod' has no
function 'nope'"`; and a missing property read on a module gives `null`.

## `->`, data members, not access-checked

Use `->` to read or write a data member of an object or a sector.

```
object Player {
    #auto[int] Player->hp = 100;    // declare a member with ->
} free {};

Player() p;
cssc::outln(p->hp);     // read a member
p->hp = 50;             // write a member
```

`->` does no access check. That's the deliberate asymmetry with `::`: a sector's
`private:` member is hidden from a `::` read but is fully reachable, and writable,
through `->`.

```
sector Vault {
private:
    #stack[int, 32] Vault->secret = 42;
public:
    #stack[int, 32] Vault->label = 1;
} free {};

cssc::outln(Vault::secret);   // 0x0, the :: read is gated
cssc::outln(Vault->secret);   // 42, -> is not gated, so the real value
Vault->secret = 0;            // allowed, the -> write isn't gated either
```

So don't assume `private:` protects a member from `->`. If you need enforced privacy,
keep secrets reachable only through `::` and never expose a `->` path to them.

## `.`, object label and method call

Use `.` to call a label (a method) on an object instance.

```
Handler() h;
h.process("hello");     // call the 'process' label
h.process(42);          // overload resolved at runtime by argument type
```

With parentheses, `.` is a method call, and calling a label that doesn't exist raises
a runtime error (`"No method '…'"`). Without parentheses, it's a property read, and a
missing one gives `null`. Object-level access control (private labels, `secure !`)
isn't enforced, so every label is callable (see [Objects](05-objects.md)).

## Wrong and right

| Wrong | Right | Why |
|-------|-------|-----|
| `p.hp` to read a member | `p->hp` | `.` is for label calls; data members use `->`. |
| `p->takeDamage(30)` to call a label | `p.takeDamage(30)` | Labels and methods are called with `.`, not `->`. |
| `obj::field` on an object | `obj->field` | `::` is for sector and module scopes, not object instances. |
| relying on `Vault->secret` being blocked | read secrets only via `Vault::secret` | `->` is never access-checked; the `::` read is. |
| `Sector.func()` | `Sector::func()` | Sector and module calls use `::`. |
| expecting `Vault::secret` to error | it returns `0x0` | A private `::` read is a null, not an error. |

## The mental model

```
alias::member      ->  sector/module scope        (public/private enforced on read)
entity->member     ->  object/sector data field   (no access check, read and write)
instance.label()   ->  object label / method       (a call; overloaded by arg type)
```

## The short version

- They aren't interchangeable. `::`, `->`, and `.` each have one job.
- `->` ignores privacy. A private sector member is readable and writable through it.
  Only `::` reads are gated.
- A `::` write to a sector member persists; it's not a no-op.
- A missing member behaves differently by target: null for sectors and properties,
  but a hard error for a missing built-in-module method call or a missing object
  method call.
- Object privacy isn't enforced. Use a sector for real privacy.

## See also

- [Sectors](06-sectors.md) for `::` public/private enforcement and isolation.
- [Objects](05-objects.md) for `->` members and `.` labels.
- [Modules](07-modules.md) for module `::` dispatch and the missing-member asymmetry.
