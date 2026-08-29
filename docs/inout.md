# Input & Output

Input and Output is litterally the basics & foumdations of programming language.
So let's learn about it in Cobalt!

## Input
To get user input in cobalt, you can either use `read()` or `readln()`.
- `read()` is just reading one word that user puts. For example:

```cobalt
string in = ""
read("Please put something: " >> in)
```

- While `readln()` reads an entire line and you can put a delimiter.
For example:

```cobalt
# No delimiter
string in1 = ""
readln("Please put some long thing: " >> in1)

# With delimiter
string in2 = ""
readln("Please put some long thing: " >> in2, ".")
```

## Output
To prints a text into the terminal, we use `print!()` and `println!()` to outputs text into the terminal.
- `print!()` is printing your text into the terminal without a newline.
- While `println!()` is printing your text into the terminal with a newline.
For Example:
```cobalt
print!("Hello, ")
println!("World!") 
# prints 'Hello, World!'
```