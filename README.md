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

**Current version: Cobalt Alpha v0.5**

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

Cobalt does not try to replace every programming language.

Instead, it aims to provide a comfortable middle ground between the
simplicity of high-level languages and the performance and control of
native languages.

---

## 🎯 Current Direction

Cobalt's development is currently centered around three major areas.

### 🖥️ Server Programming

Cobalt is being designed with server applications in mind.

Future server-oriented functionality is expected to include:

- HTTP servers
- TCP/UDP networking
- Client/server communication
- Concurrent connections
- Request handling
- Server utilities
- Configuration handling
- Logging
- Authentication-related libraries
- Database connectivity

The goal is to make writing a Cobalt server straightforward without requiring
large amounts of boilerplate.

---

### 🗄️ Database Programming

Database support is another major direction for Cobalt.

The long-term goal is to make database applications a natural use case for
the language.

Planned and experimental areas include:

- SQL database connectivity
- MySQL support
- PostgreSQL support
- SQLite support
- Connection management
- Queries
- Prepared statements
- Transactions
- Result handling
- Database utilities
- Cobalt package-based database libraries

This is especially important for Cobalt's server-side ecosystem, where
database access is often a fundamental part of an application.

---

### 🧰 General-Purpose Programming

After the server and database foundations become mature, Cobalt will expand
toward broader general-purpose programming.

Possible areas include:

- Command-line applications
- Desktop utilities
- File processing
- Networking tools
- Automation
- Data processing
- System utilities
- Application development
- Native library integration

The long-term goal is for Cobalt to be useful beyond a single niche while
maintaining its server and database strengths.

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
