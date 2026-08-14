# Cobalt Programming Language

**Cobalt** is a modern, compiled programming language designed to combine
simple, readable syntax with the performance and flexibility of native C++.

Cobalt source code is compiled through a C++ backend, allowing programs to
take advantage of native performance and the enormous C/C++ ecosystem while
providing Cobalt with its own syntax, types, standard libraries, and tooling.

Cobalt is currently focused on becoming a strong language for **server
applications, databases, and general-purpose programming**.

---

## ⚠️ Project Status

**Current version: Cobalt Alpha v0.6.x**

Cobalt is still in active development.

The language is usable for experimentation and small programs, but it is not
yet considered production-ready.

During the Alpha stage, you should expect:

- Breaking syntax changes
- New language features
- Compiler bugs
- API changes
- Performance improvements
- Library changes
- Experimental features
- Incomplete documentation

Programs written for one Alpha release may require changes when upgrading to
a newer release.

---

## ✨ Why Cobalt?

Cobalt started as a personal project to explore **compiler development,
programming language design, and native code generation**.

The goal gradually evolved into building a language that is:

- Easy to read
- Easy to learn
- Fast enough for native applications
- Capable of interacting with native libraries
- Suitable for server-side software
- Suitable for database applications
- Flexible enough for general-purpose programming

---
# Command Basics
### **How to build/compile a file into a .exe file**
```text
cobalt build testfile.cb -as testfile
```
---
### **How to run a file**
```text
cobalt run testfile.cb
```
---
### **How to run/build a .cb file with optimized version**
```text
cobalt run testfile.cb -0Performance
```
---
### **How to install a library**
```text
cobalt install library_name
```
---
### **How to remove a library**
```text
cobalt remove libary_name
```
---
### **How to lists all library available in library path**
```text
cobalt lists
```
---
### **How to run a .cb file using an experimental interpreter**
```text
cobalt run-fast testfile.cb
```
---
## **Code Example**
### Class 
```text
class UserList {
    private {
        # Variable Declarations
        str name
        int age
    }
    public {
        def getName(): str { ret name }
    }
}
```
### Write Text
```text
def main(): int {
    print("Hello, ")
    println("World!")

    ret 0;
}
```
### Variable Declaration
```text
def main(): int {
    # variable cannot be NULL
    int age = 10
    println("Hello, ")

    ret 0;
}
```
### Repeat
```text
def main(): int {
    int h = 0
    repeat 20 {
        h++
        println(h)
    }

    ret 0;
}
```
### Forever Loop
```text
def main(): int {
    int h = 0
    forever {
        println(h)
        if h >= 10 {
            break
        }
        h++
    }

    ret 0;
}
```
---

## 🚀 Key Features

### Native C++ Backend

Cobalt programs are translated into C++ and compiled into native executables.

```text
Cobalt source
     ↓
Cobalt compiler
     ↓
Generated C++
     ↓
C++ compiler
     ↓
Native executable
