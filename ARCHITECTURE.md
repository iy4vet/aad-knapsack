# Knapsack Project Architecture

## System Overview

```txt
┌─────────────────────────────────────────────────────────────────┐
│                    KNAPSACK ANALYSIS SYSTEM                     │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────┐        ┌──────────────────┐        ┌──────────────────┐
│   DATA LAYER    │        │  ALGORITHM LAYER │        │  ANALYSIS LAYER  │
└─────────────────┘        └──────────────────┘        └──────────────────┘
        │                          │                            │
        ▼                          ▼                            ▼
┌─────────────────┐        ┌──────────────────┐        ┌──────────────────┐
│ Test Datasets   │───────>│ C++ Algorithms   │───────>│ Python Simulator │
│                 │        │                  │        │                  │
│ Individual TXT  │        │ ✓ Brute Force    │        │ - Run Tests      │
│ files per test  │        │ ✓ Memoization    │        │ - Collect Data   │
│                 │        │ ✓ Dynamic Prog   │        │ - Analyse        │
│ Categories:     │        │ ✓ Branch & Bound │        │ - Visualise      │
│ - ETiny,ESmall  │        │ ✓ Combo          │        │                  │
│ - TTiny,TSmall  │        │ ✓ Random Perm    │        │ simulate.py      │
│ - RTiny,RSmall  │        │ ✓ Branch & Bound │        │                  │
│ - S1known,etc   │        │ ✓ Meet in Middle │        └──────────────────┘
│                 │        │ ✓ Greedy         │                │
└─────────────────┘        │ ✓ Efficient      │                │
                           │ ✓ Billion Scale  │                ▼
                           │                  │        ┌──────────────────┐
                           │ bin/bruteforce   │        │  OUTPUT LAYER    │
                           └──────────────────┘        └──────────────────┘
                                   │                           │
                                   ▼                           ▼
                           ┌──────────────────┐        ┌──────────────────┐
                           │   BUILD SYSTEM   │        │  Results & Viz   │
                           └──────────────────┘        └──────────────────┘
                           │                  │        │                  │
                           │ Makefile         │        │ - CSV Results    │
                           │ - Compile C++    │        │ - PNG Charts     │
                           │ - Link Binaries  │        │ - Statistics     │
                           │ - Clean Build    │        │                  │
                           └──────────────────┘        └──────────────────┘
```

## Test Case Storage Paradigm

### Directory Structure

Each dataset contains category subdirectories, with individual test files:

```txt
data/
├── knapsack_easy/              # Dataset: Easy instances
│   ├── ETiny/                  # Category: Easy-Tiny (E prefix)
│   │   ├── 0.txt
│   │   ├── 1.txt
│   │   └── ...
│   ├── ESmall/                 # Category: Easy-Small
│   ├── EMedium/
│   ├── ELarge/
│   └── EMassive/
├── knapsack_trap/              # Dataset: Trap instances
│   ├── TTiny/                  # Category: Trap-Tiny (T prefix)
│   ├── TSmall/
│   └── ...
├── knapsack_random/            # Dataset: Random instances
│   ├── RTiny/                  # Category: Random-Tiny (R prefix)
│   ├── RSmall/
│   └── ...
└── knapsack_standard1/             # Dataset: Standard instances (set 1)
    ├── S1known/                # Category: Known optima
    └── S1unknown/              # Category: Unknown optima
```

### Category Naming Convention

| Prefix | Mode      | Categories                                |
|--------|-----------|-------------------------------------------|
| E      | Easy      | ETiny, ESmall, EMedium, ELarge, EMassive  |
| T      | Trap      | TTiny, TSmall, TMedium, TLarge, TMassive  |
| R      | Random    | RTiny, RSmall, RMedium, RLarge, RMassive  |
| S1     | Standard1 | S1known, S1unknown                        |

### Test File Format

Each test case is stored as an individual `.txt` file:

```txt
┌─────────────────────────────────────────────────────────────────┐
│                     Test Case File Format                       │
├─────────────────────────────────────────────────────────────────┤
│  Line 1: <n> <capacity> <max_weight> <min_weight> [optimum]     │
│  Line 2: <optimal_pick_1> <optimal_pick_2> ... (or empty)       │
│  Lines 3+: <weight_i> <value_i>  (one per item)                 │
└─────────────────────────────────────────────────────────────────┘
```

**Example file (`data/knapsack_easy/ETiny/0.txt`):**

```txt
5 100 50 10 150
0 2 4
20 60
30 80
10 40
25 70
15 50
```

- **Line 1**: Metadata - `n=5`, `capacity=100`, `max_weight=50`, `min_weight=10`, `optimum=150`
- **Line 2**: Optimal selection - items at indices 0, 2, 4 (may be empty if unknown)
- **Lines 3+**: Item data - weight and value pairs

## Common I/O Header: file_io.hpp

All C++ algorithms include `algorithms/file_io.hpp` which provides standardised file reading.

### KnapsackInstance Struct

```cpp
struct KnapsackInstance {
    size_t n;                          // Number of items
    int64 capacity;                    // Knapsack capacity
    int64 maxWeight;                   // Maximum item weight (metadata)
    int64 minWeight;                   // Minimum item weight (metadata)
    int64 optimumValue;                // Known optimum value (-1 if unknown)
    std::vector<int64> weights;        // Item weights
    std::vector<int64> values;         // Item values
    std::vector<size_t> optimalPicks;  // Known optimal selection (empty if unknown)
};
```

### loadKnapsackInstance Function

```cpp
bool loadKnapsackInstance(const std::string& filepath, KnapsackInstance& instance);
```

- **Input**: File path to test case
- **Output**: Populated `KnapsackInstance` struct
- **Returns**: `true` on success, `false` on error

### Usage Pattern

```cpp
#include "../file_io.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    // Access data via instance
    // instance.n, instance.capacity
    // instance.weights[i], instance.values[i]

    // ... algorithm logic ...
}
```

## Data Flow Diagram

```txt
┌──────────────────┐
│  Test Case File  │
│  (.txt format)   │
└────────┬─────────┘
         │
         │ argv[1] (filepath)
         ▼
┌──────────────────┐
│ C++ Algorithm    │
│                  │
│ file_io.hpp:     │
│ loadKnapsack     │
│ Instance()       │
│                  │
│ -> Reads file    │
│ -> Populates     │
│    KnapsackInst  │
│                  │
│ Process...       │
│                  │
│ stdout:          │
│  max_value       │
│  num_items       │
│  item_indices    │
│  time            │
│  memory          │
└────────┬─────────┘
         │
         │ capture output
         ▼
┌──────────────────┐
│ Python Simulator │
│                  │
│ - Read metadata  │
│   (first 2 lines │
│    ONLY)         │
│ - Parse stdout   │
│ - Calc metrics   │
│ - Store results  │
└────────┬─────────┘
         │
         │ After all tests
         ▼
┌──────────────────┐
│  Visualisation   │
│                  │
│ - matplotlib     │
│ - seaborn        │
│ - pandas         │
└────────┬─────────┘
         │
         │ Generate
         ▼
┌──────────────────┐
│ Output Files     │
│                  │
│ ✓ CSV results    │
│ ✓ PNG plots      │
│ ✓ Statistics     │
└──────────────────┘
```

## Algorithm Communication

```txt
┌────────────────────────────────────────────────────────────────┐
│                    simulate.py                                 │
│  ┌──────────────────────────────────────────────────────┐      │
│  │  subprocess.Popen(["algorithm", "path/to/test.txt"]) │      │
│  │                                                      │      │
│  │  ┌────────────────┐    ┌────────────────┐            │      │
│  │  │  Test File     │───▶│  Algorithm.exe │            │      │
│  │  │  (argv[1])     │    │  (file_io.hpp) │            │      │
│  │  └────────────────┘    └────────────────┘            │      │
│  │                               │                      │      │
│  │                        stdout (pipe)                 │      │
│  │                               ▼                      │      │
│  │                        ┌────────────────┐            │      │
│  │                        │  Parse Output  │            │      │
│  │                        │  value, items  │            │      │
│  │                        │  time, memory  │            │      │
│  │                        └────────────────┘            │      │
│  └──────────────────────────────────────────────────────┘      │
│                                                                │
│  Note: simulate.py only reads first 2 lines of test files      │
│        (metadata + optimal picks) for validation purposes      │
└────────────────────────────────────────────────────────────────┘
```

## simulation_runs Configuration

```python
# Format: [dataset_name, max_parallel, [categories], timeout]
simulation_runs = [
    ["knapsack_easy", 4, ["ETiny", "ESmall"], 300],      # Run specific categories
    ["knapsack_trap", 4, ["TTiny"], 300],                # Single category
    ["knapsack_hard1", 2, [], 600],                       # All categories (empty list)
]
```

- **dataset_name**: Name of dataset folder in `data/`
- **max_parallel**: Maximum parallel algorithm executions
- **categories**: List of category subdirectories to include (empty = all)
- **timeout**: Per-test timeout in seconds

## Technology Stack

```txt
┌─────────────────────────────────────────────┐
│              TECHNOLOGY STACK               │
└─────────────────────────────────────────────┘

Programming Languages:
  • C++ (Algorithms)        - Performance-critical code
  • Python (Simulation)     - Analysis & visualisation
  • Shell (Build)           - Automation

C++ Components:
  • Standard Library        - vectors, chrono, iostream, fstream
  • C++17 Features          - Modern syntax
  • G++ Compiler            - Optimisation flags
  • file_io.hpp             - Common I/O header

Python Libraries:
  • pandas                  - Data manipulation
  • numpy                   - Numerical operations
  • matplotlib              - Base plotting
  • seaborn                 - Statistical visualisation
  • subprocess              - Algorithm execution

Build Tools:
  • GNU Make                - Build automation
```

## Extensibility Points

### Adding New Algorithms

1. **Create Source File**

   ```sh
   algorithms/XX-name/algorithm.cpp
   ```

2. **Include file_io.hpp and Follow I/O Format**

   ```cpp
   #include "../file_io.hpp"

   int main(int argc, char* argv[]) {
       KnapsackInstance instance;
       if (!loadKnapsackInstance(argv[1], instance)) return 1;

       // Algorithm logic using instance.n, instance.capacity,
       // instance.weights, instance.values

       // Output: max_value, num_items, item_indices, time, memory
   }
   ```

3. **Update Makefile**

   ```makefile
   name: $(TARGET_DIR)/name
   $(TARGET_DIR)/name: XX-name/algorithm.cpp
       $(CXX) $(CXXFLAGS) -o $@ $<
   ```

4. **Register in Simulator**

   ```python
   self.algorithms = {
      "name": {
         "executable": <path>,
         "name": "<display name>",
         "sort_key": lambda n, w: <simplified big-O>,
      }
   }
   ```

### Adding New Datasets

1. **Create dataset directory**: `data/knapsack_<name>/`
2. **Create category subdirectories**: `data/knapsack_<name>/<Category>/`
3. **Add test files**: `data/knapsack_<name>/<Category>/<testid>.txt`
4. **Add to simulation_runs** in `simulate.py`

### Adding New Categories to Existing Datasets

1. **Create category subdirectory**: `data/<dataset>/<NewCategory>/`
2. **Add test files** following the standard format
3. **Update simulation_runs** to include new category

## Performance Considerations

### C++ Side

- Use `-O3` optimisation
- Minimise memory allocations
- Data loaded once via `loadKnapsackInstance()`
- Measure time accurately (chrono)
- Access data via `const` references where possible

### Python Side

- Only reads first 2 lines of test files (metadata)
- Does NOT load item data into Python
- Batch operations with pandas
- Generate plots once after all tests
- Save results incrementally

### File I/O Benefits

- No stdin/stdout formatting overhead for large datasets
- Memory-mapped file access for large instances
- Test files can be inspected independently
- Category-based filtering enables targeted testing
