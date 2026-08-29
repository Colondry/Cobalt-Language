# Input & Output

Input and output utilities serve as the basic foundations for interactive programs in Cobalt[cite: 2].

---

## Input

User input is captured using `read()` or `readln()`[cite: 2].

* **`read()`**: Reads a single word into a string target[cite: 2].

```cobalt
string in = ""
read("Please enter something: " >> in)
```[cite: 2]

* **`readln()`**: Reads an entire line with optional delimiter support[cite: 2].

```cobalt
// Standard line read
string in1 = ""
readln("Enter full line: " >> in1)

// Read until custom delimiter (e.g., period)
string in2 = ""
readln("Enter sentence: " >> in2, ".")
```[cite: 2]

---

## Output

Terminal output is managed via the `print!()` and `println!()` macros[cite: 2].

* **`print!()`**: Prints output without appending a newline[cite: 2].
* **`println!()`**: Prints output followed by a newline[cite: 2].

```cobalt
print!("Hello, ")
println!("World!")
// Output: Hello, World!
```[cite: 2]