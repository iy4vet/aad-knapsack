# Data Directory

This directory contains test datasets for the knapsack algorithm simulator.

## Directory Structure

```txt
data/
├── generate_data.py              # Generator for easy/trap/random datasets
├── convert_hard1_datasets.py     # Converter for gh_knapsackProblemInstances
├── convert_hard2_datasets.py     # Converter for gh_knapsack-01-instances
├── knapsack_easy/                # Generated easy datasets
│   ├── ETiny/
│   ├── ESmall/
│   ├── EMedium/
│   ├── ELarge/
│   └── EMassive/
├── knapsack_trap/                # Generated trap datasets
│   ├── TTiny/
│   ├── TSmall/
│   ├── TMedium/
│   ├── TLarge/
│   └── TMassive/
├── knapsack_random/              # Generated random datasets
│   ├── RTiny/
│   ├── RSmall/
│   ├── RMedium/
│   ├── RLarge/
│   └── RMassive/
├── knapsack_hard1/               # Converted from gh_knapsackProblemInstances
│   ├── H1known/                  # Has known optima
│   └── H1unknown/                # Unknown optima
├── knapsack_hard2/               # Converted from gh_knapsack-01-instances
│   ├── H2pisingerlarge/
│   ├── H2pisingerlowdim/
│   └── H2xiang/
└── gh_*/                         # Raw source datasets (not converted)
```

## Test File Format

Each test case is stored as an individual `.txt` file:

```txt
Line 1: <n> <capacity> <max_weight> <min_weight> [optimum_value]
Line 2: <optimal_pick_1> <optimal_pick_2> ... (or empty)
Lines 3+: <weight_i> <value_i>  (one per item)
```

## Category Naming Convention

| Prefix | Mode   | Description                                      |
|--------|--------|--------------------------------------------------|
| E      | Easy   | Predictable instances for correctness tests      |
| T      | Trap   | Adversarial instances that trip greedy heuristics|
| R      | Random | Randomly generated instances                     |
| H1     | Hard1  | From gh_knapsackProblemInstances                 |
| H2     | Hard2  | From gh_knapsack-01-instances (Pisinger/Xiang)   |

## Size Categories

| Category | Item Count | Purpose                    |
|----------|------------|----------------------------|
| Tiny     | 20-40      | Verify correctness         |
| Small    | 100-1000   | Test exact algorithms      |
| Medium   | 10⁴-10⁵   | Test heuristics             |
| Large    | 10⁶-10⁷   | Test scalability            |
| Massive  | 10⁸-10⁹   | Extreme scale testing       |

## Generating Data

### Easy/Trap/Random Datasets

```bash
python generate_data.py
```

This generates `knapsack_easy/`, `knapsack_trap/`, and `knapsack_random/` with category subdirectories.

### Hard Datasets (Conversion)

```bash
python convert_hard1_datasets.py  # Creates knapsack_hard1/
python convert_hard2_datasets.py  # Creates knapsack_hard2/
```

These convert the raw `gh_*` source datasets into the standard file format.

## C++ Generators (Legacy)

The following C++ generators create instances in the old CSV format and are kept for reference:

- `generate_easy_instance.cpp` - Easy instances
- `generate_trap_instance.cpp` - Trap instances
- `generate_random_instance.cpp` - Random instances
