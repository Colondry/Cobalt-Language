# Cobalt Language

Welcome to the **Cobalt Language** documentation[cite: 1]. Cobalt is a lightweight programming language designed to transpile directly into standard C++23 output[cite: 1].

---

## Modules & Architecture

* **Transpiler**: Parses Cobalt source code (`.cb`), generates an Abstract Syntax Tree (AST), and transpiles statements into standard C++23 (`.cpp`)[cite: 1].
* **Runtime (`Bin/`)**: High-performance C++ header files (`csystem.hpp`, `errors.hpp`, `cotype.hpp`, `cstr.hpp`) handling error traps, exception propagation, type unwrapping, and I/O[cite: 1].
* **Library (`Lib/`)**: Core library directory containing built-in Cobalt modules, standard utilities, and global definitions[cite: 1].

---

```{toctree}
:maxdepth: 2
:caption: Documentation Contents

variables
inout