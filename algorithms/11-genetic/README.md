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
