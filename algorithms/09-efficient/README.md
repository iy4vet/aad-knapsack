# Efficient Algorithm for 0/1 Knapsack

## Algorithm Explanation

This repository implements "Algorithm A," an efficient algorithm for the 0-1 knapsack problem presented by Robert M. Nauss[cite: 2, 16].

The 0-1 knapsack problem is: given a set of items, each with a weight and a value, determine the number of each item to include in a collection so that the total weight is less than or equal to a given limit and the total value is as large as possible. In the 0-1 version, you can only choose to either take an entire item (1) or leave it (0) [cite: 25-30].

Algorithm A is an **exact algorithm** (not an approximation) that finds the guaranteed optimal solution. It works by combining a fast "pegging" technique with a traditional "branch and bound" approach.

The core process is as follows:

1. **Sort:** Variables (items) are first sorted by decreasing "bang-for-buck" (value-to-weight ratio)[cite: 34, 65].
2. **Find Bounds:** The algorithm quickly finds a "lower bound" (a good, feasible solution) and an "upper bound" (the optimal, but fractional, solution from the linear programming relaxation) [cite: 35-37, 66].
3.  **Peg Variables:** This is the key efficiency step. The algorithm uses tests (based on Lagrangean relaxation) to identify and "peg" variables that must be 0 or 1 in the optimal solution [cite: 41, 71-75, 101-103]. This step is very fast and often eliminates 80-90% of the variables from consideration[cite: 55, 133].
4.  **Solve Reduced Problem:** The algorithm then solves the much smaller "reduced" knapsack problem, consisting only of the unpegged variables, using a specialized branch and bound procedure [cite: 55, 76-79].

## Time Complexity

The time complexity of this algorithm is not described by a simple polynomial.

* The **pegging phase** (Steps 1-6) is very fast. The sorting takes $O(n \log n)$ time, and the pegging tests themselves are **linearly proportional to the number of variables.** The **branch and bound phase** (Steps 7-17) is, in the **worst-case, exponentially proportional** to the number of variables.

Therefore, the **worst-case time complexity of Algorithm A is exponential**.

However, the paper's main contribution is showing that its *practical* performance is exceptionally fast. Because the linear-time pegging phase is so effective at reducing the problem size, the "reduced" problem that the exponential branch and bound phase must solve is often very small[cite: 55].

The empirical results show this: 50-variable problems were solved in an average of 4 milliseconds, and 200-variable problems in an average of 7 milliseconds on the test hardware. The algorithm scales very well in practice, far better than competing algorithms of the time[cite: 152].

---

## Space Complexity Analysis

The space complexity of Algorithm A is determined by the following components:

- **Item storage:** $O(n)$ for the list of items and their attributes (weight, value, ratio, index).
- **Pegging sets:** $O(n)$ for the vectors tracking variables pegged to 0 or 1.
- **Solution vectors:** $O(n)$ for the current and best solution vectors.
- **LP relaxation and auxiliary arrays:** $O(n)$ for fractional solution and temporary arrays.
- **Branch and bound recursion:** $O(n)$ stack depth in the worst case (one per unpegged variable).

**Total space complexity:**
- $O(n)$ (linear in the number of items)
- No dependence on knapsack capacity $B$ (unlike DP algorithms)
- The algorithm is highly space-efficient and suitable for large $n$

---

## Proof of Correctness

Algorithm A is **guaranteed to find the optimal solution** for the 0-1 knapsack problem. The correctness follows from:

1. **LP Relaxation:** The algorithm first solves the linear programming relaxation, which provides an upper bound on the optimal integer solution.
2. **Lower Bound Heuristics:** It constructs feasible integer solutions using rounding and two greedy heuristics, ensuring a valid lower bound.
3. **Pegging Tests:** The Lagrangean-based pegging tests are mathematically proven to identify variables that must be 0 or 1 in any optimal solution. Pegged variables are fixed without loss of optimality.
4. **Reduced Problem:** The remaining unpegged variables form a smaller knapsack problem. The branch and bound procedure explores all feasible combinations, using upper bounds to prune suboptimal branches. This guarantees that no optimal solution is missed.
5. **Exhaustiveness:** The algorithm considers all possible assignments for unpegged variables, so the global optimum is always found.

**Summary:**
- The combination of LP relaxation, pegging, and branch and bound ensures that Algorithm A always returns the optimal solution.
- The correctness is supported by the original paper's mathematical proofs and extensive empirical validation.
- No approximation or heuristic shortcuts are used in the final solution phase.

---
