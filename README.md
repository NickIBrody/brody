# Brody Programming Language

Brody is a lightweight, fast scripting language inspired by Python, Lua, and Bash.
It is designed to be simple yet powerful — easy to learn, easy to embed, easy to extend.
Scripts use the `.br` file extension.

---

## Features

- Clean and minimal syntax
- Variables with automatic type inference
- Functions with closures and recursion
- Lists with built-in operations
- `if / elif / else` conditions
- `while` and `for` loops with `break` and `continue`
- Shell command execution via `shell`
- Module imports via `import`
- Built-in functions: math, strings, lists, type conversion
- Installs as a real system package via `dpkg`

---

## Quick Start
```brody
print("Hello, World!")

let name = input("Your name: ")
print("Hello, " + name + "!")

fn fib(n) {
    if n <= 1 { return n }
    return fib(n - 1) + fib(n - 2)
}

print("fib(10) =", fib(10))
```

---

## Installation

### Option 1 — Install from .deb package (Debian / Ubuntu)

Download the latest release and install:
```bash
dpkg -i brody_1.0.0_amd64.deb
```

After installation, `brody` is available system-wide:
```bash
brody myscript.br
```

### Option 2 — Build from source

**Requirements:**
- GCC
- Make
- libc (standard, always present)

**Steps:**
```bash
git clone https://github.com/yourname/brody-lang.git
cd brody-lang
make
./brody examples/hello.br
```

### Option 3 — Install system-wide from source
```bash
make
make install
brody examples/hello.br
```

### Option 4 — Build .deb package yourself
```bash
make deb
dpkg -i brody_1.0.0_amd64.deb
```

---

## Usage
```bash
brody script.br
```

That's it. No virtual environments, no package managers, no configuration files.

---

## Language Overview

### Variables
```brody
let x = 42
let name = "Brody"
let pi = 3.14
let flag = true
let nothing = nil
```

### Arithmetic
```brody
let a = 10 + 3      # 13
let b = 10 / 3      # 3.333
let c = 10 % 3      # 1
let d = 2 ** 8      # 256

x += 5
x -= 2
x *= 3
x /= 2
```

### Strings
```brody
let s = "hello" + " " + "world"
let line = "-" * 30
print(upper(s))
print(replace(s, "world", "Brody"))
print(len(s))
```

### Conditions
```brody
if x > 10 {
    print("big")
} elif x == 10 {
    print("exact")
} else {
    print("small")
}
```

### Loops
```brody
let i = 0
while i < 5 {
    print(i)
    i += 1
}

for item in ["a", "b", "c"] {
    print(item)
}

for i in range(1, 11) {
    print(i)
}
```

### Functions
```brody
fn add(a, b) {
    return a + b
}

print(add(3, 7))   # 10
```

Functions support closures:
```brody
fn make_multiplier(factor) {
    fn multiply(x) {
        return x * factor
    }
    return multiply
}

let double = make_multiplier(2)
print(double(5))   # 10
```

### Lists
```brody
let nums = [1, 2, 3, 4, 5]
push(nums, 6)
let last = pop(nums)
print(nums[0])
print(len(nums))
print(join(nums, ", "))
```

### Shell Commands
```brody
shell "echo Hello from bash!"
shell "mkdir -p /tmp/mydir"

let code = shell "ls /tmp"
if code == 0 {
    print("success")
}
```

### Imports
```brody
# utils.br
fn square(x) {
    return x * x
}

# main.br
import "utils"
print(square(4))   # 16
```

---

## Built-in Functions

| Function | Description | Example |
|----------|-------------|---------|
| `print(...)` | Print values to stdout | `print("x =", x)` |
| `input(prompt)` | Read line from stdin | `let s = input("name: ")` |
| `len(x)` | Length of string or list | `len("hello")` → 5 |
| `type(x)` | Type name of value | `type(42)` → `"int"` |
| `int(x)` | Convert to integer | `int("42")` → 42 |
| `float(x)` | Convert to float | `float("3.14")` → 3.14 |
| `str(x)` | Convert to string | `str(100)` → `"100"` |
| `sqrt(x)` | Square root | `sqrt(144)` → 12.0 |
| `abs(x)` | Absolute value | `abs(-7)` → 7 |
| `range(n)` | List 0..n-1 | `range(5)` → [0,1,2,3,4] |
| `range(a,b)` | List a..b-1 | `range(2,5)` → [2,3,4] |
| `range(a,b,s)` | List with step | `range(0,10,2)` → [0,2,4,6,8] |
| `push(list, x)` | Append to list | `push(a, 99)` |
| `pop(list)` | Remove and return last | `pop(a)` |
| `join(list, sep)` | Join list to string | `join(a, ", ")` |
| `split(str, sep)` | Split string to list | `split("a,b", ",")` |
| `upper(s)` | Uppercase | `upper("hi")` → `"HI"` |
| `lower(s)` | Lowercase | `lower("HI")` → `"hi"` |
| `trim(s)` | Strip whitespace | `trim("  hi  ")` → `"hi"` |
| `contains(s, sub)` | Check substring | `contains("hello", "ell")` → true |
| `replace(s, a, b)` | Replace in string | `replace("hi world", "world", "!")` |
| `exit(code)` | Exit with code | `exit(0)` |
| `shell(cmd)` | Run shell command | `shell "ls"` |

---

## Data Types

| Type | Example | Notes |
|------|---------|-------|
| `int` | `42`, `-7`, `1_000` | 64-bit integer |
| `float` | `3.14`, `-0.5` | 64-bit double |
| `string` | `"hello"`, `'world'` | UTF-8, single or double quotes |
| `bool` | `true`, `false` | |
| `nil` | `nil` | absence of value |
| `list` | `[1, 2, 3]` | dynamic, mixed types |
| `fn` | `fn name() {}` | first-class function |

---

## Project Structure
```
brody-lang/
├── src/             # C source files
│   ├── main.c
│   ├── lexer.c
│   ├── parser.c
│   ├── interp.c
│   └── ast.c
├── include/         # Header files
│   ├── common.h
│   ├── ast.h
│   ├── parser.h
│   └── interp.h
├── examples/        # Example .br scripts
├── stdlib/          # Standard library in Brody
├── tests/           # Test scripts
├── docs/            # Documentation
├── debian/          # Debian package metadata
└── Makefile
```

---

## Architecture

Brody is a **tree-walking interpreter** written in pure C with no external dependencies.
```
Source code (.br)
      │
      ▼
   Lexer          → tokens
      │
      ▼
   Parser         → AST (Abstract Syntax Tree)
      │
      ▼
  Interpreter     → executes AST nodes directly
      │
      ▼
   Output
```

- **Lexer** (`lexer.c`) — tokenizes source text
- **Parser** (`parser.c`) — builds AST via recursive descent
- **AST** (`ast.c`) — node definitions and memory management
- **Interpreter** (`interp.c`) — tree-walking evaluator with scoped environments

---

## Roadmap

- [ ] Dictionary type `{"key": "value"}`
- [ ] String interpolation `f"hello {name}"`
- [ ] Error handling `try / catch`
- [ ] REPL (interactive mode)
- [ ] Standard library in `.br` files
- [ ] More built-in string and list functions
- [ ] Windows build support

---

## License

MIT License — free to use, modify, and distribute.

---

## Author

Made with ❤️ and C.
