# CSSC `.obj` — Anleitung zur Nutzung

Eine `.obj`-Datei ist ein **isoliertes CSSC-Programm** in einer einzigen Datei.
Sie buendelt eine kompilierte DLL, optionale Quellen und Assets. Sie ist die
empfohlene Art, einen wiederverwendbaren Baustein zu verteilen.

> Nicht zu verwechseln mit `.cobj` — letzteres ist eine **native
> Cross-Language-Bibliothek**, die zusaetzlich aus CSSL via `include()`
> eingebunden werden kann. `.obj` ist CSSC-only.

---

## 1. Wann `.obj` benutzen?

| Situation | `.obj` ist passend? |
|-----------|---------------------|
| Du willst ein abgeschlossenes CSSC-Programm + seine Helper-Funktionen verteilen | Ja |
| Mehrere Projekte sollen denselben Code wiederverwenden | Ja |
| Du willst gegen das Modul wie gegen eine Library linken | Ja, via `#depend` |
| Du willst aus CSSL drauf zugreifen | Eher `.cobj` benutzen |
| Du willst etwas in den Sprachkern einbinden | Nein — siehe Modul-Anleitung |

---

## 2. Format-Layout (zur Info)

`.obj` ist eine simple, little-endian Binaerdatei:

```
uint32  version
uint32  project_name_len
bytes   project_name
uint32  num_entries
for each entry:
    uint32  entry_name_len
    bytes   entry_name           (relativer Pfad innerhalb der .obj)
    uint64  entry_data_len
    bytes   entry_data
```

Standard-Eintraege, die die Runtime erwartet:

| Pfad | Inhalt |
|------|--------|
| `main.dll` | Die kompilierte DLL (Pflicht) |
| `manifest.txt` | `key=value`-Metadaten |
| `source/*` | Original `.cssc`-Quellen (optional) |
| `assets/*` | Beliebige Asset-Dateien |

---

## 3. Workflow — `.obj` erstellen

### 3.1 Quellskript schreiben

```cssc
// math_helpers.cssc
sector mathlib {
public:
    #define(square) {
        #scanp(_args, int, 0) x;
        return x * x;
    }
    #define(cube) {
        #scanp(_args, int, 0) x;
        return x * x * x;
    }
} free {};
```

Das Skript laedt sich nicht selbst aus; es definiert nur den Sector
`mathlib` mit zwei Funktionen.

### 3.2 Bauen & installieren

```powershell
cssc install math-helpers -u math_helpers.cssc --name mathlib
```

| Flag | Bedeutung |
|------|-----------|
| `math-helpers` | Projekt-Name (Bindestriche erlaubt) |
| `-u <script>` | Upload-Modus: kompiliert + packt ein |
| `--name <name>` | Interner Sector-/DLL-Name (Default: project_name) |
| `-f <file>` | Zusaetzliche Datei in `assets/` einbetten (mehrfach) |
| `-v` | Verbose |

Die fertige Datei landet in `%APPDATA%/cssc/objects/math-helpers.obj`.

### 3.3 Lokal exportieren (Download-Modus)

```powershell
cssc install math-helpers
```

Kopiert die installierte `.obj` in das aktuelle Verzeichnis. Praktisch, um
sie ins Projekt-Repo einzuchecken.

---

## 4. Workflow — `.obj` einbinden

```cssc
// host.cssc
#depend['./math-helpers.obj'] mh;

#stack[int, 32] x = mh::mathlib::square(7);   // 49
cssc::outln(x);

#stack[int, 32] y = mh::mathlib::cube(3);     // 27
cssc::outln(y);

#delete[x];
#delete[y];
#free[mh];
```

- `#depend['path.obj'] alias;` extrahiert die DLL beim Laden, fuehrt deren
  Init aus und macht alle exportierten Sectors unter `alias::sector::func`
  verfuegbar.
- Auf Pfade kann **relativ zum aktuellen Skript** zugegriffen werden, oder
  absolut: `#depend['/abs/path/foo.obj'] foo;`.
- Wenn die Datei **nur unter dem Namen** angegeben wird (`#depend['math-helpers']`),
  wird zusaetzlich `%APPDATA%/cssc/objects/` durchsucht.

`#free[mh]` am Ende ist Pflicht — der `.obj`-Loader haelt die DLL offen
bis dahin.

---

## 5. Workflow — `.obj` inspizieren

Die Format-Helfer in `cssc_obj_format.py` koennen interaktiv genutzt werden:

```python
from includecpp.core.cssl.native import cssc_obj_format as obj

# Inhalt auflisten
for name, size in obj.list_entries('math-helpers.obj'):
    print(f'{size:>10}  {name}')

# Quellcode rausziehen
obj.extract_entry('math-helpers.obj', 'source/math_helpers.cssc',
                  './extracted.cssc')
```

---

## 6. cssc.cproject — Projekte mit mehreren `.obj`-Dependencies

Wenn dein Projekt mehrere `.obj`-Module benutzt, lege ein `cssc.cproject`
an, damit der Compiler die Pfade zentral findet:

```
COMPILER {
    'modules' = "/dependencies"   // Hier landen alle .obj/.cobj-Files
}
```

Mit dieser Datei finden `#depend['./mh.obj']` und Konsorten ihre Dateien
im konfigurierten Unterordner, ohne harte Pfade im Code.

---

## 7. Typische Fallstricke

- **`#free[alias]` vergessen** → die DLL bleibt im Speicher bis Programmende.
  Im Interpreter ist das ok, im kompilierten Programm ein Leak.
- **Project-Name mit Sonderzeichen** ausser `-` und Buchstaben → wird vom
  CLI abgelehnt.
- **`%APPDATA%/cssc/objects/` ist persoenlich** — auf einem fremden Rechner
  muss das Modul neu installiert werden, oder die `.obj` ins Repo committed.
- **`.obj` enthaelt eine vorkompilierte DLL** — sie ist plattform- und
  Architektur-spezifisch. Cross-platform = neu bauen.
- **Inneren Namen klar waehlen** (`--name`). Wenn du spaeter zwei
  `.obj`s mit gleichem internen Sector-Namen einbindest, kollidieren sie.

---

## 8. Cheatsheet

```powershell
# Bauen + lokal installieren
cssc install my-pkg -u script.cssc --name mylib

# Mit Assets
cssc install my-pkg -u script.cssc --name mylib -f data.bin -f icon.png

# In aktuelles Verzeichnis kopieren
cssc install my-pkg

# Inhalt anzeigen
python -c "from includecpp.core.cssl.native import cssc_obj_format as o; print(o.list_entries('my-pkg.obj'))"
```

```cssc
// Im Konsumer-Skript
#depend['./my-pkg.obj'] mp;
mp::mylib::doSomething();
#free[mp];
```
