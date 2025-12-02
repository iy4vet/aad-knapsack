# Memetic Algorithm for 0/1 Knapsack

## About the Algorithm

A **Memetic Algorithm (MA)** is a hybrid metaheuristic that combines the global search capabilities of an Evolutionary Algorithm (like the Genetic Algorithm) with the local refinement power of a Local Search technique. The name is inspired by Richard Dawkins' concept of a "meme"—a unit of cultural information that evolves and improves as it spreads.

In the context of the Knapsack Problem, this algorithm represents the state-of-the-art in heuristic solving. It doesn't just blindly evolve solutions; it mathematically reduces the problem size first and then uses "smart" agents that actively improve themselves using local search, rather than relying solely on random mutation.

## Implementation

### Modifications from Base GA

This implementation is a significant departure from the standard Genetic Algorithm. It incorporates advanced mathematical optimisation techniques to handle large-scale instances (millions of items) efficiently.

#### 1. Problem Reduction via Lagrangian Relaxation

The most critical optimisation happens *before* the genetic algorithm even starts. The Base GA operates on all $N$ items, which is inefficient for large $N$. The Memetic algorithm uses **Lagrangian Relaxation** to identify the "easy" decisions:

* **Lagrangian Multipliers**: We calculate optimal multipliers to estimate the "reduced cost" of each item.
* **Variable Fixing**: Items with very high reduced costs are fixed to $0$ (excluded), and items with very low reduced costs are fixed to $1$ (included).
* **The Core Problem**: The GA only evolves the remaining subset of items (the "Core") where the decision is difficult.

For a problem with 1,000,000 items, the Core might only contain 100–500 items. This allows the GA to converge extremely fast.

#### 2. Memetic Local Search (Tabu Search)

In the Base GA, individuals only change via random mutation. In this Memetic Algorithm, individuals actively improve themselves using **Tabu Search**:

* After mutation, an individual undergoes a local search phase.
* It explores its "neighbourhood" by flipping bits to see if a better solution exists nearby.
* **Tabu List**: To prevent cycling (flipping the same bit back and forth), recently flipped bits are marked "tabu" (forbidden) for a few iterations.

#### 3. Lagrangian-Guided Repair

The Base GA repairs overweight individuals by randomly removing items. The Memetic Algorithm uses a much smarter **Greedy Repair** based on reduced costs:

* **Removal**: If overweight, it removes items with the *worst* reduced costs (least "bang for buck" in the Lagrangian sense).
* **Addition**: If there is space, it adds items with the *best* reduced costs.

This ensures that even the repair process pushes the solution towards optimality.

#### 4. Gap-Based Adaptive Scaling

The algorithm measures the **Duality Gap** (the difference between the best possible mathematical upper bound and the current best solution).

* **Small Gap**: The problem is easy. The algorithm reduces search intensity to save time.
* **Large Gap**: The problem is hard. The algorithm automatically increases `tabuIterations` and `localSearchProb` to dig deeper.

#### 5. Heuristic Population Seeding

The initial population is not just random. It includes:

* **RCBO Individuals**: Generated using "Reduced Cost Based Ordering" (a heuristic derived from the Lagrangian relaxation).
* **VW Greedy Individuals**: Generated using the standard Value/Weight ratio.

### Complexity

The complexity is split into two phases: Preprocessing and Evolution.

#### Preprocessing (Lagrangian Relaxation)

| Function | Time Complexity | Notes |
|----------|-----------------|-------|
| `performPreprocessing()` | $O(N \log N + K \cdot N)$ | Sorting items ($N \log N$) + $K$ subgradient iterations ($O(N)$ each). |

#### Core Evolution (The Genetic Part)

Because the GA only runs on the **Core** (size $C$), and $C \ll N$, the evolution is extremely fast regardless of the original problem size.

| Function | Time Complexity | Notes |
|----------|-----------------|-------|
| `Individual::repair()` | $O(C)$ | Linear scan of the Core items only. |
| `Individual::localSearch()` | $O(I \cdot S)$ | $I$ iterations $\times$ $S$ neighbours. Constant relative to $N$. |
| `solveKnapsackGenetic()` | $O(G \cdot P \cdot (C + I \cdot S))$ | Generations $\times$ Population $\times$ (Crossover + Local Search). |

**Effective Complexity**: For large $N$, the runtime is dominated by the $O(N \log N)$ sorting in the preprocessing phase. The evolutionary part is effectively constant time $O(1)$ relative to $N$ because the Core size $C$ typically stays small (e.g., $\approx 200$) even as $N$ grows to billions.

### Summary of Improvements

| Feature | Base Genetic Algorithm | Memetic Algorithm |
| :--- | :--- | :--- |
| **Problem Scope** | Evolves all $N$ items | Evolves only "Core" items ($C \ll N$) |
| **Optimisation** | Purely biological evolution | Mathematical reduction + Evolution |
| **Local Improvement** | None (Random Mutation only) | **Tabu Search** (Active improvement) |
| **Repair Strategy** | Random Removal | **Lagrangian Reduced Cost** Greedy |
| **Scalability** | Slow for $N > 10,000$ | Efficient for $N > 1,000,000$ |

## References

1. **Chu, P. C., & Beasley, J. E. (1998).** *A Genetic Algorithm for the Multidimensional Knapsack Problem.* Journal of Heuristics. (The seminal paper on using GA with repair operators for Knapsack).
2. **Pisinger, D. (2005).** *Where are the hard knapsack problems?* Computers & Operations Research. (Discusses Core problems and hardness).
3. **Lagrangian Relaxation**: Standard technique in combinatorial optimisation to find upper bounds and reduce problem size.
