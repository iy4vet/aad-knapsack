# COMBO Algorithm

## Overview

The COMBO algorithm is a highly efficient exact algorithm for the 0-1 Knapsack Problem, developed by S. Martello, D. Pisinger, and P. Toth. It combines dynamic programming with several optimization techniques to achieve state-of-the-art performance.

## Reference

> S. Martello, D. Pisinger, P. Toth: "Dynamic Programming and Strong Bounds for the 0-1 Knapsack Problem", Management Science (1999)

## Key Techniques

### 1. Core Concept

Instead of processing all items, COMBO focuses on a "core" of items around the break item (the item where the greedy solution exceeds capacity). Items far from the core are unlikely to change the optimal solution.

### 2. Dantzig Upper Bound

Uses the LP relaxation bound: include all items up to capacity, then fractionally include the break item.

### 3. Efficient State Space Management

- States are stored sorted by weight sum
- Dominated states are pruned
- Binary solution vectors track which items are included

### 4. Rudimentary Divisibility

Reduces the effective capacity using GCD-based analysis when beneficial.

### 5. Partitioning and Sorting

Items are partitioned by their efficiency ratio relative to the break item, enabling efficient expansion of the core.

## Time Complexity

- Best case: O(n log n) when bounds are tight
- Average case: O(n × core_size) where core_size << n typically
- Worst case: O(n × capacity) for adversarial instances

## Space Complexity

- O(MAXSTATES) for the state space (configurable, default 1.5M states)
- O(n) for item storage

## Usage

```bash
./combo <input_file>
```

## Output Format

```txt
<max_value>
<num_selected_items>
<space-separated indices of selected items>
<execution_time_microseconds>
<memory_used_bytes>
```

## Parameters

The algorithm uses several internal parameters that can be tuned:

- `MAXSTATES`: Maximum number of DP states (default: 1,500,000)
- `MINRUDI`: Minimum items for rudimentary divisibility (default: 1,000)
- `MINHEUR`: Minimum items for heuristic initialization (default: 10,000)

## Strengths

- Exact algorithm (always finds optimal solution)
- Very fast for typical instances
- Memory-efficient through state pruning
- Handles large instances well

## Limitations

- May be slower than heuristics for very large instances where exactness isn't required
- Memory usage can spike for hard instances
- Performance degrades on specially crafted adversarial instances
