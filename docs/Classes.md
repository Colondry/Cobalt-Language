# Classes

Classes in Cobalt provide Object-Oriented Programming (OOP) capabilities, combining data encapsulation, member methods, and single inheritance with Cobalt’s memory ownership model (`const`, `const_ptr`, `$`, and `&`).

> **Note:** Cobalt classes do not currently support custom constructors (`init`). Instance creation relies on direct field initialization or factory functions.

---

## Defining a Class

A class definition contains member variables (fields) and methods (`def`).

```cobalt
class BankAccount {
    public {
        string owner = ""
        int balance = 0
        bool is_frozen = false

        def deposit(int amount): void {
            if is_frozen {
                println!("Account is frozen!")
                ret
            }
            balance = balance + amount
        }

        def get_balance(): int {
            ret balance
        }
    }
}

```

---

## Complete Example: Game Entity System

```cobalt
struct Weapon {
    string title = ""
    int base_damage = 0
}
struct Entity {
    string name = ""
    string enemy = ""
    int hp = 100
}

class Game {
    Weapon weapon = []

    def attack(Entity entity): void {
        println!("{} attacks {} with {}!", name, enemy, weapon.title)
        target.hp -= weapon.base_damage
    }
}

def main(): int {
    Weapon sword = [ "Excalibur", 35 ]
    Entity hero =  [name: "Hero", hp: 100, weapon: $sword]

    Weapon club = [ "Wooden Club", 10 ]
    Entity goblin = [ "Goblin", 40, $club ]

    hero.attack(&goblin)
    println!("Goblin HP: {}", goblin.hp)

    ret 0
}