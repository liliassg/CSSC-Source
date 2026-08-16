# IncludeCPP Changelog

## v5.0.0 (2026-03-02)

Major version bump consolidating all v4.9.x improvements.

### New Builtin: `is()` - Smart Truthiness / Equality Check

```cssl
// Truthiness (single arg)
is(true)       // True        is(false)    // False
is(1)          // True        is(0)        // False
is("hello")    // True        is("")       // False
is([1, 2])     // True        is([])       // False
is(null)       // False

// Equality (two positional args)
is(a, b)          // a == b
is(5, 5)          // True
is("hi", "hi")    // True

// Custom whitelists
is(x, true=["yes", "on", 1])       // in list -> True, else False
is(x, false=["no", "off", 0])      // in list -> False, else True
is(x, true=["yes"], false=["no"])  // true first, false second, else default
is(x, true="yes")                  // single value accepted (no list needed)
```

### New Builtin: `iterate()` - Fluent Collection Builder

`iterate()` is a first-class builtin (no `include` needed).

```cssl
// Range
vector<int> numbers = iterate(0, 100);
vector<int> evens   = iterate(0, 100, 2);
vector<int> numbers = iterate(min(0), max(100));  // same as iterate(0, 100)

// Wrap any value
string text    = iterate(allnames["max"]["intro"]);
vector<string> = iterate("hello");                 // char list

// Named params
vector<string> ch  = iterate(chars="hello");
vector<string> ks  = iterate(keys=myDict);
vector<dynamic> vs = iterate(values=myDict);
vector<string> rep = iterate(repeat=3, value="hi");
vector<float>  pts = iterate(from_=0.0, to=1.0, count=5);

// Fluent chain
vector<int> result = iterate(1, 50).filter("x % 2 == 0").map("x * x").limit(5);
string upper       = iterate("hello").map("x.upper()").join();
vector<int> top    = iterate(myList).sort(reverse=true).unique().limit(10);
```

**IterChain methods:** `.filter` `.map` `.sort` `.unique` `.reverse` `.limit` `.skip`
`.chunk` `.zip_with` `.with_index` `.flatten` `.window` `.interleave` `.group_by`
`.join` `.sum` `.avg` `.min` `.max` `.count` `.first` `.last` `.contains` `.find`
`.count_where` `.all` `.any` `.index_of` `.to_list` `.to_dict`

### New Builtin Include: `cssl-dtalib` - Data Visualization

```cssl
dta = include("cssl-dtalib");
analys = new dta::CsslDataAnalysis(mode=dta::Pixel, w_h=(900,600));
analys.add(container1);
analys.show();
```

Modes: `dta::Pixel` `dta::Bar` `dta::Line` `dta::Round`
Frameless PyQt6 window, dark theme, scroll-wheel zoom, hover tooltips.

### New Builtin Include: `cssl-sharing` - Inter-Runtime Communication

```cssl
auto cshare = include("cssl-sharing");
cshare = new CsslSharing(channel="game_sync", portID=4000);
cshare.open();
cshare.connect(port=4001);
cshare.interact(port=4001, FUNC);
result = cshare.find(port=4001, type="variable", name="score");
cshare.merge(port=4001);
```

### Bug Fix: `for (condition) {}` - Condition-Style For Loop

`for` now supports three syntaxes:
- `for (init; cond; update) {}` - C-style
- `for (var in range(...)) {}` - Python-style
- `for (condition) {}` - while alias (new)

```cssl
for (not CShare::exists(port=3000)) {
    print("Waiting...");
    sleep(2);
}
```

### Bug Fix: `print()` / `printl()` Output Buffering

Added `flush=True` to all `print()` calls in `CSSLRuntime.output()`, `builtin_print()`, and `builtin_println()`. Output now appears immediately in long-running loops without needing a newline.

### Bug Fix: `cssl-sharing` Bridge Runtime Access

`CsslLang.Sharing()` now stores `_bridge = self` (the `CsslLang` instance).
Operations use `_bridge.run()`, `_bridge.set_global()`, `_bridge._get_runtime()` — no direct internal runtime calls.

### Bug Fix: Container Assignment from Any Iterable

`vector<int> x = anyIterable` now accepts any sequence-like object (duck-typed: has `__iter__` + `__len__`, not str/dict) in `_exec_typed_declaration`. Previously any non-list/tuple was appended as a single element.

### Bug Fix: `string x = iterate("hello")` Auto-Join

IterChain of single chars auto-joins to a string on `string`-typed assignment.

### Bug Fix: E002 False Positives for Collection Literals

`vector<int> x = {1, 2, 3}` no longer raises LSP E002. `_types_compatible()` accepts list literals for all list-like types.

### Bug Fix: `execute()` Multi-Statement Execution

`execute("a = 1; b = 2;")` now correctly runs all statements. Was checking non-existent `_return_triggered` flag; fixed to catch `CSSLReturn` exception.

---

## v4.9.x History (consolidated into v5.0.0)

See git log for individual patch releases v4.9.13 through v4.9.19.
