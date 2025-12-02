# Algorithms for 0-1 Knapsack

We present 10 algorithms which attempt to solve the classic 0-1 Knapsack problem.
We also present theoretical analyses of each of these algorithms.

## Build

- The repository includes a `Makefile` in this folder. Compiled executables are placed in `algorithms/bin/`.
- From the top of this folder you can run (unix/WSL):
  - `make all` to build the primary algorithms
  - `make <name>` to build a single algorithm (e.g. `make bruteforce`)

When adding a new algorithm, place the source file in a numbered subdirectory (for example `11-newalgo/newalgo.cpp`) and either add an explicit rule to the `Makefile` or rely on the generic pattern rule which compiles `subdir/subdir.cpp` to `bin/subdir`.

## How simulate.py runs algorithms

- `simulate.py` expects algorithm executables to be in `algorithms/bin/` and referenced by name in the simulator's `algorithms` dictionary. Each entry should contain the key used by the simulator and the path to the executable (absolute or relative).
- Algorithms are invoked with a single command-line argument: the path to the test case file.
- Output is captured from stdout.

## Common I/O Header: file_io.hpp

All algorithms should include the common header `file_io.hpp` which provides:

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

This function:

- Opens and reads the test case file
- Populates all fields of the `KnapsackInstance` struct
- Returns `true` on success, `false` on error (with error message to stderr)

### Usage Example

```cpp
#include "../file_io.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;  // Error already printed
    }

    // Access data directly from instance:
    // - instance.n           : number of items
    // - instance.capacity    : knapsack capacity
    // - instance.weights[i]  : weight of item i
    // - instance.values[i]   : value of item i
    // - instance.optimumValue: known optimum (-1 if unknown)

    // ... algorithm logic ...

    // Output results to stdout
}
```

## Test Case File Format

Each test case is stored as an individual `.txt` file:

```txt
Line 1: <n> <capacity> <max_weight> <min_weight> [optimum_value]
Line 2: <optimal_pick_1> <optimal_pick_2> ... (may be empty)
Lines 3+: <weight_i> <value_i>  (one per item)
```

Example:

```txt
5 100 50 10 150
0 2 4
20 60
30 80
10 40
25 70
15 50
```

## Output Format (stdout)

Algorithms must write to standard output (stdout) using the exact format below:

- **Line 1**: `max_value` — the best achievable total value (integer)
- **Line 2**: `num_selected_items` — number of items selected (integer)
- **Line 3**: `item_index_1 item_index_2 ...` — 0-based indices of selected items (space-separated)
- **Line 4**: `execution_time_microseconds` — measured execution time in μs (integer)
- **Line 5**: `memory_used_bytes` — approximate memory footprint in bytes (integer)

Notes:

- All indices are 0-based
- Each numeric output should be a plain integer with no additional text or formatting
- The order of selected item indices does not matter, but they must be unique and within `[0, n-1]`
- If `num_selected_items` is zero, line 3 may be empty
- If time/memory not measured internally, the simulator will substitute estimated values

Following these rules ensures `simulate.py` can execute, parse, and visualize results for any algorithm.
