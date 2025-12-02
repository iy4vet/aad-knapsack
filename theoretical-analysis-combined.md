# Brute Force Approach for 0/1 Knapsack Problem

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

#### The Brute Force Approach

The brute force approach is the most straightforward solution: **try every possible combination of items** and select the one with maximum value that fits within the capacity constraint.

**In English:**
We use a recursive strategy that, for each item, explores two possibilities: either include the item in our knapsack or exclude it. We recursively solve the problem for the remaining items with the updated capacity, then compare which choice (include vs. exclude) gives us a better total value. We keep track of which items we selected and return the best solution found.

**Mathematically**, the recursive relation is:

$$
K(n, W) = \begin{cases}
0 & \text{if } n < 0 \text{ or } W = 0 \\
K(n-1, W) & \text{if } w_n > W \\
\max(K(n-1, W), \, v_n + K(n-1, W - w_n)) & \text{if } w_n \leq W
\end{cases}
$$

Where:
- $K(n, W)$ represents the maximum value achievable using items $0$ to $n$ with capacity $W$
- $K(n-1, W)$ represents **excluding** item $n$
- $v_n + K(n-1, W - w_n)$ represents **including** item $n$

**Key Steps:**
1. For each item, make a recursive call to exclude it
2. If the item fits, make another recursive call to include it
3. Compare both options and return the better one
4. Base case: when no items remain or capacity is zero, return 0

This approach exhaustively explores the entire solution space, forming a **binary decision tree** of depth `n`, where each level corresponds to an item and each node represents a decision (include/exclude).

---

### Time Complexity

#### Analysis

The algorithm explores a binary tree of decisions:

1. **Recursive Tree Structure:**
   - At each level (item), we make up to 2 recursive calls (include and exclude)
   - The tree has depth `n` (one level per item)
   - In the worst case, we explore all $2^n$ possible subsets

2. **Work Per Node:**
   - Each recursive call performs constant-time operations: comparison, addition, vector operations
   - Work per node: $O(1)$

3. **Total Number of Nodes:**
   - A complete binary tree of depth `n` has $2^n$ leaf nodes
   - Total nodes in the tree: $O(2^n)$

**Overall Time Complexity:**

- **Best Case:** $O(2^n)$
  - Even in the best case, the algorithm explores the exponential decision tree
  - Early pruning (when items don't fit) reduces some branches but doesn't change asymptotic complexity

- **Average Case:** $\Theta(2^n)$
  - On average, most of the exponential tree is explored
  - The recursion doesn't have significant pruning for typical inputs

- **Worst Case:** $\Theta(2^n)$
  - When all items fit within capacity, every possible subset must be considered
  - The algorithm explores all $2^n$ combinations

**Why Exponential?**
For `n` items, there are $2^n$ possible subsets (each item can be in or out). The brute force approach must examine each subset to determine which one is optimal.

**Example:** With `n = 20` items, there are $2^{20} = 1,048,576$ combinations to check. With `n = 30`, there are over 1 billion combinations!

---

### Space Complexity

#### Analysis

The algorithm uses space for:

1. **Recursive Call Stack:**
   - Maximum recursion depth: `n` (when processing items sequentially)
   - Each stack frame stores: capacity, item index, local variables
   - Stack space: $O(n)$

2. **Solution Tracking:**
   - Each recursive call may create a vector to store selected items
   - In the worst case, vectors are copied and merged at each level
   - This adds significant overhead: $O(n)$ per recursive call

3. **Input Storage:**
   - `weights` and `values` vectors: $O(n)$

4. **Output Storage:**
   - `selectedItems` vector: $O(n)$ in the worst case

**Total Space Complexity:**

- **Auxiliary Space:** $O(n)$ for the recursion stack (not counting solution tracking)
- **Total Space:** $O(n)$ for a clean recursive implementation

**Important Note:** The current implementation creates and copies vectors at each recursive level, which in practice can lead to $O(n \cdot 2^n)$ space in the worst case due to vector copying. However, with proper optimization (using references and in-place modification), the space can be reduced to $O(n)$ for the call stack alone.

---

### Correctness

#### Proof of Correctness

We prove correctness using **strong induction** on the number of remaining items.

**Claim:** The function `knapsackBruteForceRecursive(W, weights, values, n)` correctly returns the maximum value achievable using items $0$ to $n$ with capacity $W$, along with the selected items.

**Base Case:** When $n < 0$ (no items) or $W = 0$ (zero capacity):
- Returns $(0, \{\})$ (value 0, empty set)
- This is correct: with no items or no capacity, the maximum value is 0

**Inductive Hypothesis:** Assume that for all $k < n$, the function correctly computes the maximum value for items $0$ to $k$ with any capacity $w \leq W$.

**Inductive Step:** We prove correctness for item $n$.

For item $n$ with weight $w_n$ and value $v_n$:

1. **Case 1: Item doesn't fit** ($w_n > W$)
   - The algorithm returns `excludeResult = knapsackBruteForceRecursive(W, weights, values, n-1)`
   - By the inductive hypothesis, this is the optimal solution for items $0$ to $n-1$
   - Since item $n$ cannot be included, this is also optimal for items $0$ to $n$ ✓

2. **Case 2: Item fits** ($w_n \leq W$)
   - **Exclude option:** `excludeResult = knapsackBruteForceRecursive(W, weights, values, n-1)`
     - By hypothesis, this is optimal for items $0$ to $n-1$ with capacity $W$
   
   - **Include option:** `includeResult = knapsackBruteForceRecursive(W - w_n, weights, values, n-1)`
     - By hypothesis, this is optimal for items $0$ to $n-1$ with capacity $W - w_n$
     - Adding $v_n$ gives the value when including item $n$
   
   - The algorithm returns `max(includeResult, excludeResult)`
   - This is correct because the optimal solution either includes item $n$ or doesn't
   - By considering both options and taking the maximum, we get the optimal solution ✓

**Conclusion:** By strong induction, the algorithm correctly computes the optimal solution for all valid inputs.

**Completeness:** The algorithm is complete because it exhaustively explores all $2^n$ possible subsets of items, guaranteeing that the optimal solution is found.

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model (Random Access Machine):** The algorithm assumes:
  - Array/vector access takes $O(1)$ time
  - Arithmetic operations (addition, subtraction, comparison) take $O(1)$ time
  - Function calls and returns take $O(1)$ time
  - Memory allocation and deallocation take $O(1)$ time per element

#### Assumptions

1. **Input Format:**
   - `n` items with non-negative integer weights and values
   - Capacity `W` is a non-negative integer
   - All values fit within 64-bit signed integers (`int64`)

2. **Data Types:**
   - Uses `long long` (int64) to prevent overflow for large accumulated values
   - Assumes 64-bit arithmetic operations are constant time

3. **Memory:**
   - Sufficient stack space is available for recursion depth up to `n`
   - Stack overflow is not a concern (for reasonable `n`, typically $n < 10^4$)
   - For very large `n`, this could cause stack overflow

4. **Optimality:**
   - The algorithm guarantees an **optimal solution** (maximum value)
   - It's an exact algorithm, not an approximation

5. **Input Validity:**
   - No validation is performed on input data
   - Assumes well-formed input (correct counts, non-negative values)

6. **Practicality:**
   - Due to exponential time complexity, this approach is only practical for small `n`
   - Typically feasible for $n \leq 20$ to $25$ items

---

### Case Analysis (Best/Average/Worst)

#### Best Case

**Input Characteristics:** All items have weights greater than the capacity (none fit).

**Time Complexity:** $O(2^n)$

**Explanation:** Even when no items fit, the algorithm still makes recursive calls to explore both the "exclude" and "include" branches. The "include" branch immediately returns when checking if the item fits, but the recursive tree is still explored. While there's some pruning, the exponential nature remains.

**Example:** If `W = 10` and all weights are `> 10`, the algorithm still recurses through the decision tree, though the "include" branches terminate quickly.

**Space Complexity:** $O(n)$ for the recursion stack.

**Practical Note:** This case is slightly faster in practice due to early termination of "include" branches, but asymptotically still exponential.

#### Average Case

**Input Characteristics:** Random weights and values with some items fitting and some not.

**Time Complexity:** $\Theta(2^n)$

**Explanation:** For typical inputs, the algorithm explores most of the exponential decision tree. Some branches may be pruned when items don't fit, but on average, a significant portion of the $2^n$ subsets are examined. The recursive structure ensures that the algorithm visits an exponential number of nodes.

**Space Complexity:** $O(n)$ for the recursion stack.

**Practical Behavior:** The actual number of nodes visited depends on how many items can fit, but it remains exponential for any reasonable distribution of weights.

#### Worst Case

**Input Characteristics:** All items have weights less than or equal to the capacity (all fit).

**Time Complexity:** $\Theta(2^n)$

**Explanation:** This is the true worst case where the algorithm must explore every possible subset. Both "include" and "exclude" branches are fully explored for every item, resulting in a complete binary tree of depth `n` with $2^n$ leaf nodes. No pruning occurs.

**Example:** If `W = 1000` and all weights are `≤ 10`, then for `n = 20` items, all $2^{20} = 1,048,576$ combinations must be checked.

**Space Complexity:** $O(n)$ for the recursion stack (though vector copying in the current implementation may increase this).

**Critical Problem:** This exponential growth makes the algorithm **infeasible for large inputs**. Even modern computers cannot handle $n > 30$ in reasonable time.

---

### Comparison of Cases

| Case | Time Complexity | Space Complexity | Input Characteristics |
|------|----------------|------------------|----------------------|
| **Best** | $O(2^n)$ | $O(n)$ | Items don't fit (some pruning) |
| **Average** | $\Theta(2^n)$ | $O(n)$ | Mixed fitting items |
| **Worst** | $\Theta(2^n)$ | $O(n)$ | All items fit (full tree) |

**Key Insight:** Unlike some algorithms where best/worst cases differ significantly, the brute force approach is exponential **in all cases**. The difference is only in the constant factors (how much of the tree is explored), not in the asymptotic complexity.

---

### Summary

The **Brute Force solution** for the 0/1 Knapsack problem is:

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $\Theta(2^n)$ (exponential) |
| **Space Complexity** | $O(n)$ (recursion stack) |
| **Correctness** | Proven optimal via induction |
| **Practical Limit** | $n \leq 20$ to $25$ items |

**Strengths:**
- Simple and intuitive implementation
- Guarantees optimal solution
- Easy to understand and verify correctness
- No dependency on capacity value (unlike DP)

**Weaknesses:**
- **Exponential time complexity** - impractical for large `n`
- Explores redundant subproblems (no memoization)
- Significant recursive overhead
- Stack overflow risk for very large `n`

**When to Use:**
- Very small problem instances ($n \leq 20$)
- Educational purposes to understand the problem
- As a baseline to compare other algorithms
- When optimality is required and `n` is guaranteed to be small

**Why Better Algorithms Exist:**
The brute force approach has **overlapping subproblems** (the same subproblems are solved multiple times). Dynamic programming and memoization exploit this structure to reduce complexity from $O(2^n)$ to $O(n \times W)$, making much larger problems tractable.

For reference: https://www.w3schools.com/dsa/dsa_ref_knapsack.php


# Memoization Approach for 0/1 Knapsack Problem

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

#### The Memoization Approach

The memoization approach is a **top-down dynamic programming** technique that combines the recursive structure of brute force with intelligent caching to avoid redundant computations.

**In English:**
We start with the original problem and recursively break it down into smaller subproblems, just like brute force. However, before solving each subproblem, we check if we've already solved it before. If we have, we simply return the cached result. If not, we compute it, store the result in a memo table, and return it. This way, each unique subproblem is solved exactly once.

**Key Insight:** The brute force approach has **overlapping subproblems** - the same subproblem `(i, remaining_capacity)` is encountered multiple times through different recursive paths. Memoization eliminates this redundancy by caching results.

**Mathematically**, the recurrence relation with memoization is:

$$
M[i][w] = \begin{cases}
0 & \text{if } i = 0 \text{ or } w = 0 \\
M[i][w] & \text{if already computed (cached)} \\
M[i-1][w] & \text{if } w_i > w \text{ (doesn't fit)} \\
\max(M[i-1][w], \, v_i + M[i-1][w - w_i]) & \text{if } w_i \leq w \text{ (fits)}
\end{cases}
$$

Where $M[i][w]$ represents the memoized result for the maximum value achievable using the first `i` items with capacity `w`.

**Process Flow:**
1. Check if `memo[i][remaining_capacity]` is already computed (not `-1`)
2. If yes, return the cached value immediately (O(1) lookup)
3. If no, recursively compute the result:
   - If item doesn't fit: recurse with `(i-1, capacity)`
   - If item fits: take max of including vs excluding
4. Store the result in `memo[i][remaining_capacity]`
5. Return the computed result

**Comparison with Brute Force:**
- **Brute Force:** Explores $2^n$ nodes, solving the same subproblems repeatedly
- **Memoization:** Solves each unique subproblem once, with at most $n \times W$ unique subproblems

---

### Time Complexity

#### Analysis

The time complexity depends on the number of unique subproblems and the work done per subproblem.

1. **Number of Unique Subproblems:**
   - Each subproblem is defined by two parameters: item index `i` (range: $0$ to $n$) and capacity `w` (range: $0$ to $W$)
   - Total unique subproblems: $(n+1) \times (W+1) = O(n \times W)$

2. **Work Per Subproblem:**
   - **Cache hit:** If already computed, lookup is $O(1)$
   - **Cache miss:** Perform computation (comparison, addition, max operation, recursive calls)
   - The actual computation is $O(1)$ (constant operations)
   - Recursive calls don't add to the count because they either hit the cache or solve a new subproblem (counted separately)

3. **Total Computation:**
   - At most $n \times W$ subproblems are computed (cache misses)
   - Each computation takes $O(1)$ time
   - Total: $O(n \times W)$

4. **Reconstruction Phase:**
   - Backtracking through the memo table to find selected items
   - Visits at most $n$ items
   - Time: $O(n)$

**Overall Time Complexity:**

- **Best Case:** $O(n \times W)$
  - Even if many branches terminate early, we still need to fill reachable states in the memo table
  - The recursive structure ensures we compute necessary subproblems

- **Average Case:** $\Theta(n \times W)$
  - For typical inputs, most states in the memo table are visited
  - The algorithm explores the state space systematically

- **Worst Case:** $\Theta(n \times W)$
  - All states in the memo table are computed
  - This occurs when all items can potentially fit

**Comparison with Other Approaches:**
- **Brute Force:** $O(2^n)$ - exponential
- **Memoization:** $O(n \times W)$ - pseudo-polynomial
- **Dynamic Programming (Bottom-Up):** $O(n \times W)$ - same asymptotic complexity

**Pseudo-Polynomial Nature:** The complexity depends on the *value* of `W`, not just its *size* (number of bits). If `W` is exponentially large, the algorithm becomes impractical.

---

### Space Complexity

#### Analysis

The algorithm uses space for several components:

1. **Memoization Table:**
   - Dimensions: $(n+1) \times (W+1)$
   - Each entry stores an `int64` value
   - Space: $O(n \times W)$

2. **Recursion Call Stack:**
   - Maximum recursion depth: $O(n + W)$ in the worst case
   - In practice, depth is typically $O(n)$ as we process items sequentially
   - Each stack frame stores: item index, capacity, return address
   - Stack space: $O(n)$ typically, $O(n + W)$ worst case

3. **Input Storage:**
   - `weights` and `values` vectors: $O(n)$ each
   - Total: $O(n)$

4. **Output Storage:**
   - `selected_items` vector: $O(n)$ in worst case

**Total Space Complexity:**

- **Auxiliary Space:** $O(n \times W)$ (dominated by memo table)
- **Total Space:** $O(n \times W + n) = O(n \times W)$

**Comparison with Bottom-Up DP:**
- **Memoization:** Requires $O(n \times W)$ for memo table + $O(n)$ for recursion stack
- **Bottom-Up DP:** Requires $O(n \times W)$ for DP table (no recursion stack overhead)
- **Optimized Bottom-Up:** Can use $O(W)$ space with rolling array technique
- **Memoization Advantage:** Only allocates space for *reachable* states if using sparse data structures (e.g., hash maps), though this implementation uses a full 2D array

---

### Correctness

#### Proof of Correctness

We prove correctness using **strong induction** combined with the **memoization invariant**.

**Memoization Invariant:** Once `memo[i][w]` is computed and stored, it contains the correct maximum value for the subproblem of using the first `i` items with capacity `w`.

**Claim:** The function `knapsack_memoization(i, w)` correctly returns the maximum value achievable using the first `i` items with capacity `w`.

**Base Case:** When $i = 0$ (no items) or $w = 0$ (zero capacity):
- Returns $0$
- This is correct: no items or no capacity means zero value ✓

**Inductive Hypothesis:** Assume that for all pairs $(k, c)$ where either $k < i$ or $c < w$ (or both), if `knapsack_memoization(k, c)` is called, it returns the correct maximum value.

**Inductive Step:** We prove correctness for `knapsack_memoization(i, w)`.

**Case 1: Result is already memoized** (`memo[i][w] != -1`):
- Return the cached value
- By the memoization invariant, this value was correctly computed earlier ✓

**Case 2: Item doesn't fit** ($w_i > w$):
- Compute `memo[i][w] = knapsack_memoization(i-1, w)`
- By the inductive hypothesis, the recursive call returns the correct value for items $1$ to $i-1$ with capacity $w$
- Since item $i$ cannot be included, this is the optimal value ✓

**Case 3: Item fits** ($w_i \leq w$):
- Compute two options:
  - **Include:** `include_item = values[i-1] + knapsack_memoization(i-1, w - weights[i-1])`
  - **Exclude:** `exclude_item = knapsack_memoization(i-1, w)`
- By the inductive hypothesis, both recursive calls return correct values for their respective subproblems
- The maximum of these two options is the optimal choice ✓
- Store this in `memo[i][w]`

**Conclusion:** By strong induction and the memoization invariant, the algorithm correctly computes the optimal value for all valid inputs.

**Reconstruction Correctness:** The `reconstruct_solution()` function backtracks through the memo table:
- If `memo[i][w] != memo[i-1][w]`, then item $i$ must have been included (its inclusion changed the value)
- This correctly identifies all selected items ✓

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model (Random Access Machine):** The algorithm assumes:
  - Array/vector access takes $O(1)$ time
  - Arithmetic operations (addition, subtraction, comparison) take $O(1)$ time
  - Function calls take $O(1)$ time (though recursion has overhead)
  - Memory allocation is pre-computed (memo table allocated upfront)

#### Assumptions

1. **Input Format:**
   - `n` items with non-negative integer weights and values
   - Capacity `W` is a non-negative integer
   - All values fit within 64-bit signed integers (`int64`)

2. **Data Types:**
   - Uses `long long` (int64) to prevent overflow
   - Assumes 64-bit arithmetic operations are constant time

3. **Memory:**
   - Sufficient memory to allocate $(n+1) \times (W+1)$ memo table
   - For large `W` (e.g., $W > 10^7$), memory requirements can be prohibitive
   - Stack space is sufficient for recursion depth $O(n)$

4. **Initialization:**
   - Memo table initialized to `-1` to indicate uncomputed states
   - Assumes `-1` is not a valid result value (which is true for non-negative values)

5. **Optimality:**
   - Guarantees **optimal solution** (maximum value)
   - Exact algorithm, not an approximation

6. **Recursion Overhead:**
   - Function call overhead exists (stack frame creation/destruction)
   - This makes memoization slightly slower than iterative DP in practice
   - However, asymptotic complexity remains the same

7. **Cache Efficiency:**
   - Memory access patterns are less cache-friendly than bottom-up DP
   - Top-down recursion has irregular memory access patterns

---

### Case Analysis (Best/Average/Worst)

#### Best Case

**Input Characteristics:** Items with very large weights relative to capacity, causing early termination in many branches.

**Time Complexity:** $O(n \times W)$

**Explanation:** Even with early termination, the memoization approach still needs to explore and cache results for reachable states. In the best case, fewer states might be visited if many branches terminate early (items don't fit), but the algorithm still needs to compute results for all visited states. The asymptotic complexity remains $O(n \times W)$ because the memo table structure dictates the maximum number of unique subproblems.

**Space Complexity:** $O(n \times W)$ - memo table is fully allocated upfront.

**Practical Performance:** Slightly faster than average due to more cache hits from early termination patterns, but the difference is marginal.

#### Average Case

**Input Characteristics:** Random weights and values with typical distribution.

**Time Complexity:** $\Theta(n \times W)$

**Explanation:** For typical inputs, most of the $(n+1) \times (W+1)$ states are visited and computed. The recursive structure systematically explores the state space, computing each unique subproblem once. The memoization ensures that each state is computed at most once, resulting in the characteristic $O(n \times W)$ complexity.

**Space Complexity:** $O(n \times W)$ for the memo table.

**Practical Performance:** 
- Slightly slower than bottom-up DP due to function call overhead
- Better than bottom-up if many states are unreachable (sparse state space)
- Memory access patterns are less cache-friendly

#### Worst Case

**Input Characteristics:** All items can potentially fit; all states in the memo table are visited.

**Time Complexity:** $\Theta(n \times W)$

**Explanation:** When all items can fit within various capacity constraints, the algorithm explores the entire state space. Every combination of `(i, w)` for $i \in [0, n]$ and $w \in [0, W]$ is visited. Each state is computed once and cached, resulting in exactly $n \times W$ computations, each taking $O(1)$ time.

**Example:** If `W = 1000` and all weights are small (e.g., `≤ 10`), then all capacity values from `0` to `W` are reachable for each item, maximizing the number of states visited.

**Space Complexity:** $O(n \times W)$ - full memo table is used.

**Practical Performance:** This represents the maximum work the algorithm performs, matching the complexity of bottom-up DP but with additional recursion overhead.

---

### Comparison of Cases

| Case | Time Complexity | Space Complexity | States Visited |
|------|----------------|------------------|----------------|
| **Best** | $O(n \times W)$ | $O(n \times W)$ | Fewer (some pruning) |
| **Average** | $\Theta(n \times W)$ | $O(n \times W)$ | Most states |
| **Worst** | $\Theta(n \times W)$ | $O(n \times W)$ | All states |

**Key Insight:** Unlike brute force (which is exponential in all cases), memoization reduces complexity to pseudo-polynomial $O(n \times W)$ by eliminating redundant computation. The difference between cases is primarily in the *constant factors* (exact number of states visited) rather than asymptotic complexity.

---

### Comparison: Memoization vs Bottom-Up Dynamic Programming

Both approaches solve the same problem with the same asymptotic complexity, but differ in implementation style and practical performance:

| Feature | Top-Down (Memoization) | Bottom-Up (Tabulation) |
|---------|------------------------|------------------------|
| **Time Complexity** | $O(n \times W)$ | $O(n \times W)$ |
| **Space Complexity** | $O(n \times W)$ + $O(n)$ stack | $O(n \times W)$ (reducible to $O(W)$) |
| **Approach** | Recursive with caching | Iterative with table filling |
| **Computation Order** | On-demand (lazy) | Systematic (eager) |
| **Function Overhead** | Higher (recursion) | Lower (loops) |
| **Cache Efficiency** | Lower (irregular access) | Higher (sequential access) |
| **Code Intuition** | More natural (matches recurrence) | Less intuitive initially |
| **Sparse States** | Efficient (computes only needed) | Inefficient (computes all) |
| **Stack Overflow Risk** | Possible for large `n` | None |
| **Space Optimization** | Harder | Easier (rolling array) |

**When to Prefer Memoization:**
- When the state space is sparse (many unreachable states)
- When the recursive formulation is more intuitive
- For educational purposes (closer to mathematical recurrence)
- When prototyping or debugging (easier to trace)

**When to Prefer Bottom-Up DP:**
- When all states need to be computed anyway
- When maximum performance is critical
- When space optimization is needed
- For very large `n` (avoid stack overflow)

---

### Summary

The **Memoization solution** for the 0/1 Knapsack problem is:

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $\Theta(n \times W)$ (pseudo-polynomial) |
| **Space Complexity** | $O(n \times W)$ + $O(n)$ recursion stack |
| **Correctness** | Proven optimal via induction + memoization invariant |
| **Cases** | Best = Average = Worst (asymptotically) |

**Strengths:**
- Guarantees optimal solution
- Eliminates exponential redundancy of brute force
- More intuitive than bottom-up DP (follows natural recursion)
- Efficient for sparse state spaces
- Easier to understand and debug

**Weaknesses:**
- Pseudo-polynomial (impractical for very large `W`)
- Function call overhead (slower than bottom-up in practice)
- Stack overflow risk for very large `n`
- Less cache-friendly memory access patterns
- Space optimization is harder than bottom-up

**Complexity Improvement Over Brute Force:**
- **Brute Force:** $O(2^n)$ - explores all $2^n$ subsets repeatedly
- **Memoization:** $O(n \times W)$ - solves each of $n \times W$ subproblems once

**Example:** For `n = 20` and `W = 1000`:
- **Brute Force:** ~1 million recursive calls (impractical for `n > 25`)
- **Memoization:** ~20,000 unique subproblems (very practical)

For reference: https://www.w3schools.com/dsa/dsa_ref_knapsack.php, under the section "Memoization"



# Dynamic Programming Solution for 0/1 Knapsack Problem

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

#### The Dynamic Programming Approach

The algorithm uses **bottom-up dynamic programming (tabulation)** to solve this problem optimally. The key insight is to break down the problem into overlapping subproblems and build solutions incrementally.

We create a table where each cell `dp[i][w]` represents "the maximum value achievable using the first `i` items with a knapsack capacity of `w`." We start with no items and build up, considering each item one by one. For each item, at each possible capacity, we make a decision: should we include this item or not? We pick whichever choice gives us more value.

**Mathematically**, the recurrence relation is:

$$
dp[i][w] = \begin{cases}
0 & \text{if } i = 0 \text{ or } w = 0 \\
dp[i-1][w] & \text{if } w_i > w \\
\max(dp[i-1][w], \, v_i + dp[i-1][w - w_i]) & \text{if } w_i \leq w
\end{cases}
$$

Where:
- `dp[i-1][w]` represents the value if we **exclude** item `i`
- `v_i + dp[i-1][w - w_i]` represents the value if we **include** item `i`

**Backtracking:** After filling the table, we trace back from `dp[n][W]` to determine which items were selected by comparing values in adjacent cells.

---

### Time Complexity

#### Analysis

The algorithm consists of two main phases:

1. **Table Filling Phase:**
   - We have two nested loops: one iterating over `n` items, another over `W+1` capacity values
   - Each cell computation involves constant-time operations (comparison, addition, max)
   - Total operations: $O(n \times W)$

2. **Backtracking Phase:**
   - We iterate backward through at most `n` items
   - Each iteration performs constant-time operations
   - Total operations: $O(n)$

**Overall Time Complexity:**

- **Best Case:** $\Theta(n \times W)$
- **Average Case:** $\Theta(n \times W)$
- **Worst Case:** $\Theta(n \times W)$

All cases have the same complexity because the algorithm must fill the entire DP table regardless of input values. The number of operations is deterministic and depends only on `n` and `W`.

**Important Note:** This is a **pseudo-polynomial time** algorithm because the complexity depends on the *value* of `W` (the capacity), not just the *size* of the input. If `W` is exponentially large relative to `n`, the algorithm becomes impractical.

---

### Space Complexity

#### Analysis

The algorithm uses several data structures:

1. **DP Table:** `vector<vector<int64>> dp(n+1, vector<int64>(W+1, 0))`
   - Dimensions: $(n+1) \times (W+1)$
   - Space: $O(n \times W)$

2. **Input Vectors:** `weights` and `values`
   - Space: $O(n)$ each, total $O(n)$

3. **Output Vector:** `selectedItems`
   - Worst case: all items selected
   - Space: $O(n)$

4. **Other Variables:** Constant space for loop counters, temporary variables
   - Space: $O(1)$

**Total Space Complexity:**

- **Auxiliary Space:** $O(n \times W)$ (dominated by the DP table)
- **Total Space:** $O(n \times W)$

**Space Optimization Note:** This implementation uses a 2D table for clarity and to support backtracking. It's possible to optimize space to $O(W)$ using a 1D array if we only need the maximum value (without tracking selected items), though backtracking would require additional techniques.

---

### Correctness

#### Proof of Correctness

We prove correctness using **mathematical induction** on the number of items considered.

**Base Case:** When $i = 0$ (no items) or $w = 0$ (zero capacity):
- $dp[0][w] = 0$ for all $w$, which is correct (no items means no value)
- $dp[i][0] = 0$ for all $i$, which is correct (zero capacity means nothing can be taken)

**Inductive Hypothesis:** Assume that for all $k < i$, the value $dp[k][w]$ correctly represents the maximum value achievable using the first $k$ items with capacity $w$.

**Inductive Step:** We prove that $dp[i][w]$ is correct.

For item $i$ with weight $w_i$ and value $v_i$:

1. **If $w_i > w$:** The item cannot fit, so we cannot include it. The only option is to exclude it:
   $$dp[i][w] = dp[i-1][w]$$
   By the inductive hypothesis, $dp[i-1][w]$ is correct, so $dp[i][w]$ is correct.

2. **If $w_i \leq w$:** We have two choices:
   - **Exclude item $i$:** Value is $dp[i-1][w]$ (correct by hypothesis)
   - **Include item $i$:** We get value $v_i$, plus the best we can do with remaining capacity $w - w_i$ using items $1$ to $i-1$, which is $dp[i-1][w - w_i]$ (correct by hypothesis)
   
   Taking the maximum of these two options gives us the optimal value:
   $$dp[i][w] = \max(dp[i-1][w], \, v_i + dp[i-1][w - w_i])$$

**Conclusion:** By induction, $dp[n][W]$ correctly represents the maximum value achievable with all $n$ items and capacity $W$.

**Backtracking Correctness:** The backtracking phase reconstructs the solution by checking whether $dp[i][w] \neq dp[i-1][w]$. If they differ, item $i$ must have been included (since including it gave a better value). This correctly identifies all selected items.

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model (Random Access Machine):** The algorithm assumes a standard computational model where:
  - Array/vector access takes $O(1)$ time
  - Arithmetic operations (addition, subtraction, comparison) take $O(1)$ time
  - Memory access is uniform cost

#### Assumptions

1. **Input Format:**
   - `n` items with non-negative integer weights and values
   - Capacity `W` is a non-negative integer
   - All values fit within 64-bit signed integers (`int64`)

2. **Data Types:**
   - Uses `long long` (int64) to prevent overflow for large values
   - Assumes arithmetic operations on 64-bit integers are constant time

3. **Memory:**
   - Sufficient memory is available to allocate the $(n+1) \times (W+1)$ DP table
   - For very large `W`, this could require gigabytes of memory

4. **Optimality:**
   - The algorithm guarantees an **optimal solution** (maximum value)
   - Unlike greedy or heuristic approaches, this is an exact algorithm

5. **Input Validity:**
   - No validation is performed on input data
   - Assumes well-formed input (correct counts, non-negative values)

---

### Case Analysis (Best/Average/Worst)

#### Best Case

**Input Characteristics:** Any valid input with `n` items and capacity `W`.

**Time Complexity:** $\Theta(n \times W)$

**Explanation:** The dynamic programming algorithm always fills the entire $(n+1) \times (W+1)$ table, regardless of the specific values of weights and items. There is no "early termination" condition. Even if all items are too heavy (all $w_i > W$), the algorithm still iterates through all cells, though many will simply copy the value from the previous row.

**Space Complexity:** $O(n \times W)$ (same as average/worst case)

#### Average Case

**Input Characteristics:** Random weights and values with no special structure.

**Time Complexity:** $\Theta(n \times W)$

**Explanation:** The average case is identical to the best and worst cases. The algorithm's behavior is deterministic and independent of the distribution of weights and values. Each cell requires the same constant-time operations regardless of input values.

**Space Complexity:** $O(n \times W)$ (same as best/worst case)

#### Worst Case

**Input Characteristics:** Any valid input with `n` items and capacity `W`.

**Time Complexity:** $\Theta(n \times W)$

**Explanation:** Like the best and average cases, the worst case also requires filling the entire DP table. There are no inputs that cause the algorithm to perform additional work beyond the standard table filling and backtracking.

**Space Complexity:** $O(n \times W)$ (same as best/average case)

**Note on "Worst" in Practice:** While the asymptotic complexity is the same across all cases, certain inputs might be "worse" in practice:
- **Large Capacity:** If $W$ is very large (e.g., billions), the algorithm becomes impractical due to memory and time requirements
- **All Items Selected:** If all items fit in the knapsack, backtracking visits all `n` items, but this is still $O(n)$

---

### Summary

The **Dynamic Programming solution** for the 0/1 Knapsack problem is:

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $\Theta(n \times W)$ (pseudo-polynomial) |
| **Space Complexity** | $O(n \times W)$ |
| **Correctness** | Proven optimal via induction |
| **Cases** | Best = Average = Worst |

**Strengths:**
- Guarantees optimal solution
- Efficient for moderate values of `W`
- Handles arbitrary weight and value combinations

**Weaknesses:**
- Pseudo-polynomial (depends on magnitude of `W`, not just input size)
- High memory usage for large capacities
- Impractical for very large `W` (e.g., $W > 10^7$)

### For reference:
https://www.w3schools.com/dsa/dsa_ref_knapsack.php



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



## Meet-in-the-Middle Knapsack: Optimizations and Trade-offs

This implementation is based on the classic meet-in-the-middle approach for 0-1 knapsack ([GeeksforGeeks reference](https://www.geeksforgeeks.org/dsa/meet-in-the-middle/)), with several optimizations for time and memory efficiency:

### Optimizations Applied

- **Data Type Optimization:**
	- Switched from `long long` to `int` for weights, values, and capacity, reducing memory usage and improving speed (safe for datasets where values fit in 32-bit integer).
- **Mask Storage Reduction:**
	- Only store bitmasks for the left half; for the right half, store only weight and value, reconstructing the mask for the best solution only.
- **Dominance Filtering:**
	- After sorting the right half by weight, keep only non-dominated pairs (where value increases as weight increases), reducing search and memory overhead.
- **Efficient Merging:**
	- Use binary search to merge left and right halves, finding the best fit quickly.
- **Brute-force Mask Reconstruction (Right Half):**
	- For the best right half solution, reconstruct the mask only once, saving memory at the cost of a negligible time increase for small n.

### Comparison Table: Base vs. Optimized Algorithm

| Feature                        | Base Meet-in-the-Middle | Optimized Version         | Improvement / Loss         |
|--------------------------------|-------------------------|--------------------------|----------------------------|
| Data type                      | long long               | int                      | Lower memory, faster (if safe) |
| Mask storage (right half)      | All masks stored        | Only best mask reconstructed | Major memory reduction     |
| Dominance filtering            | Yes                     | Yes                      | Same (essential)           |
| Binary search for merging      | Yes                     | Yes                      | Same (essential)           |
| Output selected indices        | All indices             | All indices              | Same (no loss)             |
| Memory usage                   | High (O(2^(n/2)))       | Lower (O(2^(n/2)), but less per subset) | Improvement              |
| Time usage                     | Fast for small n        | Faster for small n       | Slight improvement         |
| Large value support            | Yes                     | No (unless int is enough)| Loss (overflow risk if values exceed int) |

**Note:**
- These optimizations make the algorithm more practical for n ≤ 40 and moderate value ranges. For very large values, revert to `long long` to avoid overflow.

---

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

#### The Meet-in-the-Middle Approach

Meet-in-the-Middle (MITM) is a **divide-and-conquer optimization technique** that reduces exponential complexity by splitting the problem into two halves, solving each independently, and then intelligently combining the results.

**Core Idea:**
Instead of exploring all $2^n$ subsets directly (like brute force), we:
1. **Divide:** Split the `n` items into two groups of roughly equal size: $n_1 = \lfloor n/2 \rfloor$ and $n_2 = \lceil n/2 \rceil$
2. **Conquer:** Generate all possible subsets for each half independently
3. **Combine:** Merge the two halves by finding compatible pairs that maximize value while staying within capacity

**In English:**
We split the items into a left half and a right half. For the left half, we compute all $2^{n_1}$ possible subsets and their (weight, value) pairs. For the right half, we do the same for all $2^{n_2}$ subsets. Then, for each left subset with weight $w_L$, we search for the right subset with the maximum value that fits in the remaining capacity $W - w_L$. This search is made efficient using sorting and binary search.

**Key Optimizations:**

1. **Dominance Filtering:**
   - After generating right subsets and sorting by weight, we filter out "dominated" subsets
   - A subset $(w_i, v_i)$ is dominated if there exists another subset $(w_j, v_j)$ where $w_j \leq w_i$ and $v_j \geq v_i$
   - Keep only non-dominated subsets (forming a **Pareto frontier**)
   - This reduces the number of candidates for binary search

2. **Binary Search:**
   - For each left subset with remaining capacity $r$, binary search for the heaviest right subset with $w_R \leq r$
   - Since right subsets are sorted by weight and filtered for dominance, the heaviest valid subset also has the maximum value
   - Search time: $O(\log m)$ where $m$ is the number of filtered right subsets

3. **Mask Reconstruction:**
   - Store bitmasks for left subsets to track which items are included
   - For right subsets, only store (weight, value) to save memory
   - Reconstruct the right mask only for the optimal solution at the end

**Mathematically**, the approach can be expressed as:

$$
\max_{S_L \subseteq \{1, ..., n_1\}, S_R \subseteq \{n_1+1, ..., n\}} \left( \sum_{i \in S_L} v_i + \sum_{j \in S_R} v_j \right)
$$

subject to:

$$
\sum_{i \in S_L} w_i + \sum_{j \in S_R} w_j \leq W
$$

**Process Flow:**

1. Generate all $2^{n_1}$ left subsets: $L = \{(w_L, v_L, mask_L)\}$
2. Generate all $2^{n_2}$ right subsets: $R = \{(w_R, v_R)\}$
3. Sort $R$ by weight: $O(2^{n_2} \log 2^{n_2}) = O(n \cdot 2^{n_2})$
4. Filter dominated subsets from $R$: $O(2^{n_2})$
5. For each $(w_L, v_L, mask_L) \in L$:
   - Compute remaining capacity: $r = W - w_L$
   - Binary search in filtered $R$ for maximum value with $w_R \leq r$: $O(\log |R|)$
   - Update best solution if $v_L + v_R > \text{best}$
6. Reconstruct right mask for optimal solution: $O(2^{n_2})$

**Comparison with Brute Force:**
- **Brute Force:** Explores $2^n$ subsets → $O(2^n)$
- **Meet-in-the-Middle:** Explores $2^{n/2} + 2^{n/2} = 2 \cdot 2^{n/2}$ subsets → $O(2^{n/2})$

**Example:** For $n = 40$:
- **Brute Force:** $2^{40} \approx 1.1 \times 10^{12}$ operations (infeasible)
- **MITM:** $2 \cdot 2^{20} \approx 2.1 \times 10^6$ operations (practical!)

---

### Time Complexity

#### Analysis

The algorithm has several distinct phases, each with its own complexity:

**Phase 1: Generate Left Subsets**
- Generate all $2^{n_1}$ subsets of the left half
- For each subset, iterate through $n_1$ items to compute weight and value
- Time: $O(n_1 \cdot 2^{n_1})$
- Since $n_1 = \lfloor n/2 \rfloor$, this is $O(n \cdot 2^{n/2})$

**Phase 2: Generate Right Subsets**
- Generate all $2^{n_2}$ subsets of the right half
- For each subset, iterate through $n_2$ items to compute weight and value
- Time: $O(n_2 \cdot 2^{n_2})$
- Since $n_2 = \lceil n/2 \rceil$, this is $O(n \cdot 2^{n/2})$

**Phase 3: Sort Right Subsets**
- Sort $2^{n_2}$ subsets by weight
- Time: $O(2^{n_2} \log 2^{n_2}) = O(n_2 \cdot 2^{n_2}) = O(n \cdot 2^{n/2})$

**Phase 4: Filter Dominated Subsets**
- Iterate through sorted right subsets once
- Keep only non-dominated subsets (Pareto frontier)
- Time: $O(2^{n_2}) = O(2^{n/2})$
- After filtering, typically $O(2^{n/2})$ subsets remain (worst case: all remain)

**Phase 5: Merge (Meet-in-the-Middle)**
- For each of $2^{n_1}$ left subsets:
  - Binary search in filtered right subsets: $O(\log m)$ where $m \leq 2^{n_2}$
- Total: $O(2^{n_1} \cdot \log 2^{n_2}) = O(2^{n/2} \cdot n)$

**Phase 6: Reconstruct Right Mask**
- Brute force search through $2^{n_2}$ subsets to find matching (weight, value)
- Time: $O(n_2 \cdot 2^{n_2}) = O(n \cdot 2^{n/2})$

**Overall Time Complexity:**

$$
T(n) = O(n \cdot 2^{n/2}) + O(n \cdot 2^{n/2}) + O(n \cdot 2^{n/2}) + O(2^{n/2}) + O(2^{n/2} \cdot n) + O(n \cdot 2^{n/2})
$$

Simplifying:

$$
T(n) = O(n \cdot 2^{n/2})
$$

**All Cases:**
- **Best Case:** $\Theta(n \cdot 2^{n/2})$
- **Average Case:** $\Theta(n \cdot 2^{n/2})$
- **Worst Case:** $\Theta(n \cdot 2^{n/2})$

The complexity is **deterministic** - all phases must execute regardless of input values. There's no early termination or pruning.

**Comparison with Other Approaches:**

| Algorithm | Time Complexity | Practical Limit |
|-----------|----------------|-----------------|
| **Brute Force** | $O(2^n)$ | $n \leq 25$ |
| **Dynamic Programming** | $O(n \times W)$ | $W \leq 10^7$ |
| **Branch & Bound** | $O(2^{n/2})$ to $O(2^n)$ | $n \leq 40$ (variable) |
| **Meet-in-the-Middle** | $O(n \cdot 2^{n/2})$ | $n \leq 40$ (consistent) |

**Key Advantage:** MITM extends the practical limit from $n \approx 25$ (brute force) to $n \approx 40$ with **predictable performance**.

---

### Space Complexity

#### Analysis

The algorithm stores subsets and intermediate results:

**1. Left Subsets Storage:**
- Store all $2^{n_1}$ subsets with (weight, value, mask)
- Each SubsetL struct: `sizeof(int) * 3 = 12` bytes (approximately)
- Total: $O(2^{n_1}) = O(2^{n/2})$ subsets
- Memory: $12 \cdot 2^{n/2}$ bytes

**2. Right Subsets Storage:**
- Store all $2^{n_2}$ subsets with (weight, value) only (no mask)
- Each SubsetR struct: `sizeof(int) * 2 = 8` bytes (approximately)
- Total: $O(2^{n_2}) = O(2^{n/2})$ subsets
- Memory: $8 \cdot 2^{n/2}$ bytes

**3. Filtered Right Subsets:**
- After dominance filtering, store non-dominated subsets
- Worst case: all $2^{n_2}$ subsets remain
- Typical case: significantly fewer (depends on data distribution)
- Memory: $O(2^{n/2})$ subsets

**4. Input Storage:**
- `weights` and `values` vectors: $O(n)$
- Total: $8n$ bytes (for `int` arrays)

**5. Output Storage:**
- `selected` vector: $O(n)$ in worst case
- Memory: $4n$ bytes

**6. Other Variables:**
- Loop counters, temporary variables: $O(1)$

**Total Space Complexity:**

$$
S(n) = O(2^{n/2}) + O(2^{n/2}) + O(2^{n/2}) + O(n) = O(2^{n/2})
$$

**Practical Memory Usage:**

For $n = 40$ (where $n/2 = 20$):
- Left subsets: $2^{20} \times 12 = 12$ MB
- Right subsets: $2^{20} \times 8 = 8$ MB
- Total: ~20 MB (very manageable!)

For $n = 48$ (where $n/2 = 24$, the hard limit in this implementation):
- Left subsets: $2^{24} \times 12 = 201$ MB
- Right subsets: $2^{24} \times 8 = 134$ MB
- Total: ~335 MB (still reasonable for modern systems)

**Comparison with Other Approaches:**

| Algorithm | Space Complexity | Example ($n=40, W=10^6$) |
|-----------|------------------|--------------------------|
| **Brute Force** | $O(n)$ | ~160 bytes |
| **Memoization** | $O(n \times W)$ | ~320 MB |
| **Dynamic Programming** | $O(n \times W)$ | ~320 MB |
| **Branch & Bound** | $O(n)$ | ~160 bytes |
| **Meet-in-the-Middle** | $O(2^{n/2})$ | ~20 MB |

**Key Insight:** MITM uses exponential space $O(2^{n/2})$, but this is manageable for $n \leq 40-48$, while being independent of capacity $W$ (unlike DP).

---

### Correctness

#### Proof of Correctness

We prove that the Meet-in-the-Middle algorithm finds the optimal solution for the 0/1 Knapsack problem.

**Claim:** The MITM algorithm correctly computes the maximum value achievable with capacity $W$.

**Proof:**

**Part 1: Completeness (Optimal Solution is Considered)**

Any valid solution to the knapsack problem can be partitioned into two disjoint subsets:
- $S_L \subseteq \{1, 2, ..., n_1\}$ (items from left half)
- $S_R \subseteq \{n_1+1, n_1+2, ..., n\}$ (items from right half)

The optimal solution $S^* = S_L^* \cup S_R^*$ has this property.

**Observation 1:** The algorithm generates **all** possible subsets of the left half, so $S_L^*$ is included in the left subsets.

**Observation 2:** The algorithm generates **all** possible subsets of the right half, so $S_R^*$ is included in the right subsets.

**Observation 3:** For the left subset $S_L^*$ with weight $w_L^*$, the remaining capacity is $r^* = W - w_L^*$.

When processing $S_L^*$, the algorithm performs binary search to find the right subset with:
- Maximum weight $\leq r^*$
- Among those, maximum value (guaranteed by dominance filtering)

Since $S_R^*$ has weight $w_R^* \leq r^*$ (by feasibility of $S^*$), and the filtered right subsets contain all non-dominated subsets, the algorithm will either:
1. Find $S_R^*$ itself, or
2. Find another subset $S_R'$ with $w_R' \leq w_R^*$ and $v_R' \geq v_R^*$ (which would be at least as good)

Either way, the algorithm considers a solution at least as good as $S^*$. ✓

**Part 2: Soundness (No Invalid Solutions)**

Every solution the algorithm produces is a valid combination of:
- A left subset $S_L$ with weight $w_L \leq W$
- A right subset $S_R$ with weight $w_R \leq W - w_L$

Therefore: $w_L + w_R \leq W$ (feasibility constraint satisfied) ✓

**Part 3: Optimality (Maximum Value is Found)**

The algorithm iterates through **all** left subsets and for each, finds the **best** compatible right subset. It maintains the maximum value found across all combinations:

$$
V^* = \max_{S_L, S_R \text{ compatible}} (v_L + v_R)
$$

Since all feasible combinations are considered (by Parts 1 and 2), the maximum found is the global optimum. ✓

**Part 4: Dominance Filtering Preserves Optimality**

**Lemma:** Filtering dominated subsets does not eliminate the optimal solution.

**Proof:** Suppose right subset $(w_i, v_i)$ is dominated by $(w_j, v_j)$ where $w_j \leq w_i$ and $v_j \geq v_i$.

For any left subset with remaining capacity $r$:
- If $(w_i, v_i)$ is feasible (i.e., $w_i \leq r$), then $(w_j, v_j)$ is also feasible (since $w_j \leq w_i \leq r$)
- The value from $(w_j, v_j)$ is at least as good: $v_j \geq v_i$

Therefore, $(w_i, v_i)$ can never contribute to the optimal solution if $(w_j, v_j)$ exists. Removing dominated subsets is safe. ✓

**Conclusion:** By completeness, soundness, optimality, and safe filtering, the MITM algorithm correctly finds the optimal solution. ✓

---

### Model of Computation/Assumptions

#### Computational Model

- **RAM Model (Random Access Machine):** The algorithm assumes:
  - Array/vector access takes $O(1)$ time
  - Arithmetic operations (addition, subtraction, comparison) take $O(1)$ time
  - Bitwise operations (masking, shifting) take $O(1)$ time
  - Memory allocation is proportional to size

#### Assumptions

1. **Input Format:**
   - `n` items with non-negative integer weights and values
   - Capacity `W` is a non-negative integer
   - All values fit within 32-bit signed integers (`int`)

2. **Data Types:**
   - Uses `int` (32-bit) for weights, values, and capacity
   - **Assumption:** Values don't exceed $2^{31} - 1 \approx 2.1 \times 10^9$
   - For larger values, use `long long` (increases memory usage)

3. **Memory Constraints:**
   - Sufficient memory to store $O(2^{n/2})$ subsets
   - For $n = 40$: ~20 MB required
   - For $n = 48$: ~335 MB required
   - Hard limit at $n_1, n_2 \leq 24$ in this implementation (prevents memory blowup)

4. **Practical Limit:**
   - The algorithm is practical for $n \leq 40$ (consistent performance)
   - Beyond $n = 48$, memory becomes prohibitive ($2^{24} \times 12 \approx 201$ MB per half)
   - The implementation exits with error code if $n_1 > 24$ or $n_2 > 24$

5. **Optimality:**
   - Guarantees **optimal solution** (maximum value)
   - Exact algorithm, not an approximation

6. **Independence from Capacity:**
   - Complexity is **independent of $W$**
   - Works efficiently even for very large capacities (e.g., $W = 10^{15}$)
   - This is a major advantage over dynamic programming

7. **Deterministic Performance:**
   - Unlike Branch & Bound, performance is **predictable**
   - Always $O(n \cdot 2^{n/2})$ regardless of input structure
   - No best/worst case variation (aside from constant factors)

8. **Mask Reconstruction:**
   - Right mask reconstruction is $O(n \cdot 2^{n/2})$ but done only once
   - This is an acceptable overhead for memory savings
   - Alternative: store all right masks for $O(1)$ reconstruction but higher memory

---

### Case Analysis (Best/Average/Worst)

#### Best Case

**Input Characteristics:** Any valid input with `n` items and capacity `W`.

**Time Complexity:** $\Theta(n \cdot 2^{n/2})$

**Explanation:** The Meet-in-the-Middle algorithm has **deterministic complexity**. Unlike Branch & Bound (which prunes) or DP (which depends on $W$), MITM must:
1. Generate all $2^{n_1}$ left subsets
2. Generate all $2^{n_2}$ right subsets
3. Sort right subsets
4. Process all left subsets with binary search

There is no "best case" that allows skipping these steps. Even if the optimal solution is found early, the algorithm must complete all phases to ensure optimality.

**Space Complexity:** $O(2^{n/2})$

**Practical Note:** Constant factors may vary (e.g., dominance filtering might reduce the number of right subsets), but asymptotic complexity remains the same.

---

#### Average Case

**Input Characteristics:** Random weights and values with typical distributions.

**Time Complexity:** $\Theta(n \cdot 2^{n/2})$

**Explanation:** The average case has the same asymptotic complexity as best/worst cases. However, practical performance benefits from:
- **Dominance filtering:** On average, removes 50-80% of right subsets
- **Cache efficiency:** Sequential access patterns in sorted arrays
- **Early loop termination:** In Phase 5, if $w_L > W$, skip that left subset immediately

**Space Complexity:** $O(2^{n/2})$

**Practical Performance:** 
- Dominance filtering typically reduces filtered right subsets to $O(2^{n/2-1})$ or less
- Binary search becomes faster with fewer candidates
- Real-world performance is often 2-3x faster than worst case

---

#### Worst Case

**Input Characteristics:** All items have identical value-to-weight ratios, or no dominance relationships exist.

**Time Complexity:** $\Theta(n \cdot 2^{n/2})$

**Explanation:** The worst case occurs when dominance filtering is ineffective:
- All right subsets are non-dominated (no filtering)
- Binary search must search through all $2^{n_2}$ right subsets
- Maximum memory usage: full $2^{n_2}$ right subsets stored

Even in this worst case, the complexity remains $O(n \cdot 2^{n/2})$, which is still much better than brute force $O(2^n)$.

**Example:** Items with weights $[1, 2, 3, 4, ...]$ and values $[2, 4, 6, 8, ...]$ (all have ratio 2.0).

**Space Complexity:** $O(2^{n/2})$ (no reduction from filtering)

**Practical Impact:** Performance is still predictable and manageable for $n \leq 40$.

---

### Comparison of Cases

| Case | Time Complexity | Space Complexity | Dominance Filtering Effectiveness |
|------|----------------|------------------|-----------------------------------|
| **Best** | $\Theta(n \cdot 2^{n/2})$ | $O(2^{n/2})$ | N/A (deterministic) |
| **Average** | $\Theta(n \cdot 2^{n/2})$ | $O(2^{n/2})$ | High (50-80% reduction) |
| **Worst** | $\Theta(n \cdot 2^{n/2})$ | $O(2^{n/2})$ | None (0% reduction) |

**Key Insight:** Unlike Branch & Bound (variable performance) or Brute Force (exponential in all cases), MITM has **consistent, predictable performance** across all input types. The difference between cases is in **constant factors** (effectiveness of filtering), not asymptotic complexity.

---

### Comparison: Meet-in-the-Middle vs Other Approaches

| Algorithm | Time Complexity | Space Complexity | Practical Limit | Depends on $W$? | Predictable? |
|-----------|----------------|------------------|----------------|----------------|--------------|
| **Brute Force** | $O(2^n)$ | $O(n)$ | $n \leq 25$ | No | Yes |
| **Memoization** | $O(n \times W)$ | $O(n \times W)$ | $W \leq 10^7$ | Yes | Yes |
| **Dynamic Programming** | $O(n \times W)$ | $O(n \times W)$ | $W \leq 10^7$ | Yes | Yes |
| **Branch & Bound** | $O(2^{n/2})$ to $O(2^n)$ | $O(n)$ | $n \leq 40$ | No | No (variable) |
| **Meet-in-the-Middle** | $O(n \cdot 2^{n/2})$ | $O(2^{n/2})$ | $n \leq 40$ | No | Yes |

**When to Use Meet-in-the-Middle:**

1. **Large Capacity:** When $W > 10^7$ and DP becomes impractical
2. **Medium $n$:** When $25 < n \leq 40$ (beyond brute force, within MITM range)
3. **Need Predictability:** When consistent performance is required
4. **Memory Available:** When $O(2^{n/2})$ memory (~20 MB for $n=40$) is acceptable

**When to Avoid Meet-in-the-Middle:**

1. **Small $n$:** When $n \leq 25$, simpler algorithms work fine
2. **Small $W$:** When $W \leq 10^6$, DP is faster and more space-efficient
3. **Large $n$:** When $n > 48$, memory becomes prohibitive
4. **Memory Constrained:** When $O(2^{n/2})$ space is too much

**Comparison with Branch & Bound:**
- **MITM:** Predictable $O(n \cdot 2^{n/2})$, uses $O(2^{n/2})$ space
- **B&B:** Variable $O(2^{n/2})$ to $O(2^n)$, uses $O(n)$ space
- **Trade-off:** MITM trades memory for consistency; B&B trades consistency for space efficiency

---

### Summary

The **Meet-in-the-Middle solution** for the 0/1 Knapsack problem is:

| Aspect | Complexity |
|--------|------------|
| **Time Complexity** | $\Theta(n \cdot 2^{n/2})$ (exponential but reduced) |
| **Space Complexity** | $O(2^{n/2})$ (exponential) |
| **Correctness** | Proven optimal via completeness + soundness |
| **Practical Limit** | $n \leq 40-48$ items |
| **Predictability** | High (deterministic performance) |

**Strengths:**
- **Optimal solution:** Guarantees finding the maximum value
- **Extends limit:** Handles $n \approx 40$ (vs. $n \approx 25$ for brute force)
- **Independent of $W$:** Works for arbitrarily large capacities
- **Predictable:** Consistent $O(n \cdot 2^{n/2})$ performance
- **Practical:** ~20 MB for $n=40$ is very manageable

**Weaknesses:**
- **Exponential space:** $O(2^{n/2})$ memory requirement
- **Not competitive with DP:** When $W$ is moderate, DP is faster
- **Hard limit:** Beyond $n \approx 48$, memory becomes prohibitive
- **No pruning:** Unlike B&B, explores all subsets (no early termination)

**Key Innovation:**
Meet-in-the-Middle achieves **square root reduction** in exponential complexity:
- From $O(2^n)$ to $O(2^{n/2})$ by splitting the problem in half
- This is a fundamental technique in exponential-time algorithms
- The square root reduction is dramatic: $2^{40} \approx 10^{12}$ → $2^{20} \approx 10^6$ (factor of $10^6$ improvement!)

**Practical Example:**
For $n = 40$:
- **Brute Force:** $2^{40} \approx 1$ trillion operations (days to compute)
- **MITM:** $2 \times 2^{20} \approx 2$ million operations (milliseconds to compute)

This makes MITM the **algorithm of choice** for knapsack problems with $25 < n \leq 40$ and large or unbounded capacities.

---



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



# Genetic Algorithm for 0/1 Knapsack

## About the Algorithm

The Genetic Algorithm (GA) is a heuristic inspired by Charles Darwin's theory of natural evolution. This algorithm reflects the process of natural selection where the fittest individuals are selected for reproduction in order to produce offspring of the next generation.

As a heuristic, the GA does not guarantee the optimal solution. However, it is very effective at finding high-quality, near-optimal solutions in a fraction of the time required by exact algorithms, especially for large problem sizes.

## Implementation

### Algorithm Overview

For the 0/1 Knapsack problem, the algorithm works as follows:

1. **Representation**: A solution, or "individual," is represented by a binary string (e.g., `[1, 0, 1, 1]`). Each bit corresponds to an item. A `1` means the item is included in the knapsack, and a `0` means it is left out.
2. **Fitness Function**: The "fitness" of an individual is the total value of the items it represents. However, if the total weight of the items exceeds the knapsack's capacity, the solution is invalid, and its fitness is considered 0.
3. **Evolution Cycle**:
    * **Initialisation**: The process starts with an initial population of randomly generated individuals.
    * **Selection**: Fitter individuals are more likely to be selected as "parents" for the next generation. This implementation uses tournament selection, where a few individuals are chosen at random, and the one with the highest fitness wins and becomes a parent.
    * **Crossover**: The genetic material of two parents is combined to create one or more "children." This is done by splitting the parents' bit strings at a random point and swapping the segments.
    * **Mutation**: To maintain genetic diversity and avoid getting stuck in local optima, random bits in a child's genetic code are flipped (0 becomes 1, and 1 becomes 0).
4. **Termination**: This process is repeated for a fixed number of generations. The best individual from the final population is presented as the (approximate) solution to the problem.

### Modifications

The C++ implementation is a robust and optimised version of the algorithm described in the reference materials. It includes features designed for performance and scalability, such as fast I/O, command-line argument parsing for hyperparameters, and memory usage tracking.

Other than re-implementing the reference Python code (see [References](#references)) in C++, we also made a few changes to make the programme feasible for large `N`.

#### Random Repair Strategy

The Python code penalises overweight individuals by assigning them a fitness of 0. Our C++ implementation includes a `Individual::repair` function that instead **randomly removes items** until the weight is valid. This allows solutions to propagate between generations, as it otherwise becomes increasingly likely for individuals to get overweight from random initialisation and mutations with large `N`.

#### Efficient Tournament Selection

The Python code shuffles the entire population list to select four individuals for a tournament, costing `O(P)` time. We instead select four random indices and ensure uniqueness by incrementing (modulo `populationSize`), costing `O(1)` space and time.

#### Dynamic Hyperparameters

Instead of using fixed values, the `populationSize` and `maxGenerations` are dynamically determined based on `N`. This allows the algorithm to adapt its effort to the problem's scale. For smaller `N`, it runs more generations with a smaller population, and for larger `N`, it uses a larger population for fewer generations to cover the solution space more effectively. These can also be overridden via command-line arguments.

### Complexity

#### Individual Operations

| Function | Time Complexity | Space Complexity | Notes |
|----------|-----------------|------------------|-------|
| `Individual::calculateMetrics()` | O(N) | O(1) | Iterates through all N items once |
| `Individual::getFitness()` | O(1) | O(1) | Amortised constant time with caching |
| `Individual::repair()` | O(N) | O(N) | Collects indices O(N), then removes items (worst case all); uses helper vector |
| `Individual::mutate()` | O(N) | O(1) | Iterates through all N bits; expected mutations: mutationRate × N |

#### Population Initialisation

| Function | Time Complexity | Space Complexity | Notes |
|----------|-----------------|------------------|-------|
| `generateInitialPopulation()` | O(P × N) | O(P × N) | Creates P individuals, each initialised and repaired |

#### Genetic Operators

| Function | Time Complexity | Space Complexity | Notes |
|----------|-----------------|------------------|-------|
| `selection()` | O(1) | O(1) | Tournament selection with 4 random individuals, fitness cached |
| `crossover()` | O(N) | O(1) | Single-point crossover with metric recalculation |
| `nextGeneration()` | O(P × N) | O(1) | Processes P individuals through selection, crossover/reproduction, mutation, and repair |

#### Overall Algorithm

**`solveKnapsackGenetic()`**

* **Time Complexity**: **O(G × P × N)**
  * Initial population generation: O(P × N)
  * G generations of evolution: G × O(P × N)
  * Finding best individual: O(P)
  * Total: dominated by O(G × P × N)

* **Space Complexity**: **O(P × N)**
  * Two populations: 2 × P individuals × N bits each
  * Item arrays (`weights`, `values`): O(N)
  * Helper vector (`included_indices`): O(N)
  * Result `selectedItems`: O(N) worst case
  * Total: dominated by O(P × N)

Where:

* **N** = number of items
* **P** = `populationSize` (dynamically set: 20–150 based on N)
* **G** = `maxGenerations` (dynamically set: 30–200 based on N)

#### Dynamic Scaling Analysis

The implementation uses adaptive hyperparameters to maintain practical efficiency:

| N Range | P (Population) | G (Generations) | G × P | Effective Complexity |
|---------|----------------|-----------------|-------|----------------------|
| N < 100 | 20 | 200 | 4,000 | O(4,000N) |
| 100 ≤ N < 1,000 | 50 | 100 | 5,000 | O(5,000N) |
| 1,000 ≤ N < 10,000 | 100 | 50 | 5,000 | O(5,000N) |
| N ≥ 10,000 | 150 | 30 | 4,500 | O(4,500N) |

By keeping the product G × P approximately constant as N grows, the algorithm achieves **pseudo-linear O(N) scaling** in practice for large problem instances. This is a key optimisation that makes the GA feasible for large-scale knapsack problems where exact algorithms would be computationally prohibitive.

| Parameter | Default Value | Override Flag |
|-----------|---------------|---------------|
| Population Size | Dynamic (20–150) | `--population_size` |
| Max Generations | Dynamic (30–200) | `--max_generations` |
| Crossover Rate | 0.53 | `--crossover_rate` |
| Mutation Rate | 0.013 | `--mutation_rate` |
| Reproduction Rate | 0.15 | `--reproduction_rate` |
| Random Seed | Random | `--seed` |

## References

1. <https://www.youtube.com/watch?v=MacVqujSXWE>
2. <https://arpitbhayani.me/blogs/genetic-knapsack> - `Solving the Knapsack Problem with Evolutionary Algorithms` _(Fun fact: Arpit Bhayani is a IIIT alum!)_
3. <https://github.com/arpitbbhayani/genetic-knapsack> - Python code for GA _(by Arpit Bhayani, linked in his blog)_
