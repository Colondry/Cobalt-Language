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

class Entity {
    string name = ""
    string enemy = ""
    int hp = 100
    Weapon weapon = []

    def attack(Entity entity): void {
        println!("{} attacks {} with {}!", name, enemy, weapon.title)
        target.hp -= weapon.base_damage
    }
}

def main(): int {
    Weapon sword = Weapon { title: "Excalibur", base_damage: 35 }
    Entity hero = Entity { name: "Hero", hp: 100, weapon: $sword }

    Weapon club = Weapon { title: "Wooden Club", base_damage: 10 }
    Entity goblin = Entity { name: "Goblin", hp: 40, weapon: $club }

    hero.attack(&goblin)
    println!("Goblin HP: {}", goblin.hp)

    ret 0
}

```

---

### Cobalt Source

```cobalt
*BankAccount.owner = "John"
*BankAccount.is_frozen = true
*BankAccount.balance = 2500

```

### Transpiled C++ Output (`.cpp`)

```cpp
class BankAccount {
private:
    std::unique_ptr<int> balance;
    std::unique_ptr<bool> is_frozen;

public:
    std::unique_ptr<c_string> owner;

    void deposit(int amount) {
        if (*is_frozen) {
            std::cout << "Account is frozen!\n";
            return;
        }
        *balance += amount;
    }

    int get_balance() const {
        return *balance;
    }
};



```