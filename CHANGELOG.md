# Changelog

# Cobalt v0.8.0 Beta "Azurit"

## 🐛 Fixes

- Fixed various transpiler bugs and edge cases.
- Improved type handling and variable analysis during transpilation.
- Fixed several issues that could cause incorrect or unexpected generated C++ code.

## ✨ New Features

### 🔐 Ownership & Borrow Checker

Cobalt v0.8.0 introduces an ownership and borrowing system designed to provide safer memory management and reduce unnecessary copying.

Previously, assigning one variable to another would simply create another value:

```cobalt
int i = 0
int i2 = i
```

Cobalt now provides explicit **borrowing** and **ownership transfer**:

```cobalt
int i = 9

int i2 = &i  # Borrowed i value
int i3 = $i  # Move ownership from i -> i3
```

#### Borrowing

The `&` operator creates a borrow instead of transferring ownership:

```cobalt
int value = 100
int borrowed = &value
```

The original variable remains the owner of the value.

#### Ownership

The `$` operator transfers ownership from one variable to another:

```cobalt
int i = 9
int i2 = $i
```

After the move, `i2` becomes the owner of the value.

This allows the compiler to detect invalid ownership usage and helps prevent unnecessary copies.

> ⚠️ The ownership and borrow checker is still under active development in the Azurit series. Its rules and behavior may change in future releases.

---

### 📌 Constants & Constant Pointers

#### Constants
Cobalt v0.8.0 introduces support for constants and constant pointers.

Constants are values that cannot moved or changed ownership.
For example:
```cobalt
const int MAX = 100
```

Attempting to move a constant ownership will result in a g++ compiler error:

```cobalt
const int MAX = 100

int MAX_Another = $MAX # ❌ g++ error! MAX is constant which it's ownership cannot be moved.
```
#### Constant Pointers
Cobalt also introduces constant pointers for cases where the value of a variable cannot be modified / read-only.
For Example:

```cobalt
const_ptr int value = 100
value = 200 # ❌ g++ error! constant pointer variable cannot be modified it's value / read-only.
```

---

## 🔧 Language Changes

### Module Access Syntax

Module access now uses `::` instead of the previous `< >` syntax.

```cobalt
User::GetUserData()
Math::PI
```

This removes conflicts with operators such as `>>` and makes module access more consistent with C++ and other languages.

---

## 🧪 Azurit Development Focus

Cobalt v0.8.0 begins a new stage of language development focused on:

* 🔐 Memory safety
* 📦 Ownership
* 🔗 Borrowing
* 🧠 Compile-time analysis
* ⚡ Reduced unnecessary copying
* 🛡️ Safer pointer usage
* 🧩 More predictable type behavior

**Cobalt v0.8.0 "Azurit" is an early step toward giving Cobalt stronger memory-safety guarantees while keeping the language flexible and performant.**

```
```
