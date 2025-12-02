# Random Permutation

It’s actually a randomized exact algorithm that runs faster with high probability (w.h.p.), meaning that it gives the exact optimal solution almost always, but not deterministically in every single run.

### Reference Article
For reference, section 2 of https://arxiv.org/pdf/2308.11307  


### Randomized 0–1 Knapsack in ˜O(n³ᐟ² wₘₐₓ) Time

This project implements a randomized approximation algorithm for the classical 0–1 knapsack problem.  
It achieves ˜O(n³ᐟ² wₘₐₓ) runtime, improving upon the standard O(nW) dynamic programming approach when W is large but the maximum item weight (wₘₐₓ) is moderate.

---

### Algorithm Intuition

1. **Standard DP Recap**  
   The classical DP computes optimal values for each sub-capacity up to W, leading to O(nW) time complexity.

2. **Randomized Reduction**  
   This algorithm samples item weights, partitions them into groups, and uses probabilistic rounding to approximate achievable weights, significantly reducing the effective state space.

3. **Runtime**  
   The resulting expected runtime is ˜O(n³ᐟ² wₘₐₓ), maintaining good accuracy while improving scalability for large instances.

---

### Applications
Useful for large-scale resource allocation or subset selection problems where full DP becomes computationally expensive.

---

## Theoretical Analysis

### An Explanation

#### Problem Statement

The **0/1 Knapsack Problem** is: given `n` items (each with weight $w_i$ and value $v_i$) and a knapsack of capacity $W$, select a subset of items to maximize total value without exceeding $W$. Each item is either included or not (0/1 selection).

#### The Randomized Permutation Approach

This algorithm randomly permutes the items and applies a dynamic programming strategy that restricts computation to a probabilistically chosen subset of the DP table. By focusing on a band of likely achievable weights (centered around the expected total weight), it avoids the full $O(nW)$ state space, instead working in a much smaller region. The random permutation ensures that, with high probability, the optimal solution is not missed.

**Key Steps:**
- Randomly permute the items
- For each item, restrict DP updates to a band around the expected cumulative weight
- Use parent tracking for solution reconstruction
- With high probability, the optimal solution is found

---

### Time Complexity

#### Analysis

- **Random permutation:** $O(n)$
- **DP band computation:** For each item, only $O(\sqrt{n \log n} \cdot w_{max})$ states are updated (not all $W$)
- **Total:** $O(n^{3/2} w_{max} \cdot \sqrt{\log n})$ (soft-O notation: $\tilde{O}(n^{3/2} w_{max})$)
- **Best, Average, Worst Case:** All cases are similar due to the probabilistic guarantee

**Comparison:**
- **Standard DP:** $O(nW)$
- **Randomized Permutation:** $\tilde{O}(n^{3/2} w_{max})$
- Significant improvement when $W \gg n w_{max}$

---

### Space Complexity

#### Analysis

- **DP arrays:** $O(W)$ for two DP rows
- **Parent tracking:** $O(nW)$
- **Auxiliary:** $O(n)$ for items, $O(n)$ for solution reconstruction
- **Total:** $O(nW)$ (same as standard DP, but much less is actively used)

---

### Correctness

#### Discussion

- The algorithm is **randomized exact**: it finds the optimal solution with high probability (w.h.p.), but not deterministically every run
- The random permutation ensures that the banded DP covers the region where the optimal solution is likely to be found
- If the optimal solution is missed, rerunning with a new permutation can recover it
- The probability of missing the optimum decreases rapidly with $n$

**Reference:** See Section 2 of [arXiv:2308.11307](https://arxiv.org/pdf/2308.11307) for formal probabilistic analysis

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model:** Assumes constant-time arithmetic, comparisons, and array access
- **Input:** Non-negative integer weights and values, capacity $W$
- **Randomness:** Relies on a good random permutation generator
- **No negative weights/values:** Assumes valid input

---

### Case Analysis (Best/Average/Worst)

#### Best Case
- **Input:** Optimal solution lies well within the DP band for the random permutation
- **Result:** Algorithm finds the optimum in one run
- **Complexity:** $\tilde{O}(n^{3/2} w_{max})$

#### Average Case
- **Input:** Typical random instance
- **Result:** Algorithm finds the optimum with high probability
- **Complexity:** $\tilde{O}(n^{3/2} w_{max})$

#### Worst Case
- **Input:** Adversarial instance or unlucky permutation
- **Result:** Algorithm may miss the optimum; rerun needed
- **Complexity:** $\tilde{O}(n^{3/2} w_{max})$ per run

---

### Summary

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $\tilde{O}(n^{3/2} w_{max})$ |
| **Space Complexity** | $O(nW)$ |
| **Correctness** | Randomized exact (w.h.p.) |
| **Cases** | Best = Average = Worst (time), solution quality varies |

**Strengths:**
- Much faster than standard DP for large $W$
- Finds optimal solution with high probability
- Can be rerun for additional confidence
- Useful for large-scale problems

**Weaknesses:**
- Not deterministic: may miss optimum in rare cases
- Still uses $O(nW)$ space for parent tracking
- Requires good random number generation

**When to Use:**
- When $W$ is very large and $w_{max}$ is moderate
- When exact solution is needed but full DP is too slow
- For large-scale resource allocation problems
