
## Theoretical Analysis

### An Explanation

#### Problem Statement

The **0/1 Knapsack Problem** is a classic optimization problem where we are given:
- A set of `n` items, each with a weight `w_i` and value `v_i`
- A knapsack with maximum capacity `W`

The goal is to select a subset of items to maximize the total value while ensuring the total weight does not exceed the capacity. Each item can either be included (1) or excluded (0).

#### The Greedy Heuristic Approach

The greedy heuristic sorts items by their value-to-weight ratio (density) and selects items in descending order of density, adding each item to the knapsack if it fits. This is optimal for the **fractional knapsack** (where items can be split), but not for the 0/1 variant (where items are indivisible).

**Mathematically:**
- For each item $i$, compute $d_i = v_i / w_i$
- Sort items so that $d_1 \geq d_2 \geq \ldots \geq d_n$
- For each item in order, if $w_i$ fits, add it to the knapsack

---

### Time Complexity

#### Analysis

1. **Density Calculation:** $O(n)$
2. **Sorting:** $O(n \log n)$ (dominates)
3. **Selection:** $O(n)$

**Overall:**
- **Best, Average, Worst Case:** $\Theta(n \log n)$
- Sorting dominates, so all cases are the same

---

### Space Complexity

#### Analysis

- **Auxiliary Space:** $O(n)$ for storing item structs and selected indices
- **Total Space:** $O(n)$

---

### Correctness

#### Discussion

- The greedy heuristic is **not guaranteed to be optimal** for the 0/1 knapsack problem.
- It is optimal for the **fractional knapsack** (where items can be split).
- For 0/1 knapsack, it may miss the optimal solution if the best combination involves lower-density items or complex trade-offs.
- However, it always produces a feasible solution (never exceeds capacity).

**Why it can fail:**
- There may exist a set of items with lower density whose combined value exceeds the sum of the highest-density items that fit.
- Example: Two items, A (weight 1, value 1, density 1) and B (weight 10, value 10, density 1). Capacity 10. Greedy picks B, but optimal is picking ten A's (if available).

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model:** Assumes constant-time arithmetic, comparisons, and array access
- **Input:** Non-negative integer weights and values, capacity $W$
- **No fractions:** Items are indivisible (0/1 selection)
- **No negative weights/values:** Assumes valid input

---

### Case Analysis (Best/Average/Worst)

#### Best Case
- **Input:** Items with strictly decreasing densities and all fit in the knapsack
- **Result:** Greedy finds the optimal solution
- **Complexity:** $\Theta(n \log n)$

#### Average Case
- **Input:** Random densities and weights
- **Result:** Greedy is fast, but solution quality varies
- **Complexity:** $\Theta(n \log n)$

#### Worst Case
- **Input:** Items where the optimal solution involves skipping high-density items for a better combination
- **Result:** Greedy may be far from optimal
- **Complexity:** $\Theta(n \log n)$

---

### Summary

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $\Theta(n \log n)$ |
| **Space Complexity** | $O(n)$ |
| **Correctness** | Heuristic, not guaranteed optimal |
| **Cases** | Best = Average = Worst (time), solution quality varies |

**Strengths:**
- Very fast and simple
- Good for large $n$ when approximate solutions are acceptable
- Optimal for fractional knapsack

**Weaknesses:**
- Not guaranteed optimal for 0/1 knapsack
- May perform poorly on some instances
- No approximation guarantee for 0/1 knapsack

**When to Use:**
- When speed is critical and approximate solutions are acceptable
- As a baseline for comparison with exact algorithms
- For very large $n$ where exact algorithms are infeasible

---
