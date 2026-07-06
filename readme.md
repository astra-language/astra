# 🌟 Astra Programming Language

<div align="center">

![Astra](https://img.shields.io/badge/Astra-v1.0%20Beta-brightgreen)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Language](https://img.shields.io/badge/Implementation-C%2B%2B17-red)

**A simple, expressive, and powerful programming language**

*Created by [RAJANALA VIJAY KUMAR](https://github.com/Rajanalavijaykumar)*

</div>

---

## 📖 About Astra

Astra is a custom programming language implemented in **C++17**. It compiles source code to bytecode and executes it on a custom Virtual Machine (VM). Designed to be simple yet expressive, Astra features unique concepts like **Chains** (typed collections), **Power Modules** (plugin-based extensions), and a **tri-state boolean system**.

```astra
\\ Hello World in Astra
write "Hello, World!"

\\ Tri-state boolean - unique to Astra!
x = true
y = false
z = maybe
write z     \\ Output: MAYBE
```

---

## 🤔 Why Astra?

Astra isn't trying to be the next Python overnight — it's built around one clear idea: **simple syntax that stays powerful as the community grows it.**

| | Astra | Typical Scripting Languages |
|---|---|---|
| **Indentation** | Not significant — blocks end with `;` | Often significant (easy to break by accident) |
| **Array + Struct + JSON-like data** | One concept: **Chains** | Usually 2-3 separate concepts (list, class/dataclass, dict/json import) |
| **Boolean logic** | `true`, `false`, **and `maybe`** — built-in tri-state | Binary only; uncertainty needs extra libraries |
| **Division by zero** | Returns `INFINITE`, program keeps running | Throws an exception, often crashes the script |
| **Extending the language** | Drop in a `.power` module (C++), call it like any built-in | Usually requires packaging, publishing, importing |
| **Function decoration** | **Modifiers** — apply at call site `func() [mod]`, before:/after: explicit, multiple modifiers per call | Fixed at definition (`@decorator`), wrapper function hidden |
| **Import name conflicts** | `include` auto-renames colliding functions, keeps running | Usually a hard import error |

**Real numbers from testing:** a recursive `fib(25)` (≈240,000 function calls) completes in **~165ms**, and `fib(30)` in **~1.6s**, with identical performance on Windows and Linux.

```astra
\\ One data structure for array, struct, AND json-like records
a:n = 10, 20, 30                          \\ array-style
p:n(name, age) = ("Alice", 30)            \\ struct-style — same "Chain" concept
write p1:name                             \\ Alice
```

---

## ✨ Features

- 🔥 **Simple Syntax** — Easy to learn, clean to write
- 🔗 **Chains** — Powerful typed data structures (array + struct combined)
- ⚡ **Power Modules** — Extend Astra with C++ plugins (.power files)
- 🔀 **Tri-state Boolean** — `true`, `false`, and `maybe`
- 🛡️ **Error Handling** — Structured `when/then` blocks
- 📁 **File I/O** — Built-in file operations with sandbox safety
- 🧩 **Module System** — Import other .astra files with `attach` (whole-file) or `include` (selective, conflict-safe)
- ♾️ **Safe Division** — Division by zero returns `INFINITE` (no crash!)
- 🖥️ **Cross-platform** — Windows, Linux, macOS
- 🔁 **REPL** — Interactive mode with history and arrow key support
- 🔒 **Constants** — `const` keyword for immutable variables
- 🏷️ **Alias System** — `alias` for scoped variable sharing
- 🎯 **Pattern Matching** — `check/end` for clean switch-like logic
- 🔍 **Variable Inspector** — `info` command for runtime debugging
- 📌 **Pointers** — `adr/val` for low-level memory access

---

## 🚀 Quick Start

### Build from Source

```bash
g++ -std=c++17 -O3 main.cpp lexer.cpp compiler.cpp vm.cpp parser.cpp \
    PowerManager.cpp chains.cpp error.cpp builtfun.cpp file.cpp common.cpp \
    -o astra
```

#### 🪟 Windows

**Build:**
```bash
# net.power
g++ -shared -std=c++17 -o powers/net.power netpower.cpp error.cpp -I"curl/include" -L"curl/lib" -lcurl

# db.power
gcc -c sqlite3.c -o sqlite3.o
g++ -shared -std=c++17 -o powers/db.power dbpower.cpp error.cpp sqlite3.o

# math.power, text.power, time.power, sys.power, web.power
g++ -shared -std=c++17 -o powers/math.power math_power.cpp error.cpp
g++ -shared -std=c++17 -o powers/text.power text.cpp error.cpp
g++ -shared -std=c++17 -o powers/time.power timepower.cpp error.cpp
g++ -shared -std=c++17 -o powers/sys.power system.cpp error.cpp
g++ -shared -std=c++17 -o powers/web.power webpower.cpp error.cpp
```

#### 🐧 Linux / macOS

**Build:**
```bash
# net.power
g++ -fPIC -shared -std=c++17 -o powers/net.power netpower.cpp error.cpp -lcurl

# db.power
gcc -fPIC -c sqlite3.c -o sqlite3.o
g++ -fPIC -shared -std=c++17 -o powers/db.power dbpower.cpp error.cpp sqlite3.o

# math.power, text.power, time.power, sys.power, web.power
g++ -fPIC -shared -std=c++17 -o powers/math.power math_power.cpp error.cpp
g++ -fPIC -shared -std=c++17 -o powers/text.power text.cpp error.cpp
g++ -fPIC -shared -std=c++17 -o powers/time.power timepower.cpp error.cpp
g++ -fPIC -shared -std=c++17 -o powers/sys.power system.cpp error.cpp
g++ -fPIC -shared -std=c++17 -o powers/web.power webpower.cpp error.cpp
```

### Run a File

**Windows:**
```
astra program.astra
```

**Linux / macOS:**
```
astra program.astra
```

### Interactive REPL

```
./astra
Astra VM Ready (Type 'exit' to quit)
Astra [1] >>
```

---

## 📚 Language Overview

### Variables & Constants
```astra
x = 10
name = "Astra"
pi = 3.14
flag = true

\\ Constant — cannot be reassigned
const MAX = 100
```

### Multi-Assignment
```astra
\\ Assign multiple variables at once
a, b, c = 10, 20, 30

\\ Range assignment
a--e = 1--5   \\ a=1, b=2, c=3, d=4, e=5
```

### Control Flow
```astra
score = 85
if score >= 90
    write "A Grade"
else if score >= 80
    write "B Grade"
else
    write "C Grade"
;
```

### Loops
```astra
\\ Count loop
repeat i to 5
    write i
;

\\ Custom start, end, step
repeat i to (1, 10, 2)
    write i     \\ 1 3 5 7 9
;

\\ While-style loop
x = 1
repeat (x <= 10)
    write x
    x++
;

\\ break and continue
repeat i to 10
    if i == 5
        break
    ;
    if i == 3
        continue
    ;
    write i
;
```

### Functions
```astra
#f factorial(n)
    if n <= 1
        return 1
    ;
    return n * factorial(n - 1)
#ef

write factorial(10)     \\ 3628800
```

### Modifiers
```astra
#m logRequest()
    before:
        write "Request started"
    after:
        write "Request ended"
#ef

#f processOrder()
    write "Processing order..."
#ef

processOrder() [logRequest]
```

**Output:**
Request started

Processing order...

Request ended

**Multiple modifiers — left-to-right before, right-to-left after:**
```astra
processOrder() [authenticate, logRequest, timer]
```

**Only before / only after — both sections optional:**
```astra
#m cleanup()
    before:
    after:
        write "Cleanup done"
#ef
```

**With return values:**
```astra
result = multiply(5, 6) [logger]
write result
```

> Unlike Python's `@decorator` (fixed at definition), Astra modifiers apply at **call site** — same function, different behavior per call.

### Alias System
```astra
\\ score is global, tempscore is usable inside functions
score = 10 alias tempscore

#f modify()
    tempscore = tempscore + 5   \\ local copy
    write tempscore             \\ 15
#ef

modify()
write score         \\ 10 — unchanged

```

### Chains (Data Structures)
```astra
\\ Simple chain (array-like)
a:n = 10, 20, 30, 40, 50
write a:n       \\ 10 20 30 40 50
write a2        \\ 20

\\ Update by index
a[2] = 99
write a:n       \\ 10 99 30 40 50

\\ Named-field chain (struct-like)
p:n(name, age) = ("Alice", 30), ("Bob", 25)
write p1:name   \\ Alice
write p2:age    \\ 25

\\ Chain functions
write len(a:n)      \\ 5
sort(a:n)
merge(a:n, b:n)
unique(a:n)
```

### Pattern Matching
```astra
x = 3
check x
    1: write "One"
    2: write "Two"
    3: write "Three"
    1-5: write "Between 1 and 5"
    "hello": write "It's hello"
    end: write "Default"
;
```

### Error Handling
```astra
when:
    write undefined_var
then UNDEFINED_VAR:
    write "Variable not found!"
then DIVISION_BY_ZERO:
    write "Cannot divide by zero!"
then:
    write "Some other error occurred"
;
```

### Attach (Module Import)
```astra
\\ Import another .astra file
attach "utils.astra"
greet("World")

\\ Import with alias to avoid conflicts
attach "math_utils.astra" as mu
write mu.add(10, 20)
```

### Include (Selective Module Import)

Unlike `attach`, which imports an entire file and raises a compile error on name
conflicts, `include` is designed for safe, repeated, selective importing.

```astra
\\ Include everything from a file
include "utils.astra"

\\ Include only specific functions/variables
include "utils.astra" only greet, farewell

\\ Include everything except specific names
include "utils.astra" except internalHelper
```

**How conflicts are handled:**
Unlike `attach`, `include` never raises a compile error for name conflicts.
If a function name has already been included (from this or another `include`),
Astra automatically renames the new one (`greet` → `greet_2`, `greet_3`, ...)
and prints an info message telling you the new name. This means `include` is
safe to use repeatedly without careful alias management, but you should watch
the console output to see if a function was silently renamed.

> ⚠️ Only **function** name collisions are auto-renamed. Chains and plain
> variables included via `include` do not get renamed — a colliding chain/
> variable name will simply overwrite or coexist based on normal variable
> rules, so avoid re-including files that share chain/variable names.

**Re-including the same file:**
`include` tracks each `(file, mode, name-list)` combination separately, so:
```astra
include "utils.astra" only greet
include "utils.astra" only farewell   \\ different selector — both run
include "utils.astra" only greet      \\ same selector as before — skipped, info message shown
```


### Power Modules
```astra
add math
write sqrt(16)      \\ 4
write pow(2, 10)    \\ 1024
write fib(10)       \\ 55

add text
write upper("hello")    \\ HELLO
write reverse("Astra")  \\ artsA
```

### File Operations
```astra
\\ Create and write
f = create("output.txt")
plus(f, "Hello Astra", "la")   \\ append line
plus(f, "First line", "fa")    \\ prepend line
close(f)

\\ Read line by line
f = create("output.txt")
repeat (eof(f) == false)
    line = read(f, "l")
    write line
;
close(f)

\\ Read all at once
f = create("output.txt")
content = read(f, "t")
write content
close(f)

\\ Fetch specific content
f = create("data.txt")
line3 = fetch(f, 3, "l")       \\ line 3
part  = fetch(f, 2, 10, "lc")   \\ line 2, first 10 chars
close(f)
```

### JSON Support
```astra
\\ Parse JSON into a chain
parseJson("data", "[{\"name\":\"Alice\",\"age\":30}]")
write data1:name    \\ Alice

\\ Convert chain to JSON
p:n(name, age) = ("Bob", 25)
json = toJson(p:n)
write json
```

### Pointers
```astra
x = 42
ptr = adr(x)        \\ get address of x
write val(ptr)      \\ 42 — dereference

adr(x, 99)          \\ set value via pointer
write x             \\ 99
```

### Variable Inspector
```astra
x = 42
info x          \\ shows type, value, memory address

a:n = 10, 20, 30
info a:n        \\ shows chain details
```

### String Comparison
Astra compares strings using ASCII values:
```astra
write "hello" > "abc"   \\ TRUE
write "abc" < "xyz"     \\ TRUE
write "abc" == "abc"    \\ TRUE

\\ String vs Number — number converts to string automatically
write "hello" > 10      \\ TRUE ('h' > '1' in ASCII)
write "9" > "10"        \\ TRUE ('9' > '1' in ASCII)
```
> ⚠️ Note: When comparing string with number, number converts to string automatically.

### User Input
```astra
name = user: Enter your name: 
write "Hello " + name

age = user: Enter your age: 
write age + 1
```

---

## 🏗️ Architecture

```
Source Code (.astra)
        ↓
    [Lexer]          Tokenization
        ↓
    [Parser]         Tokens → AST
        ↓
    [Compiler]       AST → Bytecode
        ↓
    [AstraVM]        Bytecode Execution
```

---

## 📦 Power Modules

| Module | Description |
|--------|-------------|
| `math.power` | sqrt, pow, sin, cos, rand, fib, factorial, and more |
| `text.power` | upper, lower, trim, replace, split, reverse, and more |
| `net.power` | get, post, status, download — HTTP/network requests |
| `db.power` | open, query, fetch, count, drop, escape, bulkinsert — SQLite database |
| `time.power` | time, date, sleep, diff, timestamp, stopwatch, format, addtime |
| `sys.power` | os, cpu, ram, cwd, mkdir, run, env, hostname, username, uptime |
| `web.power` | Generate HTML pages from Astra code |


### Creating a Power Module

```cpp
// mymodule.cpp
#include "astra_sdk.h"

void my_func(AstraVM* vm) {
    // pop args, push result
}

extern "C" ASTRA_API void astra_init(RegisterFunc reg) {
    reg("my_func", my_func);
}
```

```bash
g++ -shared -fPIC mymodule.cpp -o powers/mymodule.power
```

```astra
add mymodule
write my_func()
```

---

## 📁 File Extensions

| Extension | Description |
|-----------|-------------|
| `.astra` | Astra source code file |
| `.power` | Power Module (C++ plugin) |

---

## 🔑 Keywords Reference

| Keyword | Purpose |
|---------|---------|
| `write` / `writes` | Print to console |
| `if` / `else` | Conditional execution |
| `repeat` / `to` | Loop |
| `break` / `continue` | Loop control |
| `#f` / `#ef` | Function definition |
| `#m` / `#ef` | Modifier definition |
| `return` | Return value |
| `const` | Declare constant |
| `alias` | Scoped variable sharing |
| `when` / `then` | Error handling |
| `check` / `end` | Pattern matching |
| `attach` / `as` | Import .astra module |
| `include` / `only` / `except` | Selective, conflict-safe module import |
| `add` | Load power module |
| `true` / `false` / `maybe` | Boolean literals |
| `user:` | Read user input |
| `info` | Variable/chain inspector |
| `adr` / `val` | Pointer operations |
| `cls` / `clear` | Clear console |

---

## 📋 Error Codes

| Error Code | Description |
|------------|-------------|
| `UNDEFINED_VAR` | Variable used without being defined |
| `UNINITIALIZED_VAR` | Variable declared but not assigned |
| `DIVISION_BY_ZERO` | Division or modulo by zero |
| `FUNC_NOT_FOUND` | Function call to undefined function |
| `FIELD_NOT_FOUND` | Chain field not found |
| `CHAIN_NOT_FOUND` | Chain variable not found |
| `INVALID_OPERATION` | Invalid type operation |
| `TYPE_MISMATCH` | Incompatible data types |
| `ALIAS_REQUIRED` | Function needs alias prefix |

---

## 🛠️ Editor Support

A VS Code extension provides syntax highlighting and a one-click **Run** button for `.astra` files. Download the extension and the Windows installer (`astra.exe`, which adds `astra` to your system PATH automatically) from the [Releases](../../releases) page.

---

## 📜 License

This project is licensed under the **MIT License**.

Copyright (c) 2026 **RAJANALA VIJAY KUMAR**

See the [LICENSE](LICENSE) file for details.

---

## 👨‍💻 Author

**RAJANALA VIJAY KUMAR**
- GitHub: [@Rajanalavijaykumar](https://github.com/Rajanalavijaykumar)
- Organization: [astra-language](https://github.com/astra-language)

---

<div align="center">

⭐ If you like Astra, please give it a star!

*Astra Programming Language — Simple. Expressive. Powerful.*

</div>
