# Branch and Bound Approach for 0/1 Knapsack Problem

## Theoretical Analysis

### An Explanation

#### Problem Statement

The **0/1 Knapsack Problem** is a classic optimization problem where we are given:
- A set of `n` items, each with a weight `w_i` and value `v_i`
- A knapsack with maximum capacity `W`

The goal is to select a subset of items to maximize the total value while ensuring the total weight does not exceed the capacity. Each item can either be included (1) or excluded (0), hence the name "0/1 Knapsack."

**Mathematically**, we want to find:

$$
\max \sum_{i=1}^{n} v_i \cdot x_i
$$

subject to:

$$
\sum_{i=1}^{n} w_i \cdot x_i \leq W
$$

where $x_i \in \{0, 1\}$ for all $i \in \{1, 2, ..., n\}$.

#### The Branch and Bound Approach

Branch and Bound is an **intelligent exhaustive search** technique that explores the solution space more efficiently than brute force by:
1. **Branching:** Systematically exploring the decision tree (include/exclude each item)
2. **Bounding:** Computing upper bounds on potential solutions to prune branches that cannot improve the current best
3. **Pruning:** Eliminating entire subtrees when their upper bound is not better than the best solution found so far

**In English:**
We explore the solution space like a depth-first search, making binary decisions (take item or leave it) for each item. However, before exploring a branch, we calculate the **best possible value** we could achieve from that point forward (using a greedy fractional knapsack as an upper bound). If this upper bound is not better than the best complete solution we've found so far, we prune (skip) that entire branch. We also sort items by their value-to-weight ratio first, which helps us find good solutions early and prune more aggressively.

**Key Components:**

1. **Sorting:** Items are sorted by value-to-weight ratio $r_i = \frac{v_i}{w_i}$ in descending order. This heuristic helps:
   - Find high-quality solutions early (sets a high pruning threshold)
   - Make bound calculations more effective

2. **Upper Bound Function:** For any partial solution at item index $i$ with current weight $w$ and value $v$, the upper bound is:
   $$
   B(i, w, v) = v + \sum_{j=i}^{k-1} v_j + (W - w - \sum_{j=i}^{k-1} w_j) \cdot r_k
   $$
   where $k$ is the first item that doesn't fully fit. This greedily adds full items and then a fractional part of the next item.

3. **Pruning Condition:** At any node, if:
   $$
   B(i, w, v) \leq \text{maxProfit}
   $$
   we prune the entire subtree rooted at that node.

**Recursive Exploration:**

For each item at index $i$ with current weight $w$ and value $v$:

$$
\text{Branch}(i, w, v) = \begin{cases}
\text{update best if } v > \text{maxProfit} \\
\text{return if } i \geq n \text{ (base case)} \\
\text{return if } B(i, w, v) \leq \text{maxProfit} \text{ (prune)} \\
\text{Branch}(i+1, w+w_i, v+v_i) & \text{if } w + w_i \leq W \text{ (include)} \\
\text{Branch}(i+1, w, v) & \text{(exclude)}
\end{cases}
$$

**Comparison with Other Approaches:**
- **Brute Force:** Explores all $2^n$ branches blindly
- **Branch & Bound:** Explores a subset of branches, pruning unpromising paths
- **Dynamic Programming:** Avoids redundancy through memoization/tabulation
- **Branch & Bound Advantage:** No dependence on capacity value $W$ (unlike DP's $O(n \times W)$)

---

### Time Complexity

#### Analysis

Branch and Bound's time complexity is **highly variable** and depends on the effectiveness of pruning.

**Worst Case:** $O(2^n)$

**Explanation:**
- In the absolute worst case (when no pruning occurs), Branch and Bound degenerates to brute force
- This happens when:
  - The bound function never prunes (e.g., all items have identical ratios)
  - The optimal solution requires exploring most of the tree
- The algorithm still explores the full binary decision tree of depth $n$
- Total nodes in complete binary tree: $O(2^n)$

**Best Case:** $O(n \log n)$

**Explanation:**
- Dominated by the initial sorting step: $O(n \log n)$
- After sorting, if pruning is extremely effective (e.g., optimal solution found immediately and all other branches pruned):
  - Explore one path to depth $n$: $O(n)$
  - Compute bounds and prune siblings: $O(n)$
- This occurs when items have vastly different ratios and the greedy-like solution is optimal
- In practice, this is rare for realistic knapsack instances

**Average Case:** $O(2^{n/2})$ to $O(2^{n})$

**Explanation:**
- For typical problem instances with reasonable distribution of weights and values:
  - Sorting helps establish good solutions early
  - Bounding prunes a significant portion of the tree
  - Pruning efficiency improves with better ratio diversity
- Studies show that Branch and Bound often explores $O(2^{n/2})$ nodes on average for random instances
- The exact number depends heavily on:
  - Problem structure (weight/value distributions)
  - Capacity relative to total weight
  - Effectiveness of the bound function

**Per-Node Work:**
- Bound calculation: $O(n)$ in worst case (iterating through remaining items)
- In practice, bound calculations terminate early: $O(1)$ to $O(n)$
- Other operations (comparisons, updates): $O(1)$

**Total Complexity:**
- **Best Case:** $\Omega(n \log n)$ (sorting dominates)
- **Average Case:** $O(k \cdot n)$ where $k$ is the number of nodes explored (typically $k \approx 2^{n/2}$ to $2^{n}$)
- **Worst Case:** $O(2^n \cdot n)$ (full tree with $O(n)$ bound calculations per node)

**Practical Performance:**
- Much better than brute force for most instances
- Can handle $n \approx 30-40$ items efficiently (vs. $n \approx 20-25$ for brute force)
- Not competitive with DP for instances where $W$ is moderate
- Excellent for instances where $W$ is very large (avoids DP's pseudo-polynomial issue)

---

### Space Complexity

#### Analysis

The algorithm uses space for several components:

1. **Input Storage:**
   - `items` vector with Item structs: $O(n)$
   - Original `weights` and `values` vectors: $O(n)$
   - Total: $O(n)$

2. **Solution Tracking:**
   - `best` vector (best solution found): $O(n)$ boolean array
   - `curr` vector (current working solution): $O(n)$ boolean array
   - Total: $O(n)$

3. **Recursion Call Stack:**
   - Maximum depth: $n$ (one level per item decision)
   - Each stack frame stores: item index, current weight, current value, return address
   - Stack space: $O(n)$

4. **Auxiliary Variables:**
   - Sorting buffer (in-place sort): $O(\log n)$ to $O(n)$ depending on algorithm
   - Other variables (maxP, counters): $O(1)$

**Total Space Complexity:**

- **Auxiliary Space:** $O(n)$ for solution vectors + $O(n)$ for recursion stack
- **Total Space:** $O(n)$

**Comparison with Other Approaches:**
- **Brute Force:** $O(n)$ (recursion stack + solution tracking)
- **Memoization:** $O(n \times W)$ (memo table dominates)
- **Dynamic Programming:** $O(n \times W)$ (DP table)
- **Branch & Bound:** $O(n)$ (much better than DP when $W$ is large!)

**Advantage Over DP:** Branch and Bound's space complexity is independent of capacity $W$, making it preferable when:
- $W$ is extremely large (e.g., $W > 10^9$)
- Memory is constrained
- The problem structure allows effective pruning

---

### Correctness

#### Proof of Correctness

We prove correctness by showing that Branch and Bound explores all potentially optimal solutions without missing the optimum.

**Claim:** The Branch and Bound algorithm finds the optimal solution (maximum value) for the 0/1 Knapsack problem.

**Proof Strategy:** We prove two properties:
1. **Completeness:** The optimal solution is never pruned
2. **Soundness:** All non-pruned branches are correctly evaluated

---

**Part 1: Completeness (Optimal Solution is Never Pruned)**

**Lemma:** If a branch contains the optimal solution, it will never be pruned.

**Proof by Contradiction:**

Assume the optimal solution has value $V^*$ and is contained in a branch rooted at node $(i, w, v)$.

Suppose this branch is pruned, meaning:
$$
B(i, w, v) \leq \text{maxProfit}
$$

Since the optimal solution is reachable from this branch, the value achievable from $(i, w, v)$ is at least $V^*$:
$$
\text{achievable from } (i, w, v) \geq V^*
$$

The bound function $B(i, w, v)$ is constructed as an **upper bound** on achievable value, so:
$$
B(i, w, v) \geq \text{achievable from } (i, w, v) \geq V^*
$$

For pruning to occur:
$$
B(i, w, v) \leq \text{maxProfit}
$$

Therefore:
$$
V^* \leq B(i, w, v) \leq \text{maxProfit}
$$

This implies $V^* \leq \text{maxProfit}$.

But `maxProfit` represents the best complete solution found so far, and $V^*$ is the optimal solution, so:
$$
V^* \geq \text{maxProfit}
$$

Combined with $V^* \leq \text{maxProfit}$, we have:
$$
V^* = \text{maxProfit}
$$

This means we've already found the optimal solution! The branch was correctly pruned because it cannot improve upon the already-optimal solution found.

**Conclusion:** No branch containing the only optimal solution (or a strictly better solution) is ever pruned. ✓

---

**Part 2: Soundness (Correct Evaluation)**

**Lemma:** All non-pruned branches are correctly evaluated.

**Proof by Induction:**

**Base Case:** When $i \geq n$ (no more items):
- Current value $v$ is correctly recorded if $v > \text{maxProfit}$
- This is correct: we've reached a complete solution ✓

**Inductive Hypothesis:** Assume that for all recursive calls at depth $> d$, the algorithm correctly evaluates solutions.

**Inductive Step:** Consider a call at depth $d$ with node $(i, w, v)$ that is not pruned.

The algorithm explores two branches:

1. **Include item $i$** (if it fits):
   - Recurses with $(i+1, w+w_i, v+v_i)$
   - By hypothesis, this correctly evaluates all solutions including item $i$

2. **Exclude item $i$**:
   - Recurses with $(i+1, w, v)$
   - By hypothesis, this correctly evaluates all solutions excluding item $i$

Since these two branches cover all possible solutions from this node, and both are correctly evaluated, the algorithm correctly evaluates all solutions reachable from $(i, w, v)$. ✓

---

**Part 3: Optimality of Bound Function**

**Lemma:** The bound function $B(i, w, v)$ is an **upper bound** on the achievable value.

**Proof:**

The bound function computes the fractional knapsack solution from the current state:
- It greedily adds items by decreasing value-to-weight ratio
- It allows fractional items (relaxation of 0/1 constraint)

The fractional knapsack provides an **optimistic estimate** because:
1. It considers the same items available in the 0/1 problem
2. It uses the same capacity constraint
3. It relaxes the integrality constraint ($x_i \in [0,1]$ instead of $x_i \in \{0,1\}$)

Since the fractional knapsack is a **relaxation** of the 0/1 knapsack, its optimal value is always $\geq$ the 0/1 optimal value. ✓

---

**Conclusion:**

By completeness, soundness, and the correctness of the bound function, the Branch and Bound algorithm:
1. Never prunes the optimal solution
2. Correctly evaluates all non-pruned branches
3. Therefore, finds the optimal solution

✓ **Correctness proven.**

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model (Random Access Machine):** The algorithm assumes:
  - Array/vector access takes $O(1)$ time
  - Arithmetic operations (addition, subtraction, comparison, division) take $O(1)$ time
  - Floating-point operations are constant time (for ratio calculations)
  - Function calls and returns take $O(1)$ time

#### Assumptions

1. **Input Format:**
   - `n` items with positive weights and non-negative values
   - Capacity `W` is a positive integer
   - All values fit within 64-bit signed integers (`int64`)

2. **Data Types:**
   - Uses `int64` for weights, values, and capacity to prevent overflow
   - Uses `double` for ratios and bound calculations to handle fractional parts
   - Assumes floating-point arithmetic is sufficiently precise

3. **Memory:**
   - Sufficient stack space for recursion depth up to `n`
   - Stack overflow is not a concern for reasonable `n` (typically $n < 10^4$)
   - Memory usage is independent of capacity $W$

4. **Sorting:**
   - Items are sorted by value-to-weight ratio in $O(n \log n)$ time
   - Stable sort is not required (items with equal ratios can be in any order)
   - Zero-weight items are handled specially (assigned very high ratio)

5. **Bound Function:**
   - Uses **fractional knapsack** as an upper bound
   - Greedy approach on sorted items provides optimal relaxation
   - Computation can terminate early once items don't fit

6. **Optimality:**
   - Guarantees **optimal solution** (maximum value)
   - Exact algorithm, not an approximation

7. **Performance Characteristics:**
   - Performance highly dependent on problem structure
   - Works best when:
     - Items have diverse value-to-weight ratios
     - Capacity is tight (not too large or too small relative to item weights)
     - Problem exhibits structure that allows aggressive pruning

8. **System-Dependent:**
   - Memory measurement uses `getrusage()` system call (POSIX)
   - Memory reporting may vary across operating systems

---

### Case Analysis (Best/Average/Worst)

#### Best Case

**Input Characteristics:** 
- Items with vastly different value-to-weight ratios
- The greedy-like solution (taking highest-ratio items) is optimal or near-optimal
- Tight capacity constraints that enable aggressive pruning

**Time Complexity:** $O(n \log n)$

**Explanation:** 
- Sorting dominates: $O(n \log n)$
- After sorting, the algorithm:
  - Quickly finds a high-quality solution on the first deep path: $O(n)$
  - This sets a high `maxProfit` threshold
  - Subsequent bound calculations prune nearly all other branches: $O(n)$
  - Only $O(n)$ nodes are explored in total
- This occurs when the problem has a "natural" structure where high-ratio items clearly dominate

**Example:** Items with ratios like $[100, 50, 25, 10, 1]$ and capacity that fits the top few items.

**Space Complexity:** $O(n)$

**Practical Note:** This case is rare but demonstrates the power of bounding when problem structure is favorable.

---

#### Average Case

**Input Characteristics:**
- Random weights and values with typical distributions
- Moderate diversity in value-to-weight ratios
- Capacity neither too tight nor too loose

**Time Complexity:** $O(2^{n/2} \cdot n)$ to $O(2^{cn} \cdot n)$ where $0.5 < c < 1$

**Explanation:**
- For random problem instances, empirical studies show Branch and Bound typically explores $O(2^{n/2})$ to $O(2^{0.8n})$ nodes
- The exact constant depends on:
  - **Ratio diversity:** More diverse ratios → better pruning
  - **Capacity tightness:** Moderate capacity → more pruning opportunities
  - **Value/weight correlation:** Negative correlation → easier pruning
- Sorting establishes reasonably good solutions early
- Bounding eliminates a substantial fraction of branches
- Each node requires $O(1)$ to $O(n)$ work for bound calculation (typically $O(1)$ with early termination)

**Practical Performance:**
- Can handle instances with $n = 30-40$ efficiently
- Significantly faster than brute force (which struggles beyond $n = 25$)
- Performance varies widely based on problem structure

**Space Complexity:** $O(n)$

**Comparison:** Average-case performance is much better than worst case but not as good as DP for small $W$.

---

#### Worst Case

**Input Characteristics:**
- All items have identical or very similar value-to-weight ratios
- Bound function provides weak upper bounds
- Large capacity allowing many items to fit
- Optimal solution requires exploring most of the tree

**Time Complexity:** $O(2^n \cdot n)$

**Explanation:**
- When items have identical ratios, sorting provides no advantage
- The bound function rarely prunes because:
  - Upper bounds remain close to achievable values throughout the tree
  - `maxProfit` grows slowly, providing weak pruning thresholds
- The algorithm explores nearly all $2^n$ nodes in the decision tree
- Each node performs bound calculation: $O(n)$ in worst case
- Total: $O(2^n \cdot n)$ operations

**Example:** 
- All items have ratio $r_i = 2.0$
- Capacity is large enough to fit many combinations
- No clear dominant subset emerges

**Space Complexity:** $O(n)$

**Practical Impact:** 
- Performance degrades to near brute-force levels
- Still maintains $O(n)$ space advantage over DP
- Time limit may be exceeded for $n > 25-30$

---

### Comparison of Cases

| Case | Time Complexity | Nodes Explored | Pruning Effectiveness |
|------|----------------|----------------|----------------------|
| **Best** | $O(n \log n)$ | $O(n)$ | Very high (>99% pruned) |
| **Average** | $O(2^{n/2} \cdot n)$ to $O(2^{cn} \cdot n)$ | $O(2^{n/2})$ to $O(2^{cn})$ | Moderate to high (50-95% pruned) |
| **Worst** | $O(2^n \cdot n)$ | $O(2^n)$ | Very low (<10% pruned) |

**Key Factors Affecting Performance:**

1. **Ratio Diversity:** 
   - High diversity → Better pruning → Faster
   - Low diversity → Weak pruning → Slower

2. **Capacity:**
   - Too small → Few items fit → Fast (trivial)
   - Moderate → Good pruning opportunities → Fast
   - Too large → Many combinations valid → Slower

3. **Problem Structure:**
   - Structured instances (e.g., correlated weights/values) → Better
   - Random instances → Variable performance
   - Adversarial instances (identical ratios) → Worst

---

### Comparison: Branch and Bound vs Other Approaches

| Algorithm | Time Complexity | Space Complexity | When to Use |
|-----------|----------------|------------------|-------------|
| **Brute Force** | $O(2^n)$ | $O(n)$ | $n \leq 20$ (educational) |
| **Memoization** | $O(n \times W)$ | $O(n \times W)$ | Moderate $W$, need optimal |
| **Dynamic Programming** | $O(n \times W)$ | $O(n \times W)$ | Moderate $W$, need optimal |
| **Branch & Bound** | $O(2^{n/2})$ to $O(2^n)$ | $O(n)$ | Large $W$, structured problems |

**When Branch and Bound Excels:**

1. **Large Capacity:** When $W > 10^7$, DP becomes impractical due to $O(n \times W)$ complexity
2. **Space Constraints:** When memory is limited and $O(n \times W)$ is infeasible
3. **Structured Problems:** When items have diverse ratios enabling effective pruning
4. **Small to Medium $n$:** Typically $n \leq 40$ with good pruning

**When to Avoid Branch and Bound:**

1. **Small $W$:** DP is faster and more predictable
2. **Adversarial Inputs:** When ratios are uniform or performance is unpredictable
3. **Large $n$:** When $n > 50$, even with pruning, exploration may be too slow
4. **Need for Predictability:** DP has consistent $O(n \times W)$ performance

---

### Summary

The **Branch and Bound solution** for the 0/1 Knapsack problem is:

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $O(n \log n)$ to $O(2^n \cdot n)$ (highly variable) |
| **Average Time** | $O(2^{n/2} \cdot n)$ for typical instances |
| **Space Complexity** | $O(n)$ (independent of $W$!) |
| **Correctness** | Proven optimal via completeness + soundness |
| **Practical Limit** | $n \approx 30-40$ with good pruning |

**Strengths:**
- **Space efficient:** $O(n)$ independent of capacity
- **Optimal solution:** Guarantees finding the maximum value
- **Excellent for large $W$:** Avoids DP's pseudo-polynomial issue
- **Effective pruning:** Often much faster than brute force
- **No capacity dependence:** Can handle arbitrarily large $W$

**Weaknesses:**
- **Unpredictable performance:** Highly dependent on problem structure
- **Worst-case exponential:** Can degrade to $O(2^n)$
- **Slower than DP for small $W$:** DP is more predictable
- **Requires good heuristics:** Performance depends on sorting quality
- **Not suitable for large $n$:** Still exponential in worst case

**Key Innovation:**
Branch and Bound intelligently prunes the search space using **upper bounds**, reducing the effective exploration from $2^n$ nodes to (often) $2^{n/2}$ or fewer, while maintaining $O(n)$ space complexity.

For reference: https://www.geeksforgeeks.org/dsa/0-1-knapsack-using-branch-and-bound/

**Note:** Backtracking is a simpler version of Branch and Bound that explores the tree without bounding/pruning. Branch and Bound extends backtracking by adding the crucial bound-based pruning mechanism.