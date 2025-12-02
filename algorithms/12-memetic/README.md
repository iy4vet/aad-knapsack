# Memetic Algorithm for 0/1 Knapsack

## About the Algorithm

A **Memetic Algorithm (MA)** is a hybrid metaheuristic that combines the global search capabilities of an Evolutionary Algorithm (like the Genetic Algorithm) with the local refinement power of a Local Search technique. The name is inspired by Richard Dawkins' concept of a "meme"—a unit of cultural information that evolves and improves as it spreads.

In the context of the Knapsack Problem, this algorithm represents the state-of-the-art in heuristic solving. It doesn't just blindly evolve solutions; it mathematically reduces the problem size first and then uses "smart" agents that actively improve themselves using local search, rather than relying solely on random mutation.

## Implementation

### Modifications from Base GA

This implementation is a significant departure from the standard Genetic Algorithm. It incorporates advanced mathematical optimisation techniques to handle large-scale instances (millions of items) efficiently.

#### 1. Problem Reduction via Lagrangian Relaxation (Break Item Method)

The most critical optimisation happens *before* the genetic algorithm even starts. The Base GA operates on all $N$ items, which is inefficient for large $N$. The Memetic algorithm uses **Lagrangian Relaxation** to identify the "easy" decisions:

* **Break Item Method**: Instead of iterative subgradient optimisation, we determine the optimal Lagrangian multiplier $\lambda^*$ directly in $O(N \log N)$ time. Items are sorted by value-to-weight ratio, and the **Break Item**—the first item that doesn't fit when greedily filling the knapsack—defines $\lambda^* = v_b / w_b$. This is mathematically equivalent to finding the optimal dual variable in the Linear Relaxation of the Knapsack Problem.
* **Reduced Cost Calculation**: Using $\lambda^*$, each item's reduced cost is computed as $rc_i = v_i - \lambda^* \cdot w_i$. Items with positive reduced costs are "good" (tend towards inclusion), while negative reduced costs indicate "bad" items.
* **Variable Fixing**: Items are fixed based on their reduced costs. If excluding an item (fixing to $0$) or including an item (fixing to $1$) would result in a bound worse than the current lower bound, that item's decision is fixed.
* **The Core Problem**: The GA only evolves the remaining subset of items (the "Core") where the decision is difficult—items whose reduced costs are close to zero.

For a problem with 1,000,000 items, the Core might only contain 100–500 items. This allows the GA to converge extremely fast.

#### 2. Core-Based Individual Representation

The Base GA uses a bit string of length $N$ (one bit per item). The Memetic Algorithm uses a **Core-Based Representation**:

* **Reduced Bit String**: The `Individual` class only stores bits for the Core items, not all $N$ items.
* **Pre-loaded Fixed Values**: The constructor initialises `totalValue` and `totalWeight` with the contributions from items fixed to $1$, so the GA only evolves the "uncertain" part.
* **Index Mapping**: A `coreToOriginal` mapping is used to translate between core indices and original item indices.

#### 3. Fitness-Biased Crossover

The Base GA uses **Single-Point Crossover** (split at midpoint, swap halves). The Memetic Algorithm uses a **Fitness-Biased Uniform Crossover**:

* **Agreement Rule**: If both parents agree on a bit (both $0$ or both $1$), both children inherit that bit.
* **Disagreement Rule**: If parents disagree, the **fitter parent's bit is favoured** and given to Child 1, while the weaker parent's bit goes to Child 2.
* **Diversity Injection**: A random coin flip can swap the bits between children to maintain diversity.
* **Incremental Metrics**: Children's `totalValue` and `totalWeight` are computed incrementally during crossover, starting from `fixedValue` and `fixedWeight`.

This heuristic crossover ensures that good genetic material from the fitter parent is more likely to propagate, while still maintaining population diversity.

#### 4. Adaptive Core Mutation

The Base GA uses a **Per-Bit Mutation** where every bit has an independent probability of flipping (using geometric distribution for efficiency). The Memetic Algorithm uses an **Adaptive Core Mutation**:

* **Multi-Attempt Strategy**: Instead of checking every bit, the algorithm makes $3$ attempts to mutate, each with probability `coreMutationProb`.
* **Fast Random Index**: A random core index is selected directly using bit manipulation (`(rng() * coreSize) >> 32`), rather than iterating through all bits.
* **Incremental Updates**: When a bit is flipped, `totalValue` and `totalWeight` are updated immediately using the `coreToOriginal` mapping.

This is much more efficient for large problems since the mutation time is $O(1)$ regardless of core size.

#### 5. Lagrangian-Guided Repair

The Base GA repairs overweight individuals by **randomly removing items** until feasible. The Memetic Algorithm uses a **Two-Phase Greedy Repair** based on reduced costs:

* **Phase 1 (Removal)**: If overweight, items are removed in order of *worst* reduced cost (iterating backwards through `rcSortedCoreIndices`).
* **Phase 2 (Addition)**: Periodically (every 5 generations), items with the *best* reduced costs are greedily added back if space allows.

This ensures that even the repair process pushes the solution towards optimality rather than randomly degrading it.

#### 6. Memetic Local Search (Tabu Search)

In the Base GA, individuals only change via random mutation. In this Memetic Algorithm, individuals actively improve themselves using **Tabu Search**:

* After mutation, an individual undergoes a local search phase with probability `localSearchProb`.
* It explores its "neighbourhood" by sampling random bit flips and evaluating them.
* **Tabu List**: Recently flipped bits are marked "tabu" (forbidden) for a tenure period to prevent cycling.
* **Aspiration Criterion**: A tabu move is allowed if it leads to a new **best-ever** solution.
* **Backtracking**: The algorithm tracks all moves and can backtrack to the best solution found during the search.

#### 7. Elitism

The Base GA has **no elitism**—the best individual can be lost between generations. The Memetic Algorithm implements **Explicit Elitism**:

* The best individual from the current generation is **always copied** to position $0$ of the next generation.
* This guarantees monotonic improvement: the best solution found is never lost.

#### 8. Gap-Based Adaptive Scaling

The algorithm measures the **Duality Gap** (the difference between the best possible mathematical upper bound and the current best solution).

* **Small Gap** ($< 0.1\%$): The problem is easy. Local search probability and iterations are reduced to save time.
* **Moderate Gap** ($< 5\%$): Standard search intensity is applied.
* **Large Gap** ($> 5\%$): The algorithm automatically increases `tabuIterations` and `localSearchProb` to dig deeper.
* **Core Size Scaling**: For very large cores ($> 50,000$ items), parameters are scaled down to keep runtime manageable.

#### 9. Heuristic Population Seeding

The Base GA starts with a **fully random population**. The Memetic Algorithm seeds the initial population intelligently:

* **$\sim 5\%$ RCBO Individuals**: Generated using "Reduced Cost Based Ordering"—a heuristic derived from the Lagrangian relaxation that greedily adds items by reduced cost.
* **$\sim 5\%$ V/W Greedy Individuals**: Generated using the standard Value/Weight ratio heuristic.
* **$\sim 90\%$ Random Individuals**: The rest are random to maintain diversity.

All individuals are repaired with the aggressive greedy-add phase enabled for a strong initial population.

### Complexity

The complexity is split into two phases: Preprocessing and Evolution.

#### Preprocessing (Lagrangian Relaxation)

| Function | Time Complexity | Notes |
|----------|-----------------|-------|
| `performPreprocessing()` | $O(N \log N)$ | Sorting items by V/W ratio ($N \log N$) + single pass to find Break Item and compute reduced costs ($O(N)$). |

#### Core Evolution (The Genetic Part)

Because the GA only runs on the **Core** (size $C$), and $C \ll N$, the evolution is extremely fast regardless of the original problem size.

| Function | Time Complexity | Notes |
|----------|-----------------|-------|
| `Individual::repair()` | $O(C)$ | Linear scan of the Core items only. |
| `Individual::mutate()` | $O(1)$ | Fixed number of mutation attempts (3), regardless of Core size. |
| `crossover()` | $O(C)$ | Linear scan of Core items with incremental metric updates. |
| `Individual::localSearch()` | $O(I \cdot S)$ | $I$ iterations $\times$ $S$ neighbours. Constant relative to $N$. |
| `solveKnapsackGenetic()` | $O(G \cdot P \cdot (C + I \cdot S))$ | Generations $\times$ Population $\times$ (Crossover + Local Search). |

**Effective Complexity**: For large $N$, the runtime is dominated by the $O(N \log N)$ sorting in the preprocessing phase. The evolutionary part is effectively constant time $O(1)$ relative to $N$ because the Core size $C$ typically stays small (e.g., $\approx 200$) even as $N$ grows to billions.

### Summary of Improvements

| Feature | Base Genetic Algorithm | Memetic Algorithm |
| :--- | :--- | :--- |
| **Problem Scope** | Evolves all $N$ items | Evolves only "Core" items ($C \ll N$) |
| **Individual Representation** | Bit string of length $N$ | Bit string of length $C$ with pre-loaded fixed values |
| **Crossover** | Single-point (midpoint swap) | Fitness-biased uniform crossover |
| **Mutation** | Per-bit with geometric skip ($O(N)$) | Multi-attempt random flip ($O(1)$) |
| **Repair Strategy** | Random Removal | Two-phase Lagrangian Reduced Cost Greedy |
| **Local Improvement** | None (Random Mutation only) | **Tabu Search** (Active improvement) |
| **Elitism** | None | Best individual always preserved |
| **Population Seeding** | Fully random | Heuristic (RCBO + V/W Greedy) + Random |
| **Adaptive Parameters** | Fixed | Gap-based scaling of search intensity |
| **Scalability** | Slow for $N > 10,000$ | Efficient for $N > 1,000,000$ |

## References

1. **Chu, P. C., & Beasley, J. E. (1998).** *A Genetic Algorithm for the Multidimensional Knapsack Problem.* Journal of Heuristics. (The seminal paper on using GA with repair operators for Knapsack).
2. **Pisinger, D. (2005).** *Where are the hard knapsack problems?* Computers & Operations Research. (Discusses Core problems and hardness).
3. **Lagrangian Relaxation**: Standard technique in combinatorial optimisation to find upper bounds and reduce problem size.
