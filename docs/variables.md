# Introduction: Variables & Data Types

Variables and memory ownership form the core of Cobalt's data bindings. Cobalt is statically typed, requiring explicit type declarations or `auto` inference so the transpiler can accurately emit modern C++23 types.

---

## Variable Declarations

Variables can be declared using standard primitive types, automatic type deduction, or smart pointer ownership syntax.

```cobalt
// Standard variable bindings
string name = "Cobalt"
int version = 1
bool is_active = true
auto timestamp = __Time__.Now()
```
---

## Cobalt Data Types

Cobalt maps its primitive, numeric, file, and reference types directly to standard C++23 constructs during transpilation.

| Cobalt Type | Description | C++23 Transpiled Equivalent | Example Usage |
| --- | --- | --- | --- |
| `int` | 32-bit signed integer | `int` / `std::int32_t` | `int count = 10` |
| `string` / `str` | High-level managed string type | `c_string` | `string name = "Cobalt"` |
| `bool` | Logical boolean (`true` / `false`) | `bool` | `bool is_active = true` |
| `float` / `double` | Standard floating-point values | `float` / `double` | `double ver = 1.0` |
| `uint8` | Unsigned 8-bit integer | `std::uint8_t` | `uint8 byte = 255` |
| `uint16` | Unsigned 16-bit integer | `std::uint16_t` | `uint16 port = 8080` |
| `uint32` | Unsigned 32-bit integer | `std::uint32_t` | `uint32 id = 100000` |
| `uint64` | Unsigned 64-bit integer | `std::uint64_t` | `uint64 memory = 1048576` |
| `float16` | 16-bit half-precision float | `std::float16_t` | `float16 hf = 0.5f16` |
| `float32` | 32-bit single-precision float | `std::float32_t` / `float` | `float32 f = 3.14f` |
| `float64` | 64-bit double-precision float | `std::float64_t` / `double` | `float64 d = 3.14159` |
| `float128` | 128-bit extended-precision float | `std::float128_t` | `float128 precise = 1.0` |
| `auto` | Automatic type inference | `auto` / `decltype(...)` | `auto val = 42` |
| `FILE` | Native file stream handle | `FILE*` / `fsys.hpp` handle | `FILE handle = fopen("data.txt")` |

---

## Detailed Type Specifications

* **Integral Types (`int`, `uint8`–`uint64`)**: Fixed-width integer types mapping to C++ `<cstdint>` headers for deterministic memory sizing.
* **Floating-Point Types (`float16`–`float128`)**: Precision floating-point primitives utilizing C++23 `<stdfloat>` definitions.
* **String Handling (`string`, `str`)**: Text abstractions backed by C++ standard library strings and runtime memory utilities in `cstr.hpp`.
* **Type Deduction (`auto`)**: Inferred typing resolved at compile-time, translated via `decltype(...)` during heap allocations.
* **File Handle (`FILE`)**: Low-level stream handle integrated with standard filesystem wrappers (`fsys.hpp`).

---

## Ownership Model

Cobalt features an explicit memory ownership model inspired by Rust. Under the hood, heap allocations and owned bindings are managed via C++ `std::unique_ptr<>` smart pointers.

### The Move Operator (`$`)

To transfer complete ownership of a resource from one variable to another, use the `$` symbol:

```cobalt
int num1 = 10
int num2 = $num1 // Ownership of 10 moves to num2; num1 loses ownership

println!(num1)   // ERROR! num1 is a nullptr after losing ownership!

```

### Transpiler Analysis

The Cobalt transpiler actively tracks ownership states during AST analysis:

1. When `$num1` is evaluated, value ownership transitions exclusively to `num2`.
2. `num1` is invalidated or cleared to a null pointer state in C++ output.
3. Any subsequent read of `num1` triggers a transpiler error or runtime fault.

---

## Borrow Checker

To access or read a variable's value without taking over its ownership, use borrowing.

### The Borrow Operator (`&`)

Prefixing a variable with `&` creates a non-owning reference (borrower):

```cobalt
int num1 = 10
int num2 = &num1

// Compiles cleanly: num1 retains primary ownership while num2 borrows the value
println!("{} and {}", num1, num2)

```

### Key Borrowing Principles

* **Owner Retention**: Borrowing with `&` leaves the original variable (`num1`) fully valid and active.
* **Lifetime Constraints**: Borrowed references must not outlive the owner's valid scope.

## Constants `const`
There a two type of constants, the normal constant (`const`) and the constant pointer (`const_ptr`).
### Constant
Constant is a variable that cannot be moved or changed it's ownership while it's value can still be modified. 
For example:

```cobalt
const int num = 100
int num2 = $num # error! num is a constant.
```

### Constant Pointer `const_ptr`
Pointer constant is a value that cannot me modified / read-only.
For Example:

```cobalt
const_ptr int MAX = 100
*MAX = 50 # error! MAX is a constant pointer and cannot be modified.
```