# Introduction: Variables

Let's do something simple, like variables.

## Variable Declarations
Variables in Cobalt are declared staticly which means you have to let the transpiler know what type you're gonna use.

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