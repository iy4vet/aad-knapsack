# Knapsack Algorithm Simulation

## Overview

The simulation system:

- Runs multiple knapsack algorithms on test datasets
- Collects performance metrics (time, memory, accuracy)
- Generates comprehensive visualizations
- Compares algorithm trade-offs

## Setup

### Prerequisites

- Python 3.7+
- C++ compiler (g++)
- Required Python packages (see requirements.txt)

### Installation

1. Install Python dependencies:

```bash
pip install -r requirements.txt
```

1. Compile algorithms:

```bash
cd ../algorithms
make all
```

## Usage

Run the simulation:

```bash
python simulate.py
```

## Test Case Storage

### Directory Structure

Test cases are stored as individual `.txt` files organized by dataset and category:

```txt
data/
├── knapsack_easy/              # Dataset
│   ├── ETiny/                  # Category (E = Easy prefix)
│   │   ├── 0.txt
│   │   ├── 1.txt
│   │   └── ...
│   ├── ESmall/
│   ├── EMedium/
│   ├── ELarge/
│   └── EMassive/
├── knapsack_trap/
│   ├── TTiny/                  # T = Trap prefix
│   └── ...
├── knapsack_random/
│   ├── RTiny/                  # R = Random prefix
│   └── ...
├── knapsack_hard1/
│   ├── H1known/                # H1 = Hard1 prefix
│   └── H1unknown/
└── knapsack_hard2/
    ├── H2pisingerlarge/        # H2 = Hard2 prefix
    ├── H2pisingerlowdim/
    └── H2xiang/
```

### Test File Format

Each test file contains:

- **Line 1**: `<n> <capacity> <max_weight> <min_weight> [optimum_value]`
- **Line 2**: `<optimal_pick_1> <optimal_pick_2> ...` (may be empty if unknown)
- **Lines 3+**: `<weight_i> <value_i>` (one per item)

**Note**: The simulator only reads the first 2 lines (metadata) from each test file. The algorithm executables read the full file.

## Configuration

### simulation_runs Format

```python
# Format: [dataset_name, max_parallel, [categories], timeout]
simulation_runs = [
    ["knapsack_easy", 4, ["ETiny", "ESmall"], 300],   # Specific categories
    ["knapsack_trap", 4, ["TTiny"], 300],             # Single category
    ["knapsack_hard1", 2, [], 600],                   # All categories (empty list)
]
```

- **dataset_name**: Name of dataset folder in `data/`
- **max_parallel**: Maximum parallel algorithm executions
- **categories**: List of category subdirectories to include
  - Specify categories to filter: `["ETiny", "ESmall"]`
  - Empty list `[]` runs all categories in the dataset
- **timeout**: Per-test timeout in seconds

### Category Naming Convention

| Prefix | Mode    | Example Categories                        |
|--------|---------|-------------------------------------------|
| E      | Easy    | ETiny, ESmall, EMedium, ELarge, EMassive  |
| T      | Trap    | TTiny, TSmall, TMedium, TLarge, TMassive  |
| R      | Random  | RTiny, RSmall, RMedium, RLarge, RMassive  |
| H1     | Hard1   | H1known, H1unknown                        |
| H2     | Hard2   | H2pisingerlarge, H2pisingerlowdim, H2xiang|

## Metrics Collected

### 1. Execution Time

- Measured in microseconds
- Shows algorithm scalability

### 2. Solution Quality

- Compared against optimal solution
- Accuracy percentage calculated

### 3. Memory Usage

- Approximate memory footprint
- Important for large-scale problems

### 4. Optimality Gap

- For heuristic algorithms
- Shows trade-off between speed and quality

## Visualizations Generated

### 1. Time vs Problem Size

- Log-scale plot showing execution time growth
- Identifies algorithmic complexity

### 1b. Time vs Knapsack Capacity

- Scatter plot showing execution time against knapsack capacity
- Helps understand how capacity (not just item count) affects runtime

### 2. Quality vs Time (Pareto Plot)

- Trade-off between solution quality and speed
- Helps choose best algorithm for specific needs

### 3. Accuracy Distribution

- Box plots showing solution quality consistency
- Identifies reliable algorithms

### 4. Memory Usage

- Memory consumption across problem sizes
- Important for resource-constrained environments

### 5. Optimality Rate

- Percentage of optimal solutions found
- Key metric for exact algorithms

### 6. Summary Statistics Table

- Comprehensive performance overview
- Easy comparison across algorithms

## Output Structure

```txt
simulation/
├── simulate.py           # Main simulation script
├── results/              # CSV files with raw data
│   └── results_<dataset>_<categories>_YYYYMMDD_HHMMSS.csv
└── visualizations/       # Generated plots
    └── <dataset>_<categories>_YYYYMMDD_HHMMSS/
        ├── time_vs_size.png
        ├── time_vs_capacity.png
        ├── quality_vs_time.png
        ├── accuracy_distribution.png
        ├── memory_usage.png
        ├── optimality_rate.png
        └── summary_table.png
```

## Test Categories

- **Tiny**: 20-40 items (Verify exactness)
- **Small**: 10² - 10³ items (Test DP/B&B)
- **Medium**: 10⁴ - 10⁵ items (Test Heuristics)
- **Large**: 10⁶ - 10⁷ items (Test specialized algos)
- **Massive**: 10⁸ - 10⁹ items (Extreme scale)

## Adding New Algorithms

1. Implement algorithm in C++ using `file_io.hpp` (see algorithms/README.md)
2. Add compilation rule to algorithms/Makefile
3. Update algorithms dictionary in simulate.py
4. Run simulation

## Algorithm I/O

### Input (from file via file_io.hpp)

Algorithms receive the test file path as `argv[1]` and use `loadKnapsackInstance()` to read:

- n, capacity, weights[], values[], optimumValue, etc.

### Output (to stdout)

```txt
max_value
num_selected_items
item_index_1 item_index_2 ... item_index_k
execution_time_microseconds
memory_used_bytes
```

## Notes

- All indices are 0-based
- Execution time is measured internally by the algorithm
- Memory usage is approximate
- Algorithms should timeout after 5 minutes by default
- simulate.py uses an adaptive per-test timeout heuristic (base + per-item), capped by a maximum
- Time and memory visualizations use scatter plots (unconnected dots) to avoid misleading line connections
