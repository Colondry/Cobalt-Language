# Input & Output

Input and output utilities serve as the core building blocks for interactive terminal applications in Cobalt.

---

## Standard Input (`read` & `readln`)

Cobalt provides two primary operators for handling user terminal input: `read()` and `readln()`.

### 1. Single Word Input (`read`)

The `read()` function captures a single space-delimited word from standard input. It uses the stream piping operator (`>>`) to assign the input into a target string:

```cobalt
string in = ""
read("Please put something: " >> in)
```

### 2. Line Input (`readln`)

The `readln()` function reads an entire line of text, including whitespace. It supports both default line reading and optional custom termination delimiters.

```cobalt
// Standard full-line read (ends on newline)
string in1 = ""
readln("Please put some long thing: " >> in1)

// Line read with custom character delimiter
string in2 = ""
readln("Please put some long thing: " >> in2, ".")
```

#### Custom Delimiters

When a delimiter string (e.g., `"."`) is provided as the second argument to `readln()`, reading halts as soon as that character is encountered in the input stream.

---

## Standard Output (`print!` & `println!`)

Terminal output in Cobalt is handled via format macros `print!()` and `println!()`.

### `print!()`
Writes text or formatted values to standard output **without** appending a trailing newline.

### `println!()`
Writes text or formatted values to standard output and automatically appends a newline (`\n`).

```cobalt
print!("Hello, ")
println!("World!")

// Terminal Output:
// Hello, World!
```

### Format Placeholders

Both macros support brace placeholders (`{}`) to inject variables directly into output strings:

```cobalt
string lang = "Cobalt"
int version = 1

println!("Welcome to {} v{}", lang, version)