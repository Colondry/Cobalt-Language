# Introduction: Variables

Let's do something simple, like variables.

## Variable Declarations
Variables in Cobalt are staticly typed which means you have to let the transpiler know what type you're gonna use.

```cobalt
// Standard variable bindings
string name = "Cobalt"
int version = 1
bool is_active = true
```

## Ownership

Cobalt has ownership feature which similar to Rust and in this case, Cobalt uses `std::unique_ptr<>`.
To change ownership, Cobalt uses `$` symbol. For example:

```cobalt
int num1 = 10
int num2 = $num1 # num1 no longer has value of 10 and the num1 value ownership
                 # is moved into num2
println!(num1) # ERROR! num1 is a nullptr after loses ownership!
```

Cobalt transpiler automaticly detects if the value loses ownership.

## Borrow Checker

Cobalt also has a Borrow Checker which again similar to Rust. In order to use other variables value without changing ownership, the borrower is the answer.

To borrow a variable in Cobalt, you would use '&' and followed by the variable name, for example:

```cobalt
int num1 = 10
int num2 = &num1
println!("{} and {}", num1, num2) # Successfully compiled because num1 ownership stays the same
                                  # while num2 borrows the value of num1 without changing the owner
```