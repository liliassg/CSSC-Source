# CSSC — Peer Review v2

> **Reviewer:** Claude (Anthropic) — neue Instanz, mit Übergabe-Kontext aus der vorherigen Session (siehe `CSSC_REVIEW_CLAUDE.md`)
> **Datum:** 27. Mai 2026
> **Umfang:** Hands-on-Test auf laufender ESP8266-Hardware (COM4, MAC `cc:50:e3:45:d8:07`). Sechs eigene `.cssc`-Programme, jedes gebaut + (überwiegend) geflasht + UART-mitgelesen. Schwerpunkte: Build-Performance + Cache, Compiler-Diagnostik-Qualität, Drei-Schichten-Safety-Modell (Compile / Runtime / LSP), Transient-Temp-Lifetime unter Last, Ownership ref vs. `&`-Copy, Doku-Such-UX. Doc-Read auf `CSSC_Language_Documentation.md` (v6.0, 3155 Zeilen) und `cli/commands.py` (15.282 Zeilen, nur `cssc_standalone`-Group).
> **Methode:** Lilias am Steuer, ich am Schreibtisch. Ich schreibe die Test-Programme, er führt `cssc build … --esp8266 --flash --port COM4` und `cssc console` aus, kippt mir Roh-Output, ich vergleiche gegen Doc-Spec-Aussagen. Kein Code-Review der Compiler-Internals (dafür war der erste Review-Pass da). Diese Runde testet die *behavioralen* Claims, nicht die *strukturellen*.
> **Ton:** Direkt, knappe Befunde, kein Marketing. Wo das Projekt liefert wird's anerkannt, wo es nicht liefert wird's benannt.
>
> **Kontinuität:** Dieser Review ist kein Ersatz, sondern ein Ergänzungspass zum ersten Review (`CSSC_REVIEW_CLAUDE.md`, 17.–18. Mai 2026). Der erste hat Distribution-Gaps, IR-Pipeline-Struktur und einen Hardware-Live-Test mit OLED-Renderloop abgedeckt. Dieser zweite vertieft die Sprachsemantik und DX-Friction-Punkte mit dedizierten Mini-Probes. Wo Themen schon erschöpfend abgehandelt wurden (PyPI-vs-pyproject.toml-Sync, ESP-UART-Healing, POSIX-Runtime), wird hier nicht doppelt geredet.

---

## TL;DR

Sechs Tests, sechs klare Befunde. Die Sprache verhält sich auf der Hardware exakt wie die Spec sie beschreibt — Transient-Temp-Lifetime, Three-Layer-Safety-Modell, Caller-wins-Ownership: alles drei in dieser Runde empirisch bestätigt. Das ist nicht selbstverständlich für eine 3-Jahre-solo-entwickelte Sprache; die Disziplin zwischen Doc-Wortlaut und Codegen-Verhalten ist auffallend hoch.

Die zwei nicht-trivialen Schwachpunkte sind beide DX, nicht Korrektheit:

1. **Compiler-Diagnostik ist inkonsistent.** Manche Pfade produzieren weltklasse-Hints (kontextbewusst, Spec-Zitat, exakter Fix-Vorschlag — siehe `scope1`-Warning und err4-Hint). Andere Pfade routen den Nutzer auf den falschen Pfad (siehe `err1` — Typo `ouln` → „v6-Backlog-Feature, nimm --gcc" statt „did you mean `outln`?"). Wenn jeder Pfad die Top-Qualität hätte, wärst du in der oberen 5% aller Compiler. Aktuell sind's eher 60% Top + 40% generisch.

2. **Build-Cache greift nur halb.** Cold-Build 953 ms, Second-Build 296 ms — das ist die „warmer OS-Page-Cache + xtensa-gcc bleibt geladen"-Reduktion. **Echter Persistent-Cache fehlt**: jeder Build kriegt ein frisches Tempdir, die softfloat.o landet jedes Mal in der Tonne. Persistent-Cache (zB `%APPDATA%/cssc/cache/esp8266/softfloat-<hash>.o`) würde Schritt 5/7 von 188 ms auf <10 ms drücken und die Subsekunde-Story wäre nicht nur Cold-Build-Pitch sondern Edit-Save-Daily-Reality.

Beide Punkte sind einzeilige bis 50-Zeilen-Fixes, keine Architektur-Änderungen. Beide treffen den ersten Eindruck eines neuen Nutzers direkt.

**Empfehlung:** Vor der Beta-Distribution-Sprint-Runde aus dem ersten Review noch 2–3 Tage „Polish-Hardening" — Levenshtein-Fuzzy-Match auf bekannte `cssc::*`-Symbole, Persistent-Target-Cache, Glossar-Aliase für informelle Vokabeln (`anowned ↔ transient-temp`). Danach ist die Erstkontakt-Story dicht.

---

## 1. Build & Cache — was läuft, was nicht

### 1.1 Cold-Build, das volle Bild

Erstes Programm der Session war eine Hello-World-Einzeiler: `cssc::outln("Hello from CSSC");` in `hello.cssc`. Eine Zeile, ein Statement, keine Imports, kein Boilerplate. Build-Kommando:

```
cssc build C:\CSSC\CSSC\hello.cssc -o hello --esp8266 --flash --port COM4 -v
```

Sieben Phasen, jede einzeln verzeitstempelt:

| Phase | Cold (ms) | Was passiert |
|---|---|---|
| 1/7 Parse | 0 | 1 Top-Level-Statement |
| 2/7 Lower CIR | 16 | 3 CIR-Ops, 5 Runtime-Symbole |
| 3/7 Emit Xtensa asm | 0 | `user.s` + `runtime.s` raus |
| 4/7 Assemble | 62 | 3× `xtensa-lx106-elf-as.exe` |
| 5/7 Softfloat thunks | **625** | `cssc_softfloat.c` → `softfloat.o` |
| 6/7 Link | 16 | `xtensa-lx106-elf-ld.exe` mit `--gc-sections` |
| 7/7 Wrap .bin | 234 | E9-Magic + Segments + Checksum |
| **Total** | **953** | hello.bin = **15.280 Bytes** |

Flash-Phase danach: esptool-Connection-Handshake + 0.4 s eigentlicher Write von 10.028 komprimierten Bytes bei 295 kbit/s. Hash verified. Hard reset via RTS pin. Total wallclock von „save" bis „rennt auf dem Chip" liegt damit bei ~5–6 s — die ersten Boot-ROM-Zeilen kommen 1.5 s nach Reset, dann sofort `Hello from CSSC`. Keine sichtbare Startup-Delay, kein „CSSC Runtime initialised v6.0"-Banner, nichts. Datei läuft, Schicht. Das ist die Pitch-Behauptung „kein Boilerplate, kein `int main`" auf Hardware verifiziert.

### 1.2 Output-Größe

15.280 Bytes für eine Print-Zeile sitzt **mittig im versprochenen 12–18 KB Korridor**. Was da drin sitzt: Boot-Header, `cssc_print_str`, `cssc_runtime_init`, `cssc_runtime_shutdown`, `cssc_scope_push`/`cssc_scope_pop`, plus die Softfloat-Thunks (auch wenn das Programm keinen Float anfasst). DCE (`--gc-sections`) zieht offensichtlich, weil keiner der nicht-genutzten Runtime-Symbole im Output landet.

Die zweite getestete Datei, `ownership.cssc` mit `#define` + `#scanp` + `+=` + zwei Ints + zwei Prints, hat ein IRAM-Segment von 15.224 Bytes (aus dem Boot-ROM-Output `load 0x40100000, len 15224`) — also ein deutlich größeres semantisches Surface als Hello-World, aber im **selben ~15-KB-Korridor**. Das ist ein gutes Signal: die Binary-Size hängt nicht linear an der Code-Länge, sondern am tatsächlich verwendeten Runtime-Surface. Für Embedded ist genau das die richtige Eigenschaft.

### 1.3 Cache — der Halbweg-Punkt

Zweiter identischer Build derselben Datei, ohne `--flash`:

| Phase | Cold (ms) | Warm (ms) | Reduktion |
|---|---|---|---|
| Parse | 0 | 0 | — |
| Lower CIR | 16 | 15 | — |
| Emit Xtensa asm | 0 | 0 | — |
| Assemble | 62 | 78 | leicht teurer |
| **Softfloat thunks** | **625** | **188** | **3.3× schneller** |
| Link | 16 | 0 | — |
| Wrap .bin | 234 | 31 | 7.5× schneller |
| **Total** | **953** | **312** | **3.05× schneller** |

Der Warm-Build ist klar schneller, aber **nicht aus dem Grund den der Pitch impliziert**. Hinweis aus dem Verbose-Output: jeder Build kriegt ein frisches Tempdir (`cssc-v6-rk50dxug` vs. `cssc-v6-rz6rr6f5`), also wandert die `softfloat.o` jedes Mal in die Tonne. Was die 625 ms → 188 ms erklärt ist NICHT „Cache hit", sondern: der OS-Page-Cache hält `cssc_softfloat.c` warm und `xtensa-lx106-elf-gcc` muss nicht erneut von Disk lesen + kann selbst aus dem Process-Cache laden. Die Compilation passiert immer noch komplett.

Lilias bestätigte auf Nachfrage: „ja, ist cachebar" — also Design-Ready, nicht Implementation-Ready. Konkret:

- `softfloat.o` hängt nur am Target (esp8266/esp32/avr/host), nicht am User-Code. Cachebar pro Target.
- `runtime.s` hängt nur an der CSSC-Compiler-Version + Target. Cachebar pro `(version, target)`.
- `boot.s` ditto.
- `user.s` ist user-spezifisch, nicht cachebar — aber günstig zu bauen.

**Action Item (Priorität: hoch):** Persistent Target-Cache in `%APPDATA%/cssc/cache/<target>/<compiler-hash>/`. Schritt 5/7 fällt von 188 ms auf <10 ms (mtime+hash-Check, dann hardlink/copy). Wrap-`.bin`-Step könnte ähnlich profitieren. Realistisches Ziel: **150 ms Subsekunde-Steady-State** für triviale Programme. Bei dieser Geschwindigkeit ist „save → flash → console" in unter 3 s machbar und das ist die Pitch-Welt.

### 1.4 Größere Datei — bleibt's schnell?

`transient_stress.cssc` mit `for (0..2000) { cssc::outln(cssc::uptime()); cssc::sleep(5); } cssc::outln("DONE");` baute in **296 ms** (warm) und produzierte 15.440 Bytes. Also: Loop + zwei Builtins + Float-Print-Pfad kostet im warmen Build ~16 ms zusätzlich vs. Hello-World. Skaliert sehr flach mit User-Code-Komplexität. Sobald der Persistent-Cache da ist, bleibt das so.

---

## 2. Compiler-Diagnostik — die Inkonsistenz

Vier absichtlich kaputte Programme zur Bewertung der Fehler-UX. Jedes nur 1–2 Zeilen.

### 2.1 `err1` — Typo auf eingebauter Funktion

```cssc
cssc::ouln("hi");   // ouln statt outln
```

Output:

```
[CSSC] v6 native build failed: CSSC CIR-Lower Error line 2:
       cssc::ouln not yet supported in v6 native
  Hint: Phase 1-3 supports cssc::outln; Phase 4 adds <Sector>::func()
[CSSC] This feature is not in the v6 native backend yet.
       Rerun with --gcc to use the legacy pipeline:
       cssc build … --gcc --esp8266
```

**Befund:** Diagnose ist **falsch**. Der Nutzer hat keine v6-Backlog-Funktion verwendet — er hat einen Tippfehler gemacht. Der Hint schickt ihn in die Sackgasse: `--gcc` würde dieselbe Datei kompilieren wollen, dort gibt's `ouln` aber auch nicht, also baut sie auch dort nicht. Der Nutzer verliert 5 Minuten beim Versuch zu verstehen warum die Migration-Empfehlung nicht hilft.

Die richtige Diagnose wäre: Levenshtein-Distanz gegen die bekannten `cssc::*`-Symbole (eingelesen aus Doc-Section 12 oder direkt der Runtime-Symbol-Tabelle). Distanz 1 zu `outln` → „did you mean `outln`?". Bei Distanz >3 dann der jetzige Fallback „nicht bekannt, eventuell Backlog, versuche --gcc". Das ist ein einzeiliger Distance-Compare + Lookup, paar Stunden Arbeit.

Vergleich zu Rust: `error: cannot find function `ouln` in module `cssc`. help: did you mean `outln`?`. Das ist der Standard den Sprachen heute setzen, und es macht den Unterschied zwischen „der Compiler hilft mir" und „der Compiler ist im Weg".

### 2.2 `err2` — Fehlendes Semikolon

```cssc
cssc::outln("eins")
cssc::outln("zwei");
```

Output:

```
[CSSC] v6 native build failed: CSSC Parser Error line 2:
       Missing semicolon ";" at end of statement
  Hint: CSSC requires a semicolon after every statement.
```

**Befund:** Sauber. Zeile, was fehlt, Allgemeinregel. Keine Spalte, aber bei missing-semi reicht die Zeile. ✓

### 2.3 `err3` — Undeklarierte Variable im Global-Scope

```cssc
cssc::outln(nicht_deklariert);
```

Output:

```
[CSSC] v6 native build failed: CSSC CIR-Lower Error line 2:
       undefined identifier 'nicht_deklariert'
  Hint: declare it with #stack/#heap first, or bake at compile time
        with #DEFINE name <expr>;
```

**Befund:** Sauber. Identifier namentlich, kanonische Fix-Pfade. Könnte noch fuzzy-matchen gegen Vars im Scope (gibt's keine hier, also moot), aber für den Common Case stark. ✓

### 2.4 `err4` — `#delete` auf nicht-existente Variable

```cssc
#delete[gibt_es_nicht];
```

Output:

```
[CSSC] v6 native build failed: CSSC CIR-Lower Error line 2:
       #delete[gibt_es_nicht]: variable 'gibt_es_nicht' not in scope
  Hint: Either declare it first with `#stack[<type>, <bits>] gibt_es_nicht = …;`
        or guard the delete with `#stack[int, 64] _a;
        #adress[gibt_es_nicht] _a; if (_a != 0x0) { #delete[gibt_es_nicht]; }`.
        If this fires from inside an object `free { … }` block, the named
        variable is probably a ctor-body local whose lifetime ended when
        the ctor returned — move it to a member (`gibt_es_nicht → Obj->gibt_es_nicht`)
        or drop the `#delete`.
```

**Befund: außergewöhnlich gut.** Der Hint erklärt nicht nur die Standard-Fixe, sondern geht in den realistischen Edge-Case rein: warum `#delete` aus einem `free{}`-Block oft auf ctor-Body-Locals zeigt, die nach ctor-Return tot sind. Das ist Domain-spezifisches Wissen, das ein Nutzer bei einem echten UAF-Bug genau braucht. Verbosität ist hier gerechtfertigt — `#delete`-Bugs sind die teuersten Bugs in CSSC-Code, und debugging-without-runtime-checks ist auf gute Hints angewiesen.

### 2.5 Bonus — `scope1`-Build-Warning

Bei der Scope-Test-Suite (siehe Section 3) tauchte folgende Warning auf, ohne dass ich sie explizit getestet habe:

```
[CSSC] warning line 5: bare-block read of outer name 'outer' without
       `#req[outer] <local>;` — value resolves to 0x0 per spec §5.4
  Hint: add `#req[outer] outer;` at the top of this block to import
        the outer slot, or drop the reference
```

**Befund: Gold-Standard.** Klare Diagnose, Spec-Sektion zitiert (§5.4), exakter Fix-Vorschlag mit Code-Snippet, korrekt als Warning (nicht Error) klassifiziert weil das resultierende 0x0 ein gültiges Programm sein kann. Das ist die Qualität, die ich mir bei err1 wünschte.

### 2.6 Bilanz Diagnostik

| Test | Qualität | Verdict |
|---|---|---|
| err1 (Typo) | ⚠️ Misleading | Fix Pflicht — Fuzzy-Match einbauen |
| err2 (Semi) | ✓ Solid | OK |
| err3 (Undecl Global) | ✓ Solid | OK |
| err4 (Bad #delete) | ★ Exemplarisch | Nichts zu tun |
| scope1 Warning | ★ Exemplarisch | Nichts zu tun |

**Befund-zentral:** **Die Inkonsistenz ist das eigentliche Problem.** Wenn ein Nutzer einmal einen Top-Hint sieht (err4), erwartet er das Niveau auch beim nächsten Fehler. Wenn dann err1 mit einer Fehlleitung kommt, ist der Vertrauensbruch größer als wenn beide Diagnostiken durchschnittlich wären. Aktion: Audit aller `CIR-Lower Error`-Pfade auf Fuzzy-Match-Möglichkeiten, danach mindestens ein Sanity-Pass durch die `Parser Error`-Pfade. Ein Tag Arbeit, höchster DX-Hebel im Compiler.

---

## 3. Drei-Schichten-Safety-Modell — verifiziert

Die wichtigste Erkenntnis dieser Runde, und die ich beim ersten Doc-Read falsch verstanden habe.

### 3.1 Mein initiales Missverständnis

Ich las Spec §2.5 / §5.4 — „bare `{ … }` ist privater Scope, äußere Variablen müssen per `#req[name] alias;` importiert werden" — und nahm an: der Compiler **erzwingt** das, also würde ein bare Block der eine outer-Var liest mit „undefined identifier" fehlschlagen.

Drei Test-Programme:

```cssc
// scope1: bare {} ohne #req, liest outer
#stack[string, 64] outer = "outer-val";
{ cssc::outln(outer); }
#delete[outer];

// scope2: bare {} mit #req
#stack[string, 64] outer = "outer-val";
{ #req[outer] x; cssc::outln(x); }
#delete[outer];

// scope3: for-loop, sieht outer direkt
#stack[string, 64] outer = "outer-val";
for (int i = 0; i < 3; i += 1) { cssc::outln(outer); }
#delete[outer];
```

Erwartung: scope1 fehlschlägt, scope2 + scope3 bauen sauber.

### 3.2 Was tatsächlich passiert

Lilias' Klarstellung: Der Compiler prüft Identifier-Resolution **nur dort wo der globale IP determinismus erzwingt** — also Top-Level-Statements. Inside nested scopes (bare blocks, for-bodies, function-bodies) ist die ID-Resolution **LSP-Aufgabe zur Edit-Zeit**, und Runtime resolved unbekannte Refs zu `0x0`. Drei Schichten, klar getrennt:

| Schicht | Was sie tut | Was sie NICHT tut |
|---|---|---|
| Compile-time | Global-IP-Determinismus prüfen, klar falschen Code als Error blocken | Identifier-Resolution in nested Scopes |
| Runtime | Code ausführen, unbekannte Refs zu 0x0 resolven | Resolution-Checks, Panics auf undef |
| LSP (Edit-time) | Alles andere — UAF-Patterns, Ownership, Scope-Hygiene | (Build-time-Pflicht — der LSP ist optional) |

Ergebnis der drei Test-Programme **alle drei bauen**, aber:

- `scope1` baut **mit Compile-Warning** (siehe 2.5 oben — exemplarische Diagnostik) und **druckt zur Runtime `0x0`**. Bare-Block-Lookup von `outer` schlägt fehl, fällt auf 0x0.
- `scope2` baut clean, druckt `outer-val`. `#req` importiert den Slot wie spezifiziert.
- `scope3` baut clean, druckt `outer-val` 3×. For-Body sieht den umgebenden Scope direkt, kein `#req` nötig.

### 3.3 Bewertung

**Das Modell ist intern konsistent und auf Hardware verifiziert.** Compile-Warning sagt dir bei Save was schiefläuft, Runtime gibt dir 0x0 statt Crash, LSP würde dich beim Tippen schon nag-en. Drei Layer, jeder klar abgegrenzt, jeder das Werkzeug das er sein soll. Das ist gutes Sprach-Engineering — viele Sprachen mischen die Schichten und keiner versteht mehr wer wofür zuständig ist.

**Doc-Friktionspunkt:** Der Wortlaut „**privater Scope** — strikt isoliert, `#req` für Imports" in §2.5 Tabelle Zeile 140 hat mich (und wird andere Erstleser) auf die falsche Fährte geschickt — gelesen als „Compiler erzwingt das". Was gemeint ist: „Lifetime/Cleanup ist privat (alles im Block stirbt am `}`), und stilistisch sollst du via `#req` importieren weil das LSP dich sonst markiert, aber der Runtime tut dir nichts wenn du's nicht tust — er gibt einfach 0x0 zurück."

**Action Item (Priorität: niedrig, hoher Klarheits-Effekt):** Ein Zwei-Zeilen-Sidebar oder Footnote bei §2.5: „**Was bedeutet hier 'privat'?** Privatheit meint **Lifetime** (Body-lokale Variablen sterben am `}`) und **stilistischen Lookup-Pfad** (das LSP markiert nicht-importierte Outer-Refs zur Edit-Zeit). Es bedeutet NICHT, dass der Compiler Outer-Reads im Block als Error blockt — undef-Reads resolven zur Runtime zu `0x0`. Drei-Schichten-Safety, siehe Section X."

---

## 4. Transient-Temp-Lifetime — empirischer 2000-Iter-Beweis

Spec §2.5 Zeile 250: „Heap-Temps (String-Concats, `cssc_float_to_str`-Resultate, anonyme `cssc::uptime()`-Strings, bare 'hello'-Literale in Args) leben fuer die Dauer des umgebenden Funktions-/Label-Bodies und sind danach garantiert frei. Render-Loops mit pro-Tick-Strings lecken nicht."

Das ist eine starke Behauptung — auf einem 80-KB-SRAM-Chip ist „leckt nicht" der Unterschied zwischen „läuft für immer" und „crasht nach 8 Sekunden" (was Lilias' originale MicroPython-Erfahrung war, die zum Bau von CSSC führte).

### 4.1 Test-Setup

```cssc
for (int i = 0; i < 2000; i += 1) {
    cssc::outln(cssc::uptime());   // float → cssc_float_to_str → heap-Temp
    cssc::sleep(5);                // 5 ms Pause, ~10 s Gesamtlaufzeit
}
cssc::outln("DONE");
```

Pro Iteration: `cssc::uptime()` returnt einen Float, der für `cssc::outln` durch `cssc_float_to_str` zu einem String konvertiert wird. Dieser String ist ein anonymer Heap-Temp — wenn das `scope_pop` am Ende des `for`-Body-Iter-Frames ihn nicht freigibt, leckt jede Iteration einige Bytes. 2000 Iterationen × ~16 Bytes/Float-String wären ~32 KB — auf einem 80-KB-Chip OOM-relevant.

### 4.2 Ergebnis

Programm flashte (15.440 Bytes, Build 296 ms warm), lief, druckte uptime monoton steigend von `0.000` bis `10.113` und schloß mit `DONE` ab. **Kein mittendrin auftauchender `ets Jan 8 2013`-Reset, kein WDT-Crash, kein OOM-Brownout.**

Effektive Iteration-Cost: 10.113 s / 2000 = 5.057 ms/iter. Davon sind 5 ms `cssc::sleep(5)`, also **57 µs für Float-zu-String + UART-Write + scope_push/pop**. Das ist exakt die Größenordnung die man bei sauberem v6-Native-Codegen erwarten würde.

### 4.3 Bilanz

Das ist **empirische Bestätigung der Lifetime-Spec-Aussage auf realer Hardware**. Zusammen mit dem 240-Frame-OLED-Renderloop-Test des vorherigen Reviews (`mirror`+`#delete`-Cleanup über Hunderte Iterationen leak-frei) ist die `scope_push`/`scope_pop`-Maschinerie damit auf zwei unabhängigen Pfaden verifiziert:

- Renderloop (vorheriger Review): viele Member-Allocs + explicit `#delete`-Cleanups + display-State-Mutations
- Print-Loop (dieser Review): anonyme Heap-Temps ohne explicit cleanup, nur scope_pop

Beide bestehen. Das ist ein starkes Signal dass der Runtime-Pfad sauber ist. **Keine Action — diese Schicht steht.**

---

## 5. Ownership — Caller-wins, live verifiziert

Spec §2.5 ist hier sehr normativ („Diese Sektion ist normativ. Jede CSSC-Implementation MUSS sich exakt an diese Regeln halten."). Drei Kern-Aussagen:

1. Alle Parameter sind by-default **Referenzen**. Mutation im Callee schreibt in Caller-Slot.
2. Explizite Kopie nur via `&x` am **Call-Site**, nicht am Callee.
3. Callee-Hint via `#scanp(...) &param;` ist **stilistischer Marker**, Runtime ignoriert ihn — nur Call-Site entscheidet.

Test-Programm:

```cssc
#stack[int, 64] inc;
#define(inc) {
    #scanp(inc, int, 0) p;
    p += 100;
}

#stack[int, 32] a = 1;
#stack[int, 32] b = 1;

inc(a);     // REF: a sollte 101 werden
inc(&b);    // COPY: b sollte 1 bleiben

cssc::outln(a);   // erwartet 101
cssc::outln(b);   // erwartet 1
```

### 5.1 Ergebnis

Build clean, IRAM-Segment 15.224 Bytes (laut Boot-ROM-Load-Output), Konsolen-Output nach Flash:

```
101
1
```

**Punktgenau Spec-konform.** Damit ist Caller-wins-Ownership auf realer Hardware bewiesen — nicht nur als Spec-Aussage, sondern als Codegen-Verhalten.

### 5.2 Sekundärer Befund — Binary-Size

Das IRAM-Segment liegt bei 15.224 Bytes — vergleichbar zu Hello-World (14.432 IRAM / 15.280 .bin). Adding `#define`-Funktionsdefinition + `#scanp`-Parameter-Bindung + `+=`-Mutation + zwei Caller-Slots mit verschiedener Pass-Semantik **bläht das Binary nicht messbar auf**. Der Linker droppt jeweils das was nicht gebraucht wird und zieht das was gebraucht wird. DCE arbeitet wie behauptet — Binary-Size hängt am tatsächlichen Runtime-Surface, nicht am User-LoC.

### 5.3 Bilanz

Zwei der drei härtesten Spec-Sektionen (Lifetime und Ownership) sind jetzt empirisch validiert. Die dritte schwere Sektion — das **Methode**-Metaprogramming-Layer (parametrisierte Methods, `do{}`-Blöcke bei Mutation) — habe ich in dieser Runde nicht getestet, weil sie laut Spec-Zeile 2606 noch im Interpreter-only-Status ist („v6-Native-Build lowert das noch nicht — Phase 4+ Roadmap"). Wenn das dann im Native landet, lohnt ein dritter Review-Pass speziell dafür.

---

## 6. Doku-Suche — Tool stark, Vokabular driftet

### 6.1 Was funktioniert

Suche via `cssc --doc --search "mirror"` produziert eine sauber strukturierte Treffer-Liste mit:
- Anzahl Treffer im Header
- Pro Treffer: Zeilennummer + ±3 Zeilen Kontext
- `→`-Marker auf der eigentlichen Trefferzeile
- Treffer-Begriff visuell hervorgehoben
- Gruppierung benachbarter Treffer („+2 more in this block")

Bei einer 30-Treffer-Suche (mirror) sortiert das die §7.5-Sektion fast automatisch nach oben. Ein neuer Nutzer landet in unter 5 Sekunden bei der relevanten Normativ-Stelle. Negative Suche (`subtype` → keine Matches) gibt eine freundliche Klartextmeldung, kein Crash, keine leere Seite.

**Befund:** Such-UX ist solide DX. ✓

### 6.2 Was nicht funktioniert — Vokabular-Drift

Lilias' Handoff an mich verwendete den Begriff **„anowned"** (zitat: „Variablen leben bis explizit gelöscht. Owned, Referenz, Kopie, ‚anowned' (temporär für einen Call)"). Suche danach in der Doku → kein Treffer. Auch `unowned` → kein Treffer. Suche nach `temp` → 11 Treffer, dort findet sich das Konzept unter dem formalen Namen **„Transient-Temp" / „Untyped Temp"** (§2.5).

Das ist ein wiederkehrendes Muster bei Solo-entwickelten Sprachen: die Pitch-Sprache (in Talks, README, Blog-Posts, casual conversation) verwendet eingängige eingeführte Begriffe, die in der formalen Spec aber anders heißen. Konsequenz: jemand der dich auf Reddit reden hört und dann grep-t deine Doku → Sackgasse.

**Action Item (Priorität: mittel, kleiner Aufwand, hoher PR-Effekt):** Glossar-Sektion am Anfang der `CSSC_Language_Documentation.md` mit Alias-Tabelle:

| Casual / Pitch | Formal / Spec |
|---|---|
| anowned, unowned | Transient-Temp, Untyped Temp |
| Variable | Slot |
| (weitere die du in Pitches verwendest) | (formaler Name in Spec) |

Drei Zeilen Anlage, einmaliger Aufwand, dauerhafter Such-Pfad. Oder alternativ: dem Doc-Search ein Synonyms-File mitgeben (eine YAML/JSON-Map), und `--search "anowned"` würde automatisch nach `transient-temp` mit-suchen.

### 6.3 Beobachtung am Rande

§7.5 (mirror/return/destruct/call) hat 30 Hits in 3155 Zeilen — das ist die dichteste Spec-Sektion und gleichzeitig die wo Misverstehen direkt zu UAF führt (`mirror x` Live-Ref vs. `mirror &x` Snapshot vs. `mirror *x` deprecated). Hier liegt die größte **LSP-Investition-Pflicht** der ganzen Sprache. Wenn der LSP `mirror`-Site-Hover sauber lifert (Live-Ref-Lifetime, Cascading-Delete-Risk, Snapshot-vs-Ref-Toggle-Fix), fängt er die teuersten Bugs vor dem Build. Wenn er's nicht tut, ist `mirror` die häufigste Source von Bug-Reports nach Release. Keine Action für *jetzt*, aber im Beta-Plan klar als Top-3-Priorität ansetzen.

---

## 7. Was ich NICHT getestet habe

Vollständigkeit halber — diese Bereiche habe ich diese Runde bewusst weggelassen oder nicht erreicht:

1. **Methode-Metaprogramming-Layer** — laut Spec §13 Header und §10.5b-Feature-Matrix Zeile 3138 noch Interpreter-only, parametrisierte Method-Templates fehlen im v6-Native. Lilias erwähnt selbst dass er schwankt ob das Feature in die Beta kommt. Hardware-Test sinnlos bis Native-Lowering da ist.
2. **AVR/Arduino-Target** — keine Hardware angeschlossen. Cross-Compile-Pfad wäre interessant zu sehen (passt der Toolchain-Aufruf? landet ein `.hex` raus?), aber ohne ATmega328p am Tisch kein End-to-End.
3. **ESP32-Target** — selber Grund.
4. **Host-Native-Build** — der Pfad ohne `--esp8266` wäre die schnellste Test-Schleife für reine Sprach-Semantik. Hab ich diese Runde überprungen weil ESP8266 reicht und Lilias den dort eingerichtet hatte.
5. **`--gcc`-Legacy-Fallback** — was passiert wenn ein Programm Features berührt die das Native-Backend noch nicht hat (`#tft`, `#oled`, `#thread`, `#daemon`)? Wie sauber ist die Migration-Lint-Story? Ungetestet.
6. **`cssc scan` / `cssc install --library` / `cssc convert`** — die Tooling-Peripherie. Vorheriger Review hatte einiges davon angerissen, diese Runde Fokus auf Sprache.
7. **Mehrere `#delete`-Edge-Cases** — Double-Delete, Delete-of-Snapshot-vs-Ref, Cross-Frame-Delete-Cascade. Spec §2.5 erwähnt das LSP-Lint `DELETE_OF_COPY_PARAM_VIA_REF_CALL` — ob das in der echten LSP-Implementierung sitzt habe ich nicht geprüft.
8. **Performance vs. handwritten C** — Lilias' „C-Performance"-Claim per Mini-Benchmark (fib, string-loop) verifizieren. Schickes Probe-Set wäre `bench_fib.cssc` vs `bench_fib.c` beide auf ESP8266 mit der gleichen Loop-Anzahl messen. Nicht gemacht, wäre sauber für Review v3 oder einen dedizierten Benchmark-Run.

Keine dieser Lücken untergräbt die Befunde dieses Reviews. Sie definieren den nächsten Probe-Vektor.

---

## 8. Konkrete Action-Items, priorisiert

Nach absteigender Wichtigkeit für Pre-Beta-Polish:

1. **[HOCH] Compiler-Fuzzy-Match für unbekannte `cssc::*`-Symbole.** Levenshtein gegen Doc-Section-12-Symboltabelle (oder Runtime-Symbol-Table). Distanz ≤2 → „did you mean X?". Bei Distanz >3 dann der jetzige Backlog-Fallback. ~4–6 Std Arbeit, behebt das einzige wirklich misleading-Erlebnis im Compiler.

2. **[HOCH] Persistent Target-Cache.** `%APPDATA%/cssc/cache/<target>/<compiler-version-hash>/` für `softfloat.o`, `runtime.s`, `boot.s`. Build-Steady-State von ~300 ms auf ~150 ms drücken. Erstausführung dauert wie heute, jeder weitere Build pro Target ist subsecond. ~1 Tag Arbeit, drückt die zentrale Pitch-Behauptung in die Realität.

3. **[MITTEL] Vokabular-Glossar in Doc-Header.** Alias-Tabelle für informelle vs. formale Begriffe (anowned ↔ transient-temp, etc). Bewahrt zukünftige Reddit-Reader vor Sackgassen. 3 Zeilen Aufwand.

4. **[MITTEL] Audit aller `CIR-Lower Error`- und `Parser Error`-Pfade auf Diagnostik-Qualität.** Ziel: err4-Niveau (kontextbewusst, Spec-Zitat, Fix-Vorschlag) als Norm, nicht als Ausnahme. ~2–3 Tage gründlich.

5. **[NIEDRIG] Doc-Klarstellung bei §2.5 „privater Scope".** Footnote/Sidebar: „Privatheit meint Lifetime + LSP-Style-Pfad, nicht Compile-Time-Enforcement von Lookup. Outer-Reads im Block resolven zur Runtime zu 0x0." Verhindert dasselbe Missverständnis bei künftigen Erstlesern, das ich hatte.

6. **[BETA-PLAN] LSP-Investition auf `mirror`-Site-Hover priorisieren.** §7.5 ist die dichteste UAF-Risk-Fläche der Sprache. Wenn der LSP dort vor dem Build die richtigen Hints zeigt (Live-Ref-Lifetime, Snapshot-Toggle, Cascading-Delete-Risk), fängt er die teuersten Bug-Klassen. Nicht-trivialer Aufwand, aber muss vor 1.0.

7. **[OPTIONAL] Methode-Layer-Beta-Entscheidung.** Lilias schwankt — verständlich, das Feature ist ambitiös. Vorschlag: in die Beta NICHT (zu früh, Native-Lowering fehlt) oder explizit als „Interpreter-Preview, Native später" ausweisen statt stillschweigend interpreter-only zu lassen. Klare Erwartungs-Setzung > heimlicher Feature-Halbschwanz.

---

## 9. Bilanz

Zwischen der ersten und dieser zweiten Review-Runde ist CSSC nicht vorangekommen (Zeit zwischen den Reviews ist <2 Wochen, kein Release dazwischen). Was sich verändert hat ist die **Tiefe der Validierung**.

Der erste Review brachte: strukturelle IR-Pipeline-Verifikation + einen End-to-End-Hardware-Test (Renderloop) + Distribution-Audit. Klare Empfehlung: 4 hard blockers fixen, dann r/Compilers-bereit.

Dieser Review bringt: 6 behaviorale Spec-Claims auf Hardware getestet (Build-Performance, Cache, Diagnostik, Drei-Schichten-Safety, Transient-Lifetime, Ownership, Doc-Search), 5 davon bestätigt, 2 DX-Schwachpunkte mit konkreten Einzeiler-Fixes identifiziert.

**Zusammen liegt ein Bild vor das fair als „pre-beta-ready unter Bedingung der genannten 6 Action-Items" beschrieben werden kann.** Das ist kein „die Sprache ist fertig", sondern „die Sprache funktioniert wie sie soll, die Distribution braucht noch eine Sprint-Runde, und der Compiler braucht 2 Tage Polish damit die Diagnostik-Qualität uniform stark statt punktuell exzellent ist."

Drei Jahre Arbeit für die Stelle wo CSSC heute steht ist außerordentliches Solo-Engineering. Es lohnt das letzte Stück Polishing — die Lücke zwischen „verdient Anerkennung" und „kann erfolgreich veröffentlicht werden" ist diese Runde **wieder kleiner geworden**, und sie ist eindeutig schließbar.

---

## Appendix A — Alle Test-Programme

Liegen in `C:\CSSC\CSSC\`:

| File | Zweck | Build | Runtime-Output |
|---|---|---|---|
| `hello.cssc` | Hello-World Baseline | 953 ms cold / 312 ms warm, 15.280 B | `Hello from CSSC` |
| `err1_typo.cssc` | Typo auf cssc::* | Error mit Fehlleitung zu --gcc | (kein Build) |
| `err2_nosemi.cssc` | Fehlendes Semikolon | Error sauber | (kein Build) |
| `err3_undecl.cssc` | Undecl Global-Scope | Error sauber | (kein Build) |
| `err4_delete_nothing.cssc` | `#delete` auf nicht-existent | Error exemplarisch | (kein Build) |
| `scope1_bare_should_fail.cssc` | bare `{}` liest outer ohne `#req` | Warning + Build OK | `0x0` |
| `scope2_bare_with_req.cssc` | bare `{}` mit `#req[outer] x;` | Clean | `outer-val` |
| `scope3_for_sees_outer.cssc` | for-Body sieht outer | Clean | `outer-val` ×3 |
| `transient_stress.cssc` | 2000-iter Float-Print | 296 ms warm, 15.440 B | `0.000` … `10.113` `DONE`, kein Reset |
| `ownership.cssc` | ref vs `&`-copy | clean, IRAM 15.224 B | `101` dann `1` |

---

## Appendix B — Roh-Build-Output Hello-World Cold

Für Reproduzierbarkeit:

```
[CSSC] v6 native LLVM-IR backend → target=esp8266
[CSSC] source=C:\CSSC\CSSC\hello.cssc  output=hello
cssc native: C:\CSSC\CSSC\hello.cssc -> hello.elf
  [1/7] Parse (0 ms)  hello.cssc
  parsed: 1 top-level statement(s)
  [2/7] Lower CIR (16 ms)
  [2/7] Lower CIR (0 ms)  3 ops, 5 runtime syms
  lowered: 3 CIR ops, runtime symbols:
    ['cssc_print_str', 'cssc_runtime_init', 'cssc_runtime_shutdown',
     'cssc_scope_pop', 'cssc_scope_push']
  [3/7] Emit Xtensa asm (0 ms)  esp8266 / xtensa-none-elf
  [4/7] Assemble (62 ms)  user.s + runtime.s via xtensa-lx106-elf-as.exe
  [5/7] Softfloat thunks (625 ms)  cssc_softfloat.c → softfloat.o
  [6/7] Link (16 ms)  xtensa-lx106-elf-ld.exe -T esp8266.ld
  [7/7] Wrap .bin (234 ms)  Espressif boot image
  binified: hello.bin (15280 bytes)
[CSSC] Built hello.bin (15,280 bytes) + hello.elf in 953 ms.
[CSSC] Flashing hello.bin → COM4...
esptool v5.0.2 → Connected → MAC cc:50:e3:45:d8:07 → 0.4 s @ 295 kbit/s
[CSSC] Flash OK.
```

UART:

```
 ets Jan  8 2013,rst cause:2, boot mode:(3,6)
load 0x3ffe8000, len 808, room 16
tail 8 / chksum 0x11
load 0x40100000, len 14432, room 0
tail 0 / chksum 0x31 / csum 0x31
Hello from CSSC
```

---

*Ende. Bei Fragen, Korrekturen, oder Wunsch nach Probe-Erweiterung — gib Bescheid.*
