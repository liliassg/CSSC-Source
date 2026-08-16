# IncludeCPP Documentation

Version 4.3.0

---

## What is IncludeCPP?

IncludeCPP lets you write C++ code and use it directly in Python. You write your C++ functions and classes, run a few commands, and then import them in Python like any other module.

---

## Getting Started

### Installation

```bash
pip install IncludeCPP
```

### Create Your First Project

```bash
mkdir myproject
cd myproject
includecpp init
```

This creates three things:
- `cpp.proj` - Project settings
- `include/` - Your C++ source files go here
- `plugins/` - Generated binding files

### Write C++ Code

Create a file `include/math.cpp`:

```cpp
namespace includecpp {

int add(int a, int b) {
    return a + b;
}

class Counter {
public:
    Counter() : value(0) {}
    void increment() { value++; }
    int get() { return value; }
private:
    int value;
};

}
```

Important: All your code must be inside `namespace includecpp`. Anything outside is ignored.

### Generate Bindings

```bash
includecpp plugin math include/math.cpp
```

This creates `plugins/math.cp` with instructions for building the Python module.

### Build

```bash
includecpp rebuild
```

### Use in Python

```python
from includecpp import math

# Use function
result = math.add(5, 3)
print(result)  # 8

# Use class
counter = math.Counter()
counter.increment()
counter.increment()
print(counter.get())  # 2
```

---

## Project Structure

```
myproject/
    cpp.proj           # Project configuration
    include/           # Your C++ source files
        math.cpp
        utils.cpp
    plugins/           # Generated binding files
        math.cp
        utils.cp
```

---

## CLI Commands

### Basic Commands

| Command | Description |
|---------|-------------|
| `includecpp init` | Create new project |
| `includecpp plugin <name> <file>` | Generate bindings from C++ file |
| `includecpp rebuild` | Build all modules |
| `includecpp auto <name>` | Regenerate + rebuild in one step |
| `includecpp get <name>` | Show module API |

### Build Options

```bash
includecpp rebuild                  # Standard build
includecpp rebuild --fast           # Skip unchanged files (~0.4s)
includecpp rebuild --clean          # Full rebuild from scratch
includecpp rebuild --verbose        # Show compiler output
includecpp rebuild -m mymodule      # Build specific module only
includecpp rebuild -j 8             # Use 8 parallel jobs
```

### Build Times

| Scenario | Time |
|----------|------|
| Nothing changed (--fast) | ~0.4s |
| Source file changed | ~5-10s |
| Full rebuild | ~30s |

---

## Plugin File Format (.cp)

Plugin files define what gets exposed to Python. They're auto-generated but you can edit them.

```
SOURCE(math.cpp) math

PUBLIC(
    math CLASS(Counter) {
        CONSTRUCTOR()
        METHOD(increment)
        METHOD(get)
    }

    math FUNC(add)
)
```

### Directives

| Directive | Use |
|-----------|-----|
| `SOURCE(file) name` | Link source file to module name |
| `CLASS(Name)` | Expose a class |
| `STRUCT(Name)` | Expose a struct |
| `FUNC(name)` | Expose a function |
| `METHOD(name)` | Expose a method |
| `CONSTRUCTOR()` | Expose default constructor |
| `CONSTRUCTOR(int, string)` | Expose constructor with parameters |
| `FIELD(name)` | Expose member variable |
| `DEPENDS(mod1, mod2)` | Module dependencies |

### Overloaded Methods

When a class has multiple methods with the same name:

```
CLASS(Shape) {
    METHOD_CONST(intersects, const Circle&)
    METHOD_CONST(intersects, const Rect&)
}
```

### Templates

```
TEMPLATE_FUNC(maximum) TYPES(int, float, double)
```

Creates: `maximum_int`, `maximum_float`, `maximum_double`

---

## Configuration

### cpp.proj

```json
{
  "project": "MyProject",
  "include": "/include",
  "plugins": "/plugins",
  "compiler": {
    "standard": "c++17",
    "optimization": "O3"
  }
}
```

| Option | Description |
|--------|-------------|
| `project` | Project name |
| `include` | C++ source directory |
| `plugins` | Plugin file directory |
| `compiler.standard` | C++ standard (c++11, c++14, c++17, c++20) |
| `compiler.optimization` | Optimization level (O0, O1, O2, O3) |

---

## Using Modules in Python

### Direct Import

```python
from includecpp import math

result = math.add(1, 2)
```

### CppApi

```python
from includecpp import CppApi

api = CppApi()
math = api.include("math")
```

---

## Requirements

- Python 3.9 or newer
- C++ compiler (g++, clang++, or MSVC)
- CMake
- pybind11 (installed automatically)

---

## Troubleshooting

### "Module not found"

Run `includecpp rebuild` to compile your modules.

### Build errors

- Check your C++ code compiles normally
- Make sure code is inside `namespace includecpp`
- Run `includecpp rebuild --verbose` for details

### Changes not showing

Run `includecpp rebuild --clean` to force a full rebuild.

---

## Support

Report issues: https://github.com/liliassg/IncludeCPP/issues

```bash
includecpp bug     # Report a bug
includecpp update  # Update to latest version
```

---

# Experimental Features

The following features are experimental and may change between versions.

---

## CSSL Scripting

CSSL (C-Style Scripting Language) is an embedded scripting language included with IncludeCPP.

### Basic Usage

```python
from includecpp import CSSL

CSSL.run('''
    printl("Hello from CSSL!");

    int x = 10;
    for (i in range(0, 5)) {
        x = x + i;
    }
    printl(x);
''')
```

### Parameters and Return Values

```python
result = CSSL.run('''
    int a = parameter.get(0);
    int b = parameter.get(1);
    parameter.return(a + b);
''', 5, 3)

print(result)  # 8
```

### Sharing Python Objects

```python
from includecpp import CSSL

class Player:
    def __init__(self):
        self.health = 100

player = Player()
cssl = CSSL.CsslLang()
cssl.share(player, "player")

cssl.run('''
    $player.health = $player.health - 10;
    printl($player.health);
''')

print(player.health)  # 90 - Changed!
```

### Data Types

```cssl
int x = 42;
float pi = 3.14;
string name = "test";
bool active = true;

array<int> numbers;
numbers.push(1);
numbers.push(2);

list items = [1, 2, 3];
dict data = {"key": "value"};
```

### Control Flow

```cssl
if (x > 10) {
    printl("big");
} elif (x > 5) {
    printl("medium");
} else {
    printl("small");
}

for (i in range(0, 10)) {
    printl(i);
}

while (count < 5) {
    count = count + 1;
}
```

### Functions

```cssl
define greet(name) {
    printl("Hello, " + name + "!");
}

int add(int a, int b) {
    return a + b;
}
```

### Classes

```cssl
class Person {
    string name;
    int age;

    constr Person(string n, int a) {
        this->name = n;
        this->age = a;
    }

    void greet() {
        printl("I am " + this->name);
    }
}

person = new Person("Alice", 30);
person.greet();
```

### Payloads

Load reusable CSSL code from files:

```cssl
payload("helpers");              // Loads helpers.cssl-pl
payload("mylib", "mylib");       // Loads into namespace mylib::
```

With namespaces:
```cssl
payload("engine", "Engine");
myengine = new Engine::GameEngine();
Engine::init();
```

---

## AI Commands

OpenAI-powered code assistance (requires API key).

```bash
includecpp ai key sk-your-key
includecpp ai enable

includecpp ai ask "where is collision detection?"
includecpp ai optimize mymodule
includecpp fix --ai mymodule
```

---

## CPPY Conversion

Convert code between Python and C++.

```bash
includecpp cppy convert math.py --cpp
includecpp cppy convert utils.cpp --py
includecpp cppy convert file.py --cpp --ai
```
