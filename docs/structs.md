# Structs

Structs is a custom composite types used to group related data fields under a single name. They support Cobalt's explicit borrower & pointer model (const, const_ptr, *, and &) and transpile directly to C++23 structures and smart pointers.

## Defining a Struct
Use the `struct` keyword followed by the struct name and a block of field declarations.

```cobalt
struct User {
    string name = ""
    int age = 0
}
```
** Note: Fields in Cobalt structs do not require trailing commas or semicolons. **

## Instantiation and Member Access
Instantiate a struct using field-initializer syntax. Access fields using dot notation (`.`).

```cobalt
def main(): int {
    Player p1 = Player [
        "Explorer", 
        100, 
        4.5, 
        true 
    ]

    println!("Player: {}", p1.name)
    println!("HP: {}", p1.health)

    ret 0
}
```