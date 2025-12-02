For reference, https://arxiv.org/pdf/2002.00352

## Implementation notes and recent optimizations

This file documents the memory, initialization, convergence, and postprocessing
optimizations applied to the `09-billionscale/algo.cpp` implementation. All
changes preserve the algorithmic core — a dual‑descent (Lagrangian relaxation)
approach with multiplicative updates to the dual variable `lambda` — while
improving robustness on hard datasets and reducing memory where safe.


## Theoretical Analysis

### An Explanation

#### Problem Statement

The **0/1 Knapsack Problem** is: given `n` items (each with weight $w_i$ and value $v_i$) and a knapsack of capacity $W$, select a subset of items to maximize total value without exceeding $W$. Each item is either included or not (0/1 selection).

#### The Dual-Descent (Billionscale) Approach

This algorithm uses **Lagrangian relaxation** and a dual-descent method to efficiently approximate the solution for extremely large-scale knapsack instances. Instead of building a full $O(nW)$ DP table, it iteratively adjusts a dual variable (`lambda`) to guide selection, using multiplicative updates and postprocessing to ensure feasibility and improve solution quality.

**Key Steps:**
- Initialize `lambda` using the median profit/weight ratio for robust starting point
- Iteratively update `lambda` to balance total weight against capacity using multiplicative updates
- Select items where profit exceeds $\lambda \times$ weight
- Postprocess to remove excess items and greedily add back best-fitting items
- All steps are $O(n)$ per iteration, suitable for billion-item scale

---

### Time Complexity

#### Analysis

- **Initialization:** $O(n)$ for median ratio computation
- **Dual-descent loop:** $O(n)$ per iteration, up to $\text{max\_iters}$ (default 5000)
- **Postprocessing:** $O(n \log n)$ for sorting selected items and greedy add-back
- **Total:** $O(n \log n + n \cdot \text{max\_iters})$
- **Best/Average/Worst Case:** All cases scale linearly with $n$; postprocessing is $O(n \log n)$

**Comparison:**
- **Standard DP:** $O(nW)$ (infeasible for large $W$ or $n$)
- **Dual-descent:** $O(n \log n)$ to $O(n \cdot \text{max\_iters})$ (practical for billion-scale)

---

### Space Complexity

#### Analysis

- **Item storage:** $O(n)$ for weights, profits, and selection flags
- **Temporary vectors:** $O(n)$ for ratios, selected indices, and postprocessing
- **No DP table:** No $O(nW)$ memory usage
- **Total:** $O(n)$ (linear in the number of items)

**Practical Note:**
- Suitable for billion-item problems on modern hardware
- For extreme scale, further memory reduction (sampling, streaming) is recommended

---

### Correctness

#### Discussion

- The algorithm is a **heuristic** based on Lagrangian relaxation and dual optimization
- It does **not guarantee the exact optimal solution** for all instances, but finds high-quality solutions with very high probability
- The dual-descent loop converges to a solution that is feasible (does not exceed capacity) after postprocessing
- The greedy add-back step improves solution quality, especially for hard/balanced instances
- For most practical large-scale problems, the solution is near-optimal; for small $n$, exact algorithms are preferred

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model:** Assumes constant-time arithmetic, comparisons, and array access
- **Input:** Non-negative integer weights and values, capacity $W$
- **No negative weights/values:** Assumes valid input
- **Randomness:** Used for median estimation and OpenMP parallelization (optional)

---

### Case Analysis (Best/Average/Worst)

#### Best Case
- **Input:** Items with diverse profit/weight ratios, capacity matches sum of selected items
- **Result:** Algorithm converges quickly, postprocessing is minimal
- **Complexity:** $O(n)$ to $O(n \log n)$

#### Average Case
- **Input:** Random weights and profits, typical distributions
- **Result:** Algorithm converges in a few thousand iterations, postprocessing improves solution
- **Complexity:** $O(n \log n + n \cdot \text{max\_iters})$

#### Worst Case
- **Input:** Many items with similar ratios, capacity far from mean
- **Result:** Algorithm may require more iterations, postprocessing is more involved
- **Complexity:** $O(n \log n + n \cdot \text{max\_iters})$

---

### Summary

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $O(n \log n + n \cdot \text{max\_iters})$ |
| **Space Complexity** | $O(n)$ |
| **Correctness** | Heuristic, near-optimal for large $n$ |
| **Cases** | Best = Average = Worst (linear scaling) |

**Strengths:**
- Scales to billions of items
- Linear memory usage
- Fast convergence and robust postprocessing
- Near-optimal solutions for practical large-scale problems

**Weaknesses:**
- Not guaranteed exact for all instances
- Solution quality may vary for adversarial inputs
- For small $n$, exact algorithms are preferred

**When to Use:**
- When $n$ is extremely large and $W$ is moderate/large
- When memory and runtime are critical constraints
- For large-scale resource allocation and selection problems
