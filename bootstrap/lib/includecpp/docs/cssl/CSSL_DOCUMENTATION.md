# CSSL - C-Style Scripting Language

> Version 5.0.0 | A modern scripting language with C++-style syntax featuring async/await, generators, CodeInfusion, BruteInjection, Snapshots, Variable Modifiers (local/static/freezed), GUI Framework, Keyboard Framework, Python/C++ Interop, Multi-Language Support, iterate() fluent collections, is() smart truthiness, and 340+ built-in functions.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Syntax Basics](#syntax-basics)
3. [Data Types](#data-types)
4. [Variables & Scope](#variables--scope)
5. [Operators](#operators)
6. [Control Flow](#control-flow)
7. [Functions](#functions)
8. [Classes & OOP](#classes--oop)
9. [Namespaces](#namespaces)
10. [Enums & Structs](#enums--structs)
11. [Container Types](#container-types)
12. [Built-in Functions](#built-in-functions)
13. [CodeInfusion](#codeinfusion)
14. [BruteInjection](#bruteinjection)
15. [Snapshot System](#snapshot-system)
16. [C++ I/O Streams](#c-io-streams)
17. [Module System](#module-system)
18. [C++ Integration](#c-integration)
19. [Python Interop](#python-interop)
20. [Multi-Language Support](#multi-language-support)
21. [Async/Await System](#asyncawait-system-v493)
22. [Error Handling](#error-handling)
23. [CLI Reference](#cli-reference)
24. [Quick Reference](#quick-reference)

---

## Quick Start

### Installation

```bash
pip install includecpp
```

### Python Usage

```python
from includecpp import CSSL

# Execute CSSL code
CSSL.run('''
    printl("Hello CSSL!");
''')

# With parameters and return value
result = CSSL.run('''
    string name = parameter.get(0);
    printl("Hello " + name);
    parameter.return(true);
''', "World")

print(result)  # True
```

### CLI Execution

```bash
# Execute CSSL file
includecpp cssl run myfile.cssl

# Run inline code
includecpp cssl run -c 'printl("Hello!");'

# Create module from Python file
includecpp cssl makemodule mylib.py -o mylib.cssl-mod

# Format CSSL file
includecpp cssl format myfile.cssl
```

### First Script

```cssl
// Variables
string name = "CSSL";
int version = 4;

// Output
printl("Welcome to " + name + " v" + version);

// Function
void greet(string msg) {
    printl(msg);
}

greet("Hello World!");

// Class
class Greeter {
    string prefix = "Hello";

    constr(string p) {
        this->prefix = p;
    }

    void say(string name) {
        printl(this->prefix + ", " + name + "!");
    }
}

@g = new Greeter("Hi");
g.say("CSSL");
```

---

## Syntax Basics

### Comments

```cssl
// Single-line comment (C-style)
# Single-line comment (Python-style)
/* Multi-line
   comment */
```

### Semicolons

Semicolons are optional but recommended for clarity.

```cssl
printl("Hello")    // Works
printl("Hello");   // Also works (recommended)
```

### I/O Functions

```cssl
// Output
printl("Text");              // Print with newline
println("Text");             // Alias for printl
print("No newline");         // Print without newline
debug("Debug info");         // Print with [DEBUG] prefix
error("Error message");      // Print with [ERROR] prefix
warn("Warning");             // Print with [WARN] prefix
log("INFO", "Message");      // Print with custom prefix

// Input
string name = input("Enter your name: ");
int age = int(input("Enter your age: "));
string raw = input();        // Without prompt
```

### String Syntax

```cssl
// Basic strings
string s1 = "Double quoted";
string s2 = 'Single quoted';
string s3 = `Raw string (no escape processing)`;

// String interpolation
string name = "CSSL";
printl("Welcome to <name>");  // "Welcome to CSSL"

// F-strings (v4.6.3)
printl(f"Hello {name}!");
printl(f"{red('Error:')} Something went wrong");
printl(f"Result: {bold(str(42))}");

// Escape sequences
"\n"  // Newline
"\t"  // Tab
"\\"  // Backslash
"\""  // Double quote
```

---

## Data Types

### Primitive Types

| Type | Description | Example |
|------|-------------|---------|
| `int` | Integer | `int x = 42;` |
| `float` | Floating point | `float f = 3.14;` |
| `string` | Text string | `string s = "Hello";` |
| `bool` | Boolean | `bool b = true;` |
| `bit` | Binary 0 or 1 (v4.9.0) | `bit flag = 1;` |
| `byte` | 8-bit value (v4.9.0) | `byte b = 1^200;` |
| `address` | Memory reference (v4.9.0) | `address addr = memory(x).get("address");` |
| `dynamic` | Any type (flexible) | `dynamic x = "text";` |
| `void` | No return value | `void func() { }` |
| `undefined` | Function errors ignored | `undefined func() { }` |
| `null` / `None` | Absence of value | `dynamic x = null;` |
| `json` | JSON type | `json data = {"key": "value"};` |

### Boolean Values

```cssl
// Python-style
bool a = True;
bool b = False;

// C-style
bool c = true;
bool d = false;
```

### Type Conversion

```cssl
int("42");          // String to int: 42
float("3.14");      // String to float: 3.14
str(42);            // Int to string: "42"
bool(1);            // Int to bool: true
int("ff", 16);      // Hex to int: 255
list("abc");        // String to list: ['a', 'b', 'c']
dict([["a", 1]]);   // List to dict
```

### Type Checking

```cssl
typeof(42);              // "int"
typeof("hello");         // "str"
typeof([1, 2]);          // "list"
isinstance(42, "int");   // true
isint(42);               // true
isfloat(3.14);           // true
isstr("hello");          // true
isbool(true);            // true
islist([1, 2]);          // true
isdict({"a": 1});        // true
isnull(null);            // true
```

### Binary Types (v4.9.0)

**bit** - Single binary value (0 or 1):
```cssl
bit flag = 1;
printl(flag);           // 1

flag.switch();          // Toggle: 1 -> 0
printl(flag);           // 0

flag.set(1);            // Set to 1
flag.clear();           // Set to 0

bit copy = flag.copy(); // Create copy
```

**byte** - 8-bit value with x^y notation:
```cssl
// x^y notation: x = base (0/1), y = weight (0-255)
byte b = 1^200;         // Value = 200

printl(b.value());      // 200
printl(b.to_str());     // "11001000" (binary)
printl(b.info());       // Full info dict

// Bit operations
printl(b.at(7));        // Get bit 7
b.switch(7);            // Toggle bit 7
b.set(0, 1);            // Set bit 0 to 1
b.write(0, 4);          // Set bits 0-3 to 1

// Copy and reverse
byte r = b.reverse();   // Reverse bit order
byte c = b.copy();      // Copy byte
```

**address** - Memory reference (pointer-like):
```cssl
string text = "Hello";

// Get memory address
address addr = memory(text).get("address");

// Reflect to get object back
obj = addr.reflect();   // Returns the original object
// Or use builtin
obj = reflect(addr);    // Same result

printl(obj);            // "Hello"
```

---

## Variables & Scope

### Basic Variable Declaration

```cssl
// Typed variables (recommended)
int count = 0;
string name = "CSSL";
float pi = 3.14159;
bool active = true;

// Dynamic variables (flexible but slower)
dynamic value = "text";
value = 42;  // Can change type
```

### Scope Behavior

Variables are **local by default**. Each function/class has its own scope.

```cssl
define myFunction() {
    string name = "Alice";  // Local to myFunction
    printl(name);           // Works
}

myFunction();
// printl(name);  // Error! 'name' doesn't exist here
```

### Global Variables

```cssl
// Method 1: global keyword
global counter = 0;

define increment() {
    global(counter);  // Access global
    counter = counter + 1;
}

// Method 2: r@ prefix
r@globalVar = 100;

define readGlobal() {
    printl(r@globalVar);  // Access global
}

// Method 3: global modifier on function
global define myFunc() {
    // This function is globally accessible
}

// Method 4: global variable declaration
global int sharedCounter = 0;
```

### Variable References

```cssl
// Module reference (@)
@Module = include("mymodule.cssl");
@ModuleName.function();

// Global reference (r@)
r@globalVar = "shared";

// Struct self-reference (s@)
s@Backend.Loop.timer;   // Access global struct member

// Shared object reference ($)
$SharedData = {"key": "value"};
$SharedData.key;  // Access shared

// Captured reference (% for snapshots)
snapshot(myVar);
printl(%myVar);  // Access snapshotted value
```

### Variable Modifiers (v4.9.4)

Control how variables can be accessed and modified with special modifiers:

```cssl
// local - forbids global reference (@), allows snapshot (%)
local int privateCounter = 0;
@privateCounter;  // Returns null (cannot access via @)
snapshot(privateCounter);
%privateCounter;  // Works normally

// static - allows global reference (@), forbids snapshot (%)
static int cachedValue = 42;
@cachedValue;     // Works normally
snapshot(cachedValue);
%cachedValue;     // Returns null (cannot snapshot)

// local static - forbids both @ and % access
local static int internalOnly = 100;
@internalOnly;    // Returns null
%internalOnly;    // Returns null

// freezed - creates immutable variable (cannot be reassigned)
freezed string constant = "immutable";
constant = "new value";  // Returns null (cannot reassign)

// Combine modifiers
freezed local int secretConst = 999;  // Immutable and no global access
```

**Modifier Summary:**
| Modifier | `@var` (global) | `%var` (snapshot) | Reassignment |
|----------|-----------------|-------------------|--------------|
| (none)   | ✓ allowed       | ✓ allowed         | ✓ allowed    |
| `local`  | ✗ returns null  | ✓ allowed         | ✓ allowed    |
| `static` | ✓ allowed       | ✗ returns null    | ✓ allowed    |
| `local static` | ✗ returns null | ✗ returns null | ✓ allowed  |
| `freezed`| ✓ allowed       | ✓ allowed         | ✗ returns null |

---

## Operators

### Arithmetic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition | `5 + 3` → `8` |
| `-` | Subtraction | `5 - 3` → `2` |
| `*` | Multiplication | `5 * 3` → `15` |
| `/` | Division | `6 / 2` → `3` |
| `%` | Modulo | `7 % 3` → `1` |

### Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `==` | Equal to | `5 == 5` → `true` |
| `!=` | Not equal to | `5 != 3` → `true` |
| `<` | Less than | `3 < 5` → `true` |
| `>` | Greater than | `5 > 3` → `true` |
| `<=` | Less or equal | `3 <= 3` → `true` |
| `>=` | Greater or equal | `5 >= 3` → `true` |

### Logical Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `&&` / `and` | Logical AND | `true && false` → `false` |
| `\|\|` / `or` | Logical OR | `true \|\| false` → `true` |
| `!` / `not` | Logical NOT | `!true` → `false` |

### Special Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `in` | Containment | `"a" in "abc"` → `true` |
| `::` | Namespace access | `json::parse(s)` |
| `.` | Member access | `obj.method()` |
| `->` | C++ style access | `this->member` |
| `\|` | Pipe operator | `data \| filter \| process` |
| `<<` | Stream output | `cout << "text"` |
| `>>` | Stream input | `cin >> variable` |
| `~` | Destructor | `~object` |
| `&` | Reference | `&variable` |
| `++` | Append | `constr++ { }` |

### Injection Operators

| Operator | Name | Description |
|----------|------|-------------|
| `<==` | Inject Left | Replace target with source |
| `==>` | Inject Right | Right-side injection |
| `+<==` | Brute+ Left | Keep old + add new |
| `==>+` | Brute+ Right | Keep old + add new (right) |
| `-<==` | Brute- Left | Move and remove old |
| `===>-` | Brute- Right | Move and remove (right) |
| `<<==` | Infuse Left | Code infusion |
| `==>>` | Infuse Right | Code infusion (right) |
| `+<<==` | Infuse+ Left | Code copy & add |
| `==>>+` | Infuse+ Right | Code copy & add (right) |

### Flow Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `->` | Flow right | `data -> process` |
| `<-` | Flow left | `result <- compute` |

---

## Control Flow

### If/Else

```cssl
if (condition) {
    // code
} elif (other_condition) {
    // code
} else {
    // code
}

// Without parentheses
if condition {
    // code
}
```

### While Loop

```cssl
int i = 0;
while (i < 10) {
    printl(i);
    i = i + 1;
}
```

### For Loop

```cssl
// C-style for loop
for (int i = 0; i < 10; i = i + 1) {
    printl(i);
}

// Foreach with range
foreach (i in range(10)) {
    printl(i);
}

// Foreach with collection
list items = [1, 2, 3];
foreach (item in items) {
    printl(item);
}

// For-in loop
for item in items {
    printl(item);
}
```

### Switch/Case

```cssl
switch (value) {
    case 1:
        printl("One");
        break;
    case 2:
    case 3:
        printl("Two or Three");
        break;
    default:
        printl("Other");
}
```

### Break/Continue

```cssl
foreach (i in range(10)) {
    if (i == 5) {
        break;     // Exit loop
    }
    if (i % 2 == 0) {
        continue;  // Skip to next iteration
    }
    printl(i);
}
```

---

## Functions

### Basic Functions

```cssl
// Simple function
define greet() {
    printl("Hello!");
}

// Typed function with return
int add(int a, int b) {
    return a + b;
}

// Dynamic parameters
void process(dynamic data) {
    printl(typeof(data) + ": " + str(data));
}

// C++ style void function
void myFunction() {
    printl("C++ style");
}
```

### Function Modifiers

```cssl
// const - Immutable function (no side effects)
const define pureFunc() {
    return 42;
}

// private - No external injections
private define secureFunc() {
    // Protected from injection
}

// static - No instance overhead
static define staticFunc() {
    // Optimized static function
}

// global - Globally accessible
global define globalFunc() {
    // Accessible from anywhere
}

// embedded - Immediate &target replacement (v4.2.5)
embedded define override &println {
    print("[LOG] ");
    %println(args);  // Call original
}

// native - Force C++ execution (v4.6.0)
native define fastFunc() {
    // Runs in C++ interpreter
}

// unative - Force Python execution (v4.6.5)
unative define pythonFunc() {
    // Runs in Python interpreter
}

// meta - Source function (must return)
meta define mustReturn() {
    return true;
}

// closed - Protect from external injection
closed define protectedFunc() {
    // Cannot be injected from outside
}

// open - Accept any parameter type
open define flexibleFunc(open param) {
    printl(typeof(param));
}
```

### Function Appending

```cssl
// Original function
define myFunc() {
    printl("Original");
}

// Append to existing function (keeps old + adds new)
myFunc++ {
    printl("Added behavior");
}

// Calling myFunc now outputs:
// "Original"
// "Added behavior"
```

### Parameter Switch (v4.2.5)

```cssl
define processData(...params)
except {
    // Handle invalid parameters
    printl("Invalid parameters!");
}
always {
    // Always execute (like finally)
    printl("Cleanup");
}
```

---

## Classes & OOP

### Basic Class

```cssl
class Person {
    string name = "";
    int age = 0;

    // Constructor
    constr(string n, int a) {
        this->name = n;
        this->age = a;
    }

    // Method
    void introduce() {
        printl("I'm " + this->name + ", " + this->age + " years old");
    }
}

// Create instance
@p = new Person("Alice", 25);
p.introduce();
```

### Inheritance

```cssl
class Animal {
    string name = "";

    constr(string n) {
        this->name = n;
    }

    void speak() {
        printl("...");
    }
}

// C++ style inheritance (with 'extends')
class Dog extends Animal {
    constr(string n) {
        super(n);  // Call parent constructor
    }

    void speak() {
        printl(this->name + " says: Woof!");
    }

    void fetch() {
        printl(this->name + " fetches the ball!");
    }
}

@dog = new Dog("Buddy");
dog.speak();   // "Buddy says: Woof!"
dog.fetch();   // "Buddy fetches the ball!"
```

### Super Proxy (v4.8.8)

```cssl
class Parent {
    void greet() {
        printl("Hello from Parent");
    }
}

class Child extends Parent {
    void greet() {
        super->greet();  // Call parent method
        printl("Hello from Child");
    }
}

@c = new Child();
c.greet();
// Output:
// "Hello from Parent"
// "Hello from Child"
```

### Destructor (v4.8.8)

```cssl
class Resource {
    constr() {
        printl("Resource acquired");
    }

    // Destructor
    ~constr() {
        printl("Resource released");
    }
}

@r = new Resource();  // "Resource acquired"
~r;                    // "Resource released"
// Or: instance::delete(r);
```

### Constructor Modifiers (v4.8.8)

```cssl
class SafeResource {
    // secure - Constructor runs only on exception
    secure constr() {
        printl("Handling error...");
    }
}

class ManualInit {
    // callable - Constructor must be manually called
    callable constr() {
        printl("Initialized!");
    }
}

@m = new ManualInit();           // NOT initialized
instance::call_constructor(m);   // "Initialized!"
```

### Global/Embedded Classes

```cssl
// Global class - accessible everywhere
global class GlobalHelper {
    static void help() {
        printl("Help!");
    }
}

// Embedded class - auto-replaces target
embedded class @TargetClass {
    // Replaces TargetClass
}
```

---

## Namespaces

### Basic Namespace

```cssl
namespace utils {
    void log(string msg) {
        printl("[LOG] " + msg);
    }

    int add(int a, int b) {
        return a + b;
    }

    class Helper {
        void assist() {
            printl("Helping!");
        }
    }
}

// Usage
utils::log("Hello");           // "[LOG] Hello"
int sum = utils::add(1, 2);    // 3
@h = new utils::Helper();
h.assist();                    // "Helping!"
```

### Nested Namespaces

```cssl
namespace app {
    namespace core {
        namespace utils {
            void helper() {
                printl("Deep helper");
            }
        }
    }
}

app::core::utils::helper();  // "Deep helper"
```

---

## Enums & Structs

### Enums

```cssl
enum Color {
    RED = 0,
    GREEN = 1,
    BLUE = 2
}

enum Status {
    PENDING,    // 0
    ACTIVE,     // 1
    COMPLETED   // 2
}

int c = Color.RED;        // 0
int s = Status.ACTIVE;    // 1

// Embedded enum (replaces target)
embedded enum @TargetEnum {
    NEW_VALUE = 100
}
```

### Structs

```cssl
struct Point {
    x: 0,
    y: 0,
    name: "origin"
}

struct Config {
    host: "localhost",
    port: 8080,
    ssl: false
}

// Access struct
printl(Point.x);       // 0
printl(Config.host);   // "localhost"

// Global struct reference
s@Config.port = 9000;  // Modify global struct
```

---

## Container Types

### Stack

```cssl
stack<string> names;
names.push("Alice");
names.push("Bob");

string top = names.pop();    // "Bob"
string peek = names.peek();  // "Alice"
int size = names.size();     // 1
bool empty = names.empty();  // false
```

### Vector

```cssl
vector<int> nums;
nums.push_back(1);
nums.push_back(2);
nums.push_back(3);

int first = nums[0];           // 1
int size = nums.size();        // 3
nums.insert(1, 10);            // [1, 10, 2, 3]
nums.erase(2);                 // [1, 10, 3]
nums.clear();                  // []
```

### Array

```cssl
array<float> values;
values.push(1.0);
values.push(2.0);

float v = values.get(0);  // 1.0
values.set(0, 5.0);       // [5.0, 2.0]
```

### Map

```cssl
map<string, int> ages;
ages.set("Alice", 25);
ages.set("Bob", 30);

int age = ages.get("Alice");     // 25
bool has = ages.has("Charlie");  // false
list keys = ages.keys();         // ["Alice", "Bob"]
list vals = ages.values();       // [25, 30]
```

### Queue (v4.7)

```cssl
queue<int> tasks;
tasks.enqueue(1);
tasks.enqueue(2);
tasks.enqueue(3);

int first = tasks.dequeue();  // 1
int peek = tasks.peek();      // 2
int size = tasks.size();      // 2
bool empty = tasks.empty();   // false
tasks.clear();                // []
```

### DataStruct (Universal Container)

```cssl
datastruct<dynamic> data;
data.add("text");
data.add(42);
data.add(true);

// Access by index
printl(data[0]);  // "text"

// As lazy declarator
datastruct<string> lazy;
lazy.declare("key", "value");
printl(lazy.get("key"));  // "value"
```

### Iterator

```cssl
iterator<int> it = range(5);

while (it.has_next()) {
    printl(it.next());
}

// With task
iterator<int> taskIt;
taskIt.add_task(lambda x: x * 2);
taskIt.set_data([1, 2, 3]);
// Results: [2, 4, 6]
```

### List & Dict

```cssl
// Python-style list
list items = [1, 2, 3, 4, 5];
items.append(6);
items.remove(3);
int first = items[0];

// Python-style dict
dict config = {
    "host": "localhost",
    "port": 8080
};
config["ssl"] = true;
printl(config["host"]);
```

---

## Built-in Functions

### Command-Line Arguments

CSSL exposes the process arguments through three top-level builtins —
no `include` or namespace required, they work in both the Python
fallback and the C++ accelerated interpreter:

| Builtin | Returns | Notes |
|---|---|---|
| `argv()` | `list<string>` | Full `sys.argv` including the script path at index 0 |
| `argc()` | `int` | `len(argv())` |
| `parameter` | object | Inside a function body: provides positional access to the caller's args; see usage below |

```cssl
// my_tool.cssl
void main() {
    int n = argc();
    if (n < 2) {
        printl("Usage: my_tool.cssl <name> [age]");
        exit(1);
    }
    string name = argv()[1];           // [0] is the script path
    printl("Hello " + name);
    if (n >= 3) {
        int age = toint(argv()[2]);
        printl("Age: " + age);
    }
}
```

`parameter` is the function-local complement — inside any function,
it holds whatever was passed by the *caller*, regardless of declared
arity. Useful for variadic helpers:

```cssl
int sum() {
    int total = 0;
    for (int i = 0; i < parameter.count(); i++) {
        total = total + parameter.get(i);
    }
    parameter.return(total);
}

void main() {
    printl(sum(1, 2, 3, 4));   // 10
}
```

### Output Functions

```cssl
print("no newline");
printl("with newline");
println("alias for printl");
debug("debug message");    // [DEBUG] prefix
error("error message");    // [ERROR] prefix
warn("warning");           // [WARN] prefix
log("CUSTOM", "message");  // [CUSTOM] prefix
```

### String Functions

```cssl
len("hello");                    // 5
upper("hello");                  // "HELLO"
lower("HELLO");                  // "hello"
trim("  text  ");                // "text"
split("a,b,c", ",");             // ["a", "b", "c"]
join(["a", "b"], "-");           // "a-b"
replace("hello", "l", "x");      // "hexxo"
substr("hello", 1, 3);           // "ell"
contains("hello", "ell");        // true
startswith("hello", "he");       // true
endswith("hello", "lo");         // true
indexof("hello", "l");           // 2
format("Hello {}!", "World");    // "Hello World!"
repeat("ab", 3);                 // "ababab"
reverse("hello");                // "olleh"
capitalize("hello");             // "Hello"
title("hello world");            // "Hello World"
padleft("42", 5, "0");           // "00042"
padright("42", 5, "0");          // "42000"
ord("A");                        // 65
chr(65);                         // "A"
isalpha("abc");                  // true
isdigit("123");                  // true
```

### iterate() - Fluent Collection Builder

```cssl
// Range
vector<int> nums = iterate(0, 100);          // [0, 1, ..., 99]
vector<int> evens = iterate(0, 100, 2);      // [0, 2, 4, ..., 98]
vector<int> nums = iterate(min(0), max(100)); // same as iterate(0, 100)

// Wrap any value
string text = iterate(allnames["max"]["intro"]); // string stays string
vector<string> chars = iterate("hello");      // ['h','e','l','l','o']
vector<dynamic> vals = iterate(myDict);       // dict values as list

// Named params
vector<string> ch  = iterate(chars="hello"); // char list
vector<string> ks  = iterate(keys=myDict);   // dict keys
vector<dynamic> vs = iterate(values=myDict); // dict values
vector<string> rep = iterate(repeat=3, value="hi"); // ["hi","hi","hi"]
vector<float> pts  = iterate(from_=0.0, to=1.0, count=5); // linspace

// Fluent chain
vector<int> result = iterate(1, 50).filter("x % 2 == 0").map("x * x").limit(5);
string upper       = iterate("hello").map("x.upper()").join();
vector<int> top    = iterate(myList).sort(reverse=true).unique().limit(10);
auto groups        = iterate(data).group_by("x % 3");
```

**IterChain methods:**

| Method | Description |
|--------|-------------|
| `.filter("x > 0")` | Keep elements where expression is truthy |
| `.map("x * 2")` | Transform each element |
| `.sort(reverse=false, key=fn)` | Sort elements |
| `.unique()` | Remove duplicates, preserve order |
| `.reverse()` | Reverse order |
| `.limit(n)` | Keep first n elements |
| `.skip(n)` | Skip first n elements |
| `.chunk(n)` | Split into sub-lists of size n |
| `.zip_with(other)` | Zip with another collection into pairs |
| `.with_index(start=0)` | Add index: [[0,v1],[1,v2],...] |
| `.flatten()` | Flatten one level of nesting |
| `.window(size, step=1)` | Sliding window |
| `.interleave(other)` | Interleave with another collection |
| `.group_by(fn)` | Group elements into dict by key |
| `.join(sep="")` | Join as string |
| `.sum()` | Sum of numeric elements |
| `.avg()` | Average of numeric elements |
| `.min()` / `.max()` | Min/max element |
| `.count()` | Number of elements |
| `.first()` / `.last()` | First/last element |
| `.contains(v)` | True if value exists |
| `.find(fn)` | First element matching predicate |
| `.count_where(fn)` | Count matching elements |
| `.all(fn)` / `.any(fn)` | All/any match predicate |
| `.index_of(v)` | Index of value (-1 if not found) |
| `.to_list()` | Get as plain list |
| `.to_dict()` | Convert [[k,v],...] pairs to dict |

### is() - Smart Truthiness / Equality Check

```cssl
// Truthiness (single arg)
is(true)       // True        is(false)      // False
is(1)          // True        is(0)          // False
is("hello")    // True        is("")         // False
is([1, 2])     // True        is([])         // False
is(null)       // False

// Equality (two positional args)
a = 5;
is(a, 5)       // True
is(a, 6)       // False
is("hi", "hi") // True

// Custom whitelists
is(x, true=["yes", "on", 1])      // in list -> True, else False
is(x, false=["no", "off", 0])     // in list -> False, else True
is(x, true=["yes"], false=["no"]) // true first, false second, else default
is(x, true="yes")                  // single value (no list needed)
```

### List Functions

```cssl
push(list, item);           // Add to end
pop(list);                  // Remove from end
shift(list);                // Remove from start
unshift(list, item);        // Add to start
slice(list, 1, 3);          // Sublist
sort(list);                 // Sort ascending
rsort(list);                // Sort descending
unique(list);               // Remove duplicates
flatten([[1], [2, 3]]);     // [1, 2, 3]
filter(list, func);         // Filter elements
map(list, func);            // Transform elements
reduce(list, func, init);   // Reduce to single value
find(list, value);          // Find first match
findindex(list, value);     // Find index
every(list, func);          // All match?
some(list, func);           // Any match?
range(5);                   // [0, 1, 2, 3, 4]
range(1, 5);                // [1, 2, 3, 4]
range(0, 10, 2);            // [0, 2, 4, 6, 8]
enumerate(list);            // [[0, a], [1, b], ...]
zip(list1, list2);          // Pair elements
reversed(list);             // Reverse copy
sorted(list);               // Sorted copy
count(list, value);         // Count occurrences
first(list);                // First element
last(list);                 // Last element
take(list, n);              // First n elements
drop(list, n);              // All except first n
chunk(list, size);          // Split into chunks
shuffle(list);              // Randomize order
sample(list, n);            // Random n elements
```

### Dictionary Functions

```cssl
keys(dict);                     // Get keys
values(dict);                   // Get values
items(dict);                    // Get key-value pairs
haskey(dict, "key");            // Check key exists
getkey(dict, "key", default);   // Get with default
setkey(dict, "key", value);     // Set key
delkey(dict, "key");            // Delete key
merge(dict1, dict2);            // Merge dictionaries
update(dict1, dict2);           // Update in place
fromkeys(["a", "b"], 0);        // {"a": 0, "b": 0}
invert(dict);                   // Swap keys/values
pick(dict, ["key1", "key2"]);   // Select keys
omit(dict, ["key1"]);           // Exclude keys
```

### Math Functions

```cssl
abs(-5);                // 5
min(1, 2, 3);           // 1
max(1, 2, 3);           // 3
sum([1, 2, 3]);         // 6
avg([1, 2, 3]);         // 2.0
round(3.7);             // 4
floor(3.7);             // 3
ceil(3.2);              // 4
pow(2, 3);              // 8
sqrt(16);               // 4.0
mod(7, 3);              // 1
random();               // 0.0-1.0
randint(1, 10);         // 1-10
sin(0);                 // 0.0
cos(0);                 // 1.0
tan(0);                 // 0.0
asin(0);                // 0.0
acos(1);                // 0.0
atan(0);                // 0.0
atan2(1, 1);            // 0.785...
exp(1);                 // 2.718...
log(10);                // 2.302...
log10(100);             // 2.0
radians(180);           // 3.14159...
degrees(3.14159);       // ~180

// Constants
pi;                     // 3.14159...
e;                      // 2.71828...
```

### Time/Date Functions

```cssl
now();                  // Current timestamp
timestamp();            // Unix timestamp
sleep(1000);            // Sleep 1 second (ms)
delay(500);             // Delay 500ms
date();                 // Current date string
time();                 // Current time string
datetime();             // Full datetime
strftime("%Y-%m-%d");   // Format time
CurrentTime;            // Alternative current time
```

### File Functions

```cssl
pathexists("file.txt");         // Check exists
exists("file.txt");             // Alias
isfile("file.txt");             // Is file?
isdir("folder");                // Is directory?
basename("/path/file.txt");     // "file.txt"
dirname("/path/file.txt");      // "/path"
joinpath("dir", "file.txt");    // "dir/file.txt"
abspath("relative");            // Full path
normpath("a//b/../c");          // "a/c"

readfile("file.txt");           // Read entire file
writefile("file.txt", "data");  // Write file
appendfile("file.txt", "more"); // Append to file
readlines("file.txt");          // Read as lines

listdir("folder");              // List directory
makedirs("a/b/c");              // Create nested dirs
removefile("file.txt");         // Delete file
removedir("folder");            // Delete empty dir
copyfile("src", "dst");         // Copy file
movefile("src", "dst");         // Move file
rename("old", "new");           // Rename
filesize("file.txt");           // Get size in bytes
```

### JSON Functions

```cssl
tojson({"a": 1});               // '{"a": 1}'
fromjson('{"a": 1}');           // {"a": 1}
json::read("file.json");        // Read JSON file
json::write("file.json", data); // Write JSON file
json::parse(str);               // Parse JSON string
json::stringify(obj);           // Stringify object
json::pretty(obj);              // Pretty print
json::keys(obj);                // Get keys
json::values(obj);              // Get values
json::get(obj, "key");          // Get nested value
json::set(obj, "key", value);   // Set nested value
json::has(obj, "key");          // Check key exists
json::merge(obj1, obj2);        // Merge objects
```

### Hash Functions

```cssl
md5("text");            // MD5 hash
sha1("text");           // SHA-1 hash
sha256("text");         // SHA-256 hash
```

### Regex Functions

```cssl
match("hello123", "\\d+");          // "123"
search("hello123world", "\\d+");    // Match object
findall("a1b2c3", "\\d");           // ["1", "2", "3"]
sub("hello123", "\\d+", "XXX");     // "helloXXX"
```

### System Functions

```cssl
getcwd();               // Current working directory
chdir("path");          // Change directory
mkdir("folder");        // Create directory
rmdir("folder");        // Remove directory
clear();                // Clear screen
cls();                  // Alias for clear
env("HOME");            // Get env variable
setenv("VAR", "value"); // Set env variable
exit(0);                // Exit with code
platform();             // "Windows", "Linux", etc.
version();              // Python version
argv();                 // Command line args
argc();                 // Argument count
isLinux();              // true if Linux
isWindows();            // true if Windows
isMac();                // true if macOS
```

### Color Functions (v4.6.5)

```cssl
// Named colors
red("error text");
green("success text");
blue("info text");
yellow("warning text");
cyan("note text");
magenta("special text");
white("white text");
black("black text");

// Bright variants
bright_red("bright error");
bright_green("bright success");
// ... and more

// Background colors
bg_red("red background");
bg_blue("blue background");
bg_rgb(255, 200, 0, "custom bg");

// RGB color
rgb(255, 128, 0, "orange text");

// Styles
bold("bold text");
italic("italic text");
underline("underlined");
dim("dimmed text");
blink("blinking");
strikethrough("crossed out");
reset();  // Reset all styles

// In f-strings
printl(f"{red('Error:')} {bold('Something failed')}");
```

### Instance Functions

```cssl
instance::getMethods(obj);      // Get all methods
instance::getClasses(obj);      // Get all classes
instance::getVars(obj);         // Get all variables
instance::getAll(obj);          // Get all members
instance::call(obj, "method");  // Call method
instance::has(obj, "member");   // Check member exists
instance::type(obj);            // Get type name
instance::exists(obj);          // Check if valid
instance::isavailable(obj);     // Alias
instance::delete(obj);          // Call destructor (v4.8.8)
instance::call_constructor(obj);// Manual init (v4.8.8)
```

### Watcher Functions (v4.6.5)

```cssl
watcher::get(name);      // Get live Python instance value
watcher::set(name, val); // Set live Python instance value
watcher::list();         // List all watched instances
watcher::exists(name);   // Check if watcher exists
watcher::refresh(name);  // Refresh watched instance
```

---

## CodeInfusion

CodeInfusion allows injecting code into existing functions.

### Basic Code Infusion

```cssl
define original() {
    printl("Original behavior");
}

// Replace with new code (<<==)
original <<== {
    printl("New behavior");
}

original();  // "New behavior"
```

### Add to Existing (Keep Original)

```cssl
define myFunc() {
    printl("Original");
}

// Add code before original (+<<==)
myFunc +<<== {
    printl("Before");
}

myFunc();
// Output:
// "Before"
// "Original"
```

### Using Captured Values

The infused body receives the wrapped call's arguments via `parameter`
(same API as a regular function body — see "Functions"). The earlier
`args[0]` syntax in some examples reads as `null` at runtime; use
`parameter.get(i)` instead.

```cssl
define logger(string msg) {
    printl(msg);
}

// Override with access to original via %
logger <<== {
    string msg = parameter.get(0);
    printl("[LOG] " + msg);
    %logger(msg);                // Call original with the same arg
}
```

For process-level CLI arguments (the `cssl run script.cssl ...`
invocation), use the top-level builtins `argv()` / `argc()` —
they're unrelated to infusion bodies' `parameter`.

---

## BruteInjection

BruteInjection allows value/function injection with powerful filtering.

### Basic Injection

```cssl
// Replace value
target <== source;

// Right injection
source ==> target;
```

### Copy & Add

```cssl
// Keep old + add new
target +<== newValue;
```

### Move & Remove

```cssl
// Move and remove original
target -<== source;
```

### Injection Filters

```cssl
// Integer filter
result [integer::gt=5] <== numbers;     // Greater than 5
result [integer::lt=10] <== numbers;    // Less than 10
result [integer::range=1,100] <== nums; // Range 1-100

// String filter
result [string::startswith="test"] <== strings;
result [string::contains="key"] <== strings;
result [string::where="abc"] <== strings;

// JSON filter
result [json::has="key"] <== objects;
result [json::where="key=value"] <== objects;

// Type filter
result [type::filter=string] <== mixed;  // Only strings

// Custom filter
register_filter("mytype", "myhelper", func);
result [mytype::myhelper=value] <== data;
```

---

## Snapshot System

The Snapshot system (v4.8.8) allows capturing and restoring variable states.

### Basic Snapshots

```cssl
string version = "1.0";
snapshot(version);  // Capture current value

version = "2.0";    // Modify

printl(version);        // "2.0"
printl(%version);       // "1.0" (snapshotted value)
```

### Named Snapshots

```cssl
int counter = 100;
snapshot(counter, "backup");  // Custom name

counter = 500;

printl(get_snapshot("backup"));  // 100
```

### Snapshot Functions

```cssl
// Capture
snapshot(variable);              // Auto-name
snapshot(variable, "name");      // Custom name

// Retrieve
get_snapshot("name");            // Get value
%name;                           // Shorthand access

// Check
has_snapshot("name");            // true/false
list_snapshots();                // ["name1", "name2", ...]

// Manage
clear_snapshot("name");          // Delete one
clear_snapshots();               // Delete all
restore_snapshot("name");        // Restore to current

// Function snapshots
snapshot(printl);                // Capture function
embedded define override &printl {
    %printl("Prefix: " + args[0]);  // Call original
}
```

### Direct Snapshot Assignment (v4.8.9)

Assign values directly to snapshots using the `%name = value` syntax:

```cssl
// Assign variable value to snapshot
string greeting = "Hello";
%savedGreeting = greeting;       // Snapshot now holds "Hello"

// Assign literal directly
%message = "World";              // Create snapshot with literal

// Typed expression assignment
%config = (int maxRetries = 3);  // Creates variable AND snapshot
// Now 'maxRetries' = 3 and '%config' = 3

// Call snapshotted function
%savedGreeting = somefunc;
%savedGreeting();                // Call the snapshotted function
```

---

## C++ I/O Streams

CSSL supports C++ style I/O streams (v4.8.4).

### Standard Streams

```cssl
// Output
cout << "Hello";
cout << " World" << endl;

// Error output
cerr << "Error message" << endl;

// Log output
clog << "Log message" << endl;

// Input
string name;
cout << "Enter name: ";
cin >> name;

// Getline
string line = getline(cin);
```

### File Streams

```cssl
// Write to file
@file = ofstream("output.txt");
file << "Hello" << endl;
file << "World" << endl;
file.close();

// Read from file
@input = ifstream("input.txt");
string line;
while (getline(input, line)) {
    printl(line);
}
input.close();

// Read/write
@rw = fstream("data.txt", "r+");
```

### Stream Manipulators

```cssl
// Precision
cout << setprecision(2) << 3.14159 << endl;  // "3.14"

// Width and fill
cout << setw(10) << setfill('0') << 42 << endl;  // "0000000042"

// Number format
cout << fixed << 3.14159 << endl;       // Fixed point
cout << scientific << 3.14159 << endl;  // Scientific
```

---

## Module System

### Include Files

```cssl
// Include CSSL file
include "utils.cssl";

// Include as module
@utils = include("utils.cssl");
utils.helper();

// Include with alias
@lib = include("library.cssl");
lib::function();
```

### CSSL Modules (.cssl-mod)

Create modules from Python files:

```bash
includecpp cssl makemodule mylib.py -o mylib.cssl-mod
```

Use in CSSL:

```cssl
include "mylib.cssl-mod";
mylib.function();
```

### Package System

```cssl
package "myapp" {
    package-includes {
        include "lib1.cssl"
        include "lib2.cssl"
    }

    exec package {
        // Main package code
        printl("Package loaded");
    }
}
```

### Service Definition

```cssl
service:
    name: "MyService"
    version: "1.0.0"
    author: "Developer"

service-init {
    // Initialization
    printl("Service initializing...");
}

service-include {
    include "dependencies.cssl"
}

service-run {
    // Main service code
    printl("Service running");
}
```

---

## C++ Integration

### Import C++ Modules (v4.8.8)

```cssl
// Import pre-built C++ module
@math = includecpp("C:/projects/mylib/cpp.proj", "fastmath");

// Use module
result = math.fibonacci(10);      // 55
heavy = math.heavy_compute(100);  // Fast C++ execution
```

### C++ Operations

```cssl
sizeof(int);                    // Get type size
memcpy(dest, src, size);        // Memory copy
memset(buffer, value, size);    // Memory set
```

### Native/Unative Functions

```cssl
// Force C++ execution (v4.6.0)
native define fastFunction() {
    // Runs in C++ interpreter for speed
    foreach (i in range(1000000)) {
        // Fast loop
    }
}

// Force Python execution (v4.6.5)
unative define pythonFunction() {
    // Runs in Python for compatibility
    // Use for advanced CSSL features
}
```

### Memory Binding (v4.9.0)

Bind functions or classes to a memory address for deferred execution:

```cssl
// Get memory info about an object
data = memory(myObject);
addr = data.get("address");  // Memory address as hex string
type = data.get("type");     // Type name
methods = data.get("methods"); // List of methods

// Bind function to execute when host is called
define myFunc() : uses memory(hostFunction) {
    printl("Runs before hostFunction executes");
}

// When hostFunction is called, myFunc runs first, then hostFunction

// Bind class to execute constructor when host class is instantiated
class MyClass : uses memory(HostClass) {
    constr MyClass() {
        printl("Runs when HostClass constructor is called");
    }
}

// Practical example: Hook into existing function
define logCalls() : uses memory(printl) {
    // This runs every time printl is called
}
```

---

## Python Interop

### Import Python Modules

```cssl
@math = pyimport("math");
printl(math.sqrt(16));  // 4.0

@json = pyimport("json");
data = json.loads('{"key": "value"}');
```

### Python Type Conversion

```cssl
// Convert CSSL to Python
python::pythonize(csslObject);
python::wrap(csslObject);
python::export(csslObject);

// Convert Python to CSSL
python::csslize(pythonObject);
python::import(pythonObject);
```

### Parameter Exchange

```cssl
// In Python:
// CSSL.run(code, param1, param2)

// In CSSL:
val = parameter.get(0);      // Get first parameter
parameter.return(result);    // Return to Python

// Or via python namespace
val = python::param_get(0);
python::param_return(result);

// Count and check
n = python::param_count();
has = python::param_has(0);
all = python::param_all();
```

---

## Multi-Language Support

CSSL supports multi-language code blocks (v4.1.0).

### Language Blocks

```cssl
supports python {
    # Python code here
    def helper():
        return 42
}

supports javascript {
    // JS-like code
    function jsHelper() {
        return "hello";
    }
}
```

### Cross-Language Instance Access

```cssl
// Access C++ class
cpp$MyClass.method();
cpp$MyClass.property = value;

// Access Python object
py$PyObject.method();
py$PyObject.attribute;

// Access JavaScript object (if JS support enabled)
js$JsObject.function();
```

### Library Include

```cssl
// Include language-specific library
libinclude("numpy", "python");
libinclude("lodash", "javascript");
```

---

## Async/Await System (v4.9.3)

CSSL provides full async/await support for concurrent operations with generators for lazy iteration.

### Async Functions

```cssl
// Define async function with 'async' modifier
async define fetchData(string url) {
    result = http.get(url);
    return result;
}

// Calling async function returns a Future immediately
future f = fetchData("http://example.com");

// Wait for result with await
data = await f;
printl(data);
```

### Using the Async Module

```cssl
// Run any function asynchronously
future f = async.run(slowFunction, arg1, arg2);

// Wait for result
result = async.wait(f);

// Or use await keyword
result = await f;

// Cancel operation
async.stop(f);

// Async sleep (milliseconds)
async.sleep(1000);
```

### Multiple Async Operations

```cssl
// Wait for all to complete
future f1 = async.run(task1);
future f2 = async.run(task2);
future f3 = async.run(task3);

results = async.all([f1, f2, f3]);
// results = [result1, result2, result3]

// Race - first to complete wins
winner = async.race([f1, f2, f3]);
```

### Generators with Yield

```cssl
// Generator function using yield
generator<int> define Range(int n) {
    int i = 0;
    while (i < n) {
        yield i;
        i = i + 1;
    }
}

// Use generator
gen = Range(5);
while (gen.has_next()) {
    printl(gen.next());  // 0, 1, 2, 3, 4
}

// Convert to list
numbers = Range(10).to_list();
```

### Generator Methods

```cssl
gen = myGenerator();

gen.next()      // Get next value
gen.has_next()  // Check if more values
gen.send(val)   // Send value into generator
gen.to_list()   // Consume all into list
gen.take(n)     // Take up to n values
gen.skip(n)     // Skip n values
```

### Future States

```cssl
future f = async.run(myFunc);

// Check state
if (f.is_done()) {
    result = f.result();
}

// Chain callbacks
f.then(lambda result: printl("Got: " + result));

// Cancel if needed
f.cancel();
```

### Practical Example

```cssl
// Async HTTP requests in parallel
async define fetchAll(array urls) {
    futures = [];
    foreach (url in urls) {
        futures.push(async.run(http.get, url));
    }
    return async.all(futures);
}

// Use it
data = await fetchAll(["http://api1.com", "http://api2.com"]);
```

---

## Error Handling

### Try/Catch/Finally

```cssl
try {
    // Code that might fail
    result = riskyOperation();
}
catch (e) {
    // Handle error
    printl("Error: " + e);
}
finally {
    // Always executes
    cleanup();
}
```

### Throw Exceptions

```cssl
define validate(int x) {
    if (x < 0) {
        throw "Value must be positive";
    }
    return x;
}

try {
    validate(-5);
}
catch (e) {
    printl("Caught: " + e);
}
```

### Assertions

```cssl
assert(x > 0, "x must be positive");
assert(list.length > 0);  // Default message
```

---

## CLI Reference

### CSSL Commands

```bash
# Run CSSL file
includecpp cssl run myfile.cssl

# Run inline code
includecpp cssl run -c 'printl("Hello!");'

# Run with parameters
includecpp cssl run script.cssl --args "param1" "param2"

# Format CSSL file
includecpp cssl format myfile.cssl
includecpp cssl format myfile.cssl --inplace

# Create module from Python
includecpp cssl makemodule mylib.py -o mylib.cssl-mod

# Show CSSL version
includecpp cssl version

# Validate CSSL syntax
includecpp cssl check myfile.cssl
```

### C++ Build Commands

```bash
# Build all modules
includecpp rebuild

# Build specific module
includecpp rebuild --modules mymodule

# Clean build
includecpp rebuild --clean

# Verbose output
includecpp rebuild --verbose
```

---

## Quick Reference

### Variable Prefixes

| Prefix | Meaning | Example |
|--------|---------|---------|
| `@` | Module/local | `@Module = include("m.cssl")` |
| `r@` | Global | `r@globalVar = 1` |
| `s@` | Struct self | `s@Config.port` |
| `$` | Shared | `$SharedData = {}` |
| `%` | Snapshot | `%capturedVar` |

### Function Modifiers

| Modifier | Effect |
|----------|--------|
| `const` | Immutable, no side effects |
| `private` | No external injections |
| `static` | No instance overhead |
| `global` | Globally accessible |
| `native` | Force C++ execution |
| `unative` | Force Python execution |
| `embedded` | Immediate &target replacement |
| `meta` | Must return value |
| `closed` | Protected from injection |
| `open` | Accept any parameter type |
| `secure` | Constructor on exception only |
| `callable` | Manual constructor call |

### Container Quick Reference

| Type | Creation | Add | Remove | Access |
|------|----------|-----|--------|--------|
| `stack<T>` | `stack<T> s;` | `s.push(x)` | `s.pop()` | `s.peek()` |
| `vector<T>` | `vector<T> v;` | `v.push_back(x)` | `v.erase(i)` | `v[i]` |
| `array<T>` | `array<T> a;` | `a.push(x)` | `a.remove(i)` | `a.get(i)` |
| `map<K,V>` | `map<K,V> m;` | `m.set(k,v)` | `m.remove(k)` | `m.get(k)` |
| `queue<T>` | `queue<T> q;` | `q.enqueue(x)` | `q.dequeue()` | `q.peek()` |
| `list` | `list l = [];` | `l.append(x)` | `l.remove(x)` | `l[i]` |
| `dict` | `dict d = {};` | `d["k"] = v` | `del d["k"]` | `d["k"]` |

### Operator Priority (High to Low)

1. `()` `[]` `.` `->` `::`
2. `~` `!` `not`
3. `*` `/` `%`
4. `+` `-`
5. `<<` `>>`
6. `<` `<=` `>` `>=`
7. `==` `!=`
8. `&&` `and`
9. `||` `or`
10. `<==` `==>` `<<==` `==>>`
11. `=`

---

## Version History

- **v4.9.0** - Complete documentation update, includecpp() builtin, snapshot system enhancements
- **v4.8.8** - Snapshot system, super->method(), destructor support, callable/secure constructors
- **v4.8.4** - C++ I/O streams (cout, cin, fstream)
- **v4.7.0** - Thread-safe Queue type
- **v4.6.5** - Color functions, watcher namespace, unative modifier
- **v4.6.3** - F-strings support
- **v4.6.0** - Native modifier for C++ execution
- **v4.2.5** - Embedded functions, parameter switch, bytearrayed
- **v4.1.0** - Multi-language support, libinclude

---

*CSSL is part of IncludeCPP - Professional C++/Python Integration*
