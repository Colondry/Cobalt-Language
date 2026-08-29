# Cobalt Language

Welcome to the **Cobalt Language** documentation. Cobalt is a lightweight programming language designed to transpile directly into standard C++23 output.

---

## Modules & Architecture

* **Transpiler**: Parses Cobalt source code (`.cb`), generates an Abstract Syntax Tree (AST), and transpiles statements into standard C++23 (`.cpp`).
* **Runtime (`Bin/`)**: A collection of high-performance C++ header files (`csystem.hpp`, `errors.hpp`, `cotype.hpp`, `cstr.hpp`) handling error traps, exception propagation, type unwrapping, and I/O.
* **Library (`Lib/`)**: The core library directory containing built-in Cobalt modules, standard utilities, and global definitions.

---

## Introduction: Basics

Let's do something simple, like variables.

### Variable Declarations
Variables in Cobalt are declared staticly which means you have to let the transpiler know what type you're using.
```cobalt
// Standard variable bindings
string name = "Cobalt"
int version = 1
bool is_active = true

```

### Ownership

Cobalt has ownership feature which similar to Rust and in this case, Cobalt uses `std::unique_ptr<>`.
To change ownership, Cobalt uses `$` symbol. For example:
```cobalt
int num1 = 10
int num2 = $num1 # num1 no longer has value of 10 and the num1 value ownership
                 # is moved into num2
println!(num1) # ERROR! num1 is a nullptr after loses ownership!
```
Cobalt transpiler automaticly detects if the value is moved.