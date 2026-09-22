# Changelog:

## Transpiler
- Refactored the compilation pipeline for better performance and easier extension.
- Rewrote the `if` / `elif` / `else` parser to correctly handle nested conditionals,
  chained branches, and complex conditions.
---
## New Features
- **`do <start> to <end>` loops.** Iterate over a numeric range without manual
  counter management. Equivalent to `for range(start, end)`.
  ```cobalt
  do 0 to 10 {
      println!("Hello!")
  }
---
## What changed
- **More Efficient & Flexible** → Refactored the compilation pipeline for better performance and easier extension
- **Smarter If, Elif and Else Parser** → specific about *what* it now handles
- Added inline block support to: **While**, **If elif** and **else**, **Try** and **Except**, and  **Do To**
---