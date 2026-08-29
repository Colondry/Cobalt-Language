# Cobalt Language

Welcome to the **Cobalt Language** documentation. Cobalt is a lightweight programming language designed to transpile directly into standard C++23 output.

---

## Modules & Architecture

* **Transpiler**: Parses Cobalt source code (`.cb`), generates an Abstract Syntax Tree (AST), and transpiles statements into standard C++23 (`.cpp`).
* **Runtime (`Bin/`)**: A collection of high-performance C++ header files (`csystem.hpp`, `errors.hpp`, `cotype.hpp`, `cstr.hpp`) handling error traps, exception propagation, type unwrapping, and I/O.
* **Library (`Lib/`)**: The core library directory containing built-in Cobalt modules, standard utilities, and global definitions.

---
## Map:
- `Introduction: Variables` : `https://cobalt-language.readthedocs.io/en/latest/variables.html`
- `Input & Output`          : `https://cobalt-language.readthedocs.io/en/latest/inout.html`