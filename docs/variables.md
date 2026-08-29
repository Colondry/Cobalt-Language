# Introduction: Variables

Variables and memory ownership form the core of Cobalt's variable bindings[cite: 3].

## Variable Declarations

Variables in Cobalt are statically typed, requiring explicit type declarations for transpilation[cite: 3].

```cobalt
// Standard variable bindings
string name = "Cobalt"
int version = 1
bool is_active = true
```[cite: 3]

---

## Ownership

Cobalt features an explicit ownership model powered by `std::unique_ptr<>` in C++23[cite: 3]. Ownership transfer is performed using the `$` operator[cite: 3]:

```cobalt
int num1 = 10
int num2 = $num1 // Ownership of 10 moves to num2; num1 is invalidated

println!(num1)   // ERROR! num1 is now a nullptr/invalid after losing ownership
```[cite: 3]

The transpiler automatically validates ownership state during code generation[cite: 3].

---

## Borrow Checker

To access a variable's data without taking ownership, use borrowing via the `&` operator[cite: 3]:

```cobalt
int num1 = 10
int num2 = &num1

// Compiles cleanly: num1 retains ownership while num2 borrows the reference
println!("{} and {}", num1, num2)
```[cite: 2]