# Classes

Classes in Cobalt provide Object-Oriented Programming (OOP) capabilities, combining data encapsulation, member methods, and single inheritance with Cobalt’s memory ownership model (`const`, `const_ptr`, `$`, and `&`).

> **Note:** Cobalt classes do not currently support custom constructors (`init`). Instance creation relies on direct field initialization or factory functions.

---

## Defining a Class

A class definition contains member variables (fields) and methods (`def`).

```cobalt
class BankAccount {
    pub string owner
    int balance
    bool is_frozen

    pub def deposit(amount: int): void {
        if self.is_frozen {
            println!("Account is frozen!")
            ret
        }
        self.balance = self.balance + amount
    }

    pub def get_balance(): int {
        ret self.balance
    }
}

```

---

## Instantiation and Factory Patterns

Since constructors are not supported yet, instances are constructed using struct-style member initialization or by defining standalone factory functions.

### Field-Assignment Instantiation

```cobalt
BankAccount acc = BankAccount {
    owner: "Alice",
    balance: 500,
    is_frozen: false
}

```

### Factory Function Pattern

To encapsulate complex setup logic, use factory functions that construct and return owned instances:

```cobalt
def create_account(string owner_name, int initial_deposit): BankAccount {
    BankAccount acc = [
        owner_name,
        initial_deposit,
        false
    ]
    ret $acc // Transfer ownership to caller
}

```

---

## Ownership & Mutability Matrix

Class instances follow Cobalt's standard memory ownership rules:

| Binding Declaration | Can Modify Fields / Call Mutating Methods? | Can Move Ownership (`$`)? |
| --- | --- | --- |
| `Player p` | ✅ Yes | ✅ Yes |
| `const Player p` | ✅ Yes | ❌ No |
| `const_ptr Player p` | ❌ No (Read-Only) | ✅ Yes |
| `const const_ptr Player p` | ❌ No (Read-Only) | ❌ No |

### Ownership Transfers & Borrowing

```cobalt
@import <base>

def process_player(p: &Player): void {
    println!("Processing player: {}", p.name)
}

def archive_player(p: Player): void {
    // Takes full ownership of the instance
}

def main(): int {
    Player hero = Player { name: "Aragorn", hp: 100 }

    // Borrow hero
    process_player(&hero)

    // Move hero ownership
    archive_player($hero)

    ret 0
}

```

---

## Inheritance and Overriding

Classes support single inheritance using the `:` syntax. Derived classes inherit all public fields and methods from the base class.

* `virtual`: Place before a base class method to allow child classes to override it.
* `override`: Place before a derived class method to explicitly override a base implementation.

```cobalt
class Character {
    pub string name
    pub int hp

    pub virtual def take_damage(amount: int): void {
        self.hp = self.hp - amount
        println!("{} took {} damage.", self.name, amount)
    }
}

class Boss : Character {
    pub int armor

    pub override def take_damage(amount: int): void {
        int actual_damage = amount - self.armor
        if actual_damage < 0 { actual_damage = 0 }
        
        self.hp = self.hp - actual_damage
        println!("Boss {} absorbed damage! HP lost: {}", self.name, actual_damage)
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

## C++ Transpilation Mapping

Cobalt transpiles classes directly into C++23 classes managed via `std::unique_ptr`.

### Cobalt Source

```cobalt
*BackAccount.owner = "John"

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