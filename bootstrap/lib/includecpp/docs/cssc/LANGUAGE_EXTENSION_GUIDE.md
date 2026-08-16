# CSSC / CSSL — Sprach-Erweiterungs-Anleitung

Wie du eine neue **Direktive**, einen neuen **Builtin** oder ein neues
**Modul** sauber durch den ganzen Stack ziehst:
**Interpreter, nativer Compiler, Runtime und LSP**.

---

## 0. Architektur-Schnellueberblick

| Schicht | Datei | Was sie tut |
|---------|-------|-------------|
| Lexer + Parser | `includecpp/core/cssl/cssl_cssc.py` | Quelle → Tokens → AST. **Wird von beiden Pipelines geteilt.** |
| Interpreter | `includecpp/core/cssl/cssl_cssc.py` (zwei `CsscRuntime`-Klassen) | `cssc run` — AST direkt ausfuehren |
| Nativer Compiler | `includecpp/core/cssl/native/cssc_compiler.py` | `cssc build` — AST → C → gcc → .exe |
| Runtime-Library | `includecpp/core/cssl/native/cssc_runtime.[ch]` | Wird in jedes kompilierte .exe eingelinkt |
| LSP | `includecpp/vscode/cssl/server/analysis/diagnostic_provider.py` + `vscode/cssl/syntaxes/cssc.tmLanguage.json` | Editor-Diagnostics + Syntax-Highlight |

Wer eine neue Sprach-Funktion einbaut, fasst typischerweise **drei bis fuenf**
dieser Dateien an. Diese Anleitung zeigt das Schritt fuer Schritt.

---

## 1. Neuer Builtin: `cssc::foo(args)`

Beispiel: `cssc::detime()` (gibt aktuelle Zeit als float `HH.MM` zurueck).

### 1.1 Interpreter

Beide `CsscRuntime`-Klassen in `cssl_cssc.py` haben eine Builtin-Tabelle:

```python
'detime': lambda: (lambda t: t.tm_hour + t.tm_min / 100.0)(time.localtime()),
```

Die Tabelle taucht **zweimal** auf (zwei Runtime-Klassen, gleicher Code).
Beide eintragen.

### 1.2 Doc-Tabelle

Im selben File gibt es eine `BUILTIN_DOCS`-Map fuer die Hover-/`--help`-Anzeige:

```python
'detime': 'Current time as float HH.MM (e.g. 17.32 = 17h 32min)',
```

### 1.3 Native Runtime — Header

`includecpp/core/cssl/native/cssc_runtime.h`:

```c
CSSC_API CsscVal  cssc_builtin_detime(void);
```

### 1.4 Native Runtime — Implementation

`cssc_runtime.c`:

```c
CSSC_API CsscVal cssc_builtin_detime(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    return cssc_float((double)tm->tm_hour + (double)tm->tm_min / 100.0);
}
```

### 1.5 Compiler-Mapping

`native/cssc_compiler.py` — die Tabelle in `_gen_ns_call`:

```python
'detime': ('cssc_builtin_detime', 0),    # 0 = nargs
```

### 1.6 Test

```cssc
cssc::outln(cssc::detime());
```

Im Interpreter UND nach `cssc build` muss die Ausgabe identisch sein.

---

## 2. Neue Direktive: `#myDir[arg] var;`

Beispiel: `#console[w, h] name;` als Vorlage.

### 2.1 Parser-Branch

In `cssl_cssc.py`, `_parse_directive`:

```python
if dname == 'myDir':
    parts = [p.strip() for p in info['arg'].split(',')]
    self._match(TK_SEMICOLON)
    return {
        'type': 'directive',
        'directive': 'myDir',
        'param0': parts[0] if len(parts) > 0 else '',
        'var': info['var'],
        'line': tok.line,
    }
```

### 2.2 Interpreter-Handler

In **beiden** `_exec_node`-Dispatchern:

```python
elif directive == 'myDir':
    arg0 = node.get('param0', '')
    self._variables[node.get('var')] = my_python_construct(arg0)
```

### 2.3 Native Codegen

In `native/cssc_compiler.py`, `_gen_node` Direktiven-Branch:

```python
elif directive == 'myDir':
    arg0 = node.get('param0', '')
    var = node.get('var', '')
    self._store_allocated(var, f'cssc_my_dir_create({arg0})')
```

### 2.4 LSP

Diagnostic-Provider — Direktive zur Whitelist:

```python
_CSSC_BUILTIN_DIRECTIVES = {
    ...,
    'myDir',
}
```

TextMate-Grammar fuer Highlighting (`vscode/cssl/syntaxes/cssc.tmLanguage.json`):

```json
{
    "name": "meta.directive.myDir.cssc",
    "match": "(#)(myDir)([\\[\\(])([^\\]\\)]*)([\\]\\)])\\s*([a-zA-Z_][a-zA-Z0-9_]*)",
    "captures": {
        "1": { "name": "punctuation.definition.directive.cssc" },
        "2": { "name": "keyword.control.cssc" },
        ...
    }
}
```

---

## 3. Neues Modul: `#include('mymod') alias;`

Beispiel: `cssc.peek` als Vorlage.

### 3.1 Modul-Klasse anlegen

Neue Datei `includecpp/core/cssl/cssl_mymod.py`:

```python
class CsscMymodModule:
    _name = 'mymod'

    def get_namespace(self):
        return {
            'doStuff': self._do_stuff,
            'count':   self._count,
        }

    def get_docs(self):
        return {
            'doStuff(x)': 'Does the thing with x',
            'count()':    'Returns an int',
        }

    def _do_stuff(self, x):
        return x * 2

    def _count(self):
        return 42
```

### 3.2 In den Module-Dispatcher haengen

In `cssl_cssc.py` gibt es **zwei** `_load_module`-Methoden:

```python
if module_name == 'mymod':
    from .cssl_mymod import CsscMymodModule
    return CsscMymodModule()
```

(Beim zweiten Dispatcher: gleicher Eintrag in der `modules`-Tabelle.)

### 3.3 Hover/Diagnostic

Wenn das Modul typisierte Bezeichner einfuehrt: in
`diagnostic_provider.py` die `_MODULE_REQUIRED_DIRECTIVES`-Map ergaenzen,
damit der LSP weiss, welche Direktiven nur nach `#include('mymod')`
verfuegbar sind.

### 3.4 Im Skript benutzen

```cssc
#include('mymod') mm;
cssc::outln(mm::count());
mm::doStuff(7);
#free[mm];
```

---

## 4. Neue Statement-Form (z. B. `do { ... } until cond;`)

Statt `#` verwendet ein neues **Statement** in der Regel reservierte Keywords.

### 4.1 Token

In `cssl_cssc.py`:

```python
TK_DO = 'DO'
TK_UNTIL = 'UNTIL'
KEYWORDS['do'] = TK_DO
KEYWORDS['until'] = TK_UNTIL
```

### 4.2 Parser

`_parse_statement`:

```python
if tok.type == TK_DO:
    self._advance()
    body = self._parse_block_or_stmt()
    self._expect(TK_UNTIL)
    cond = self._parse_expression()
    self._match(TK_SEMICOLON)
    return {'type': 'do_until', 'body': body, 'cond': cond, 'line': tok.line}
```

### 4.3 Interpreter

```python
elif ntype == 'do_until':
    while True:
        for stmt in node['body']:
            self._exec_node(stmt)
        if self._eval(node['cond']):
            break
```

### 4.4 Native Codegen

```python
elif ntype == 'do_until':
    self._emit('do {')
    self.indent_level += 1
    self._loop_depth += 1
    for stmt in node.get('body', []):
        self._gen_node(stmt)
    self._loop_depth -= 1
    self.indent_level -= 1
    cond = self._gen_expr(node.get('cond'))
    self._emit(f'}} while (!cssc_is_truthy({cond}));')
```

### 4.5 LSP — TextMate-Grammar

Keywords ergaenzen:

```json
{
    "name": "keyword.control.cssc",
    "match": "\\b(do|until|...)\\b"
}
```

---

## 5. CSSL-Erweiterungen

CSSL ist Python-naeher und groesstenteils interpreterbasiert. Erweiterungen
gehen meist ueber:

- Built-in-Funktionen via `cssl_builtins.py`
- Neue Module via `cssl_<name>.py` und Eintrag in
  `cssl_runtime.py`'s Module-Dispatcher
- Klassen/Sektoren werden zur Laufzeit aus dem Quellcode geparst

Das Pattern ist analog zum CSSC-Modul-Beispiel oben — nur dass die Module
in CSSL als gewoehnliche Python-Klassen mit `__init__`, Methoden und ggf.
einem `get_namespace()` exportiert werden.

---

## 6. Test-Loop

Fuer jede Aenderung gilt:

1. **Interpreter** zuerst gruen kriegen — schnell zu iterieren:

   ```python
   from includecpp.core.cssl.cssl_cssc import CsscRunner
   r = CsscRunner()
   r.run_file('test.cssc')
   ```

2. **Nativer Build** parallel testen:

   ```powershell
   cssc build test.cssc -o test --save-c -v
   ```

   Mit `--save-c` schreibt der Compiler den generierten `test.c` neben das
   `.exe`. Beim Durchlesen siehst du sofort, wo Codegen-Bugs sitzen.

3. **Outputs vergleichen** — Interpreter und kompilierte Version muessen
   identisch sein. Abweichungen sind fast immer Bugs in der Codegen-Mapping
   (zwei verschiedene Builtin-Tabellen, vergessener Doppel-Eintrag).

4. **LSP separat starten** und `code .` im Test-Verzeichnis oeffnen, um
   Highlights und Diagnostics zu pruefen.

---

## 7. Typische Fallstricke

- **Zwei `_exec_node`-Methoden vergessen.** `cssl_cssc.py` enthaelt
  `class CsscRuntime` zweimal (alte und neue Architektur, beide aktiv).
  Aenderungen muessen in beide.
- **Builtin-Tabellen doppelt.** Genauso fuer Builtins und Module-Dispatcher.
- **Die native Runtime wird bei jedem `cssc build` neu gelinkt.** Du musst
  also keine `.lib` separat bauen — nur die .c/.h editieren.
- **`--save-c` ist Gold wert** wenn der Compiler-Output sich seltsam
  verhaelt. Lies den generierten C-Code, finde die Stelle, fixiere den
  Codegen.
- **GCC -O2 inlined Builtins.** Wenn du Funktionen mit Side-Effects
  emittierst (z. B. `time()`), muessen die in einer separaten C-File-Section
  liegen oder als `__attribute__((noinline))` markiert sein.
- **Token-Typ vergessen.** Wenn du ein neues Keyword einfuegst, **muss** es
  im `KEYWORDS`-Dict registriert sein, sonst lextet der Lexer es als
  Identifier.

---

## 8. Wo nachschauen

| Frage | Datei |
|-------|-------|
| Wie macht der Lexer aus `0x1A` einen Number-Token? | `cssl_cssc.py:_read_number` |
| Wie produziert der Parser ein AST-Knoten fuer `#define`? | `cssl_cssc.py:_parse_directive` |
| Wie wird `if/else` im Interpreter ausgefuehrt? | `cssl_cssc.py:_exec_if` |
| Wie wird `if/else` zu C kompiliert? | `cssc_compiler.py:_gen_if` |
| Welche Methoden hat `cssc_runtime.h`? | Direkt im Header — alle `CSSC_API`-Funktionen sind public |
| Welche Module sind eingebunden? | `cssl_cssc.py:_load_module` (zweimal!) |
| Welche TextMate-Patterns gibt es? | `vscode/cssl/syntaxes/cssc.tmLanguage.json` |
| Welche Direktiven kennt der LSP? | `diagnostic_provider.py:_CSSC_BUILTIN_DIRECTIVES` |

---

## 9. Cheatsheet — neue Direktive in 5 Minuten

1. `cssl_cssc.py` — Parser-Branch in `_parse_directive`.
2. `cssl_cssc.py` — Handler in BEIDEN `_exec_node`-Methoden.
3. `cssc_compiler.py` — `elif directive == '...'` in `_gen_node`.
4. `cssc_runtime.[ch]` — falls C-Helfer noetig, dort exportieren.
5. `diagnostic_provider.py` — Direktive-Name zur Whitelist.
6. `cssc.tmLanguage.json` — Highlight-Pattern.
7. `OBJ_USAGE_GUIDE.md` / `CSSC_Language_Documentation.md` — User-facing
   Doku updaten.
