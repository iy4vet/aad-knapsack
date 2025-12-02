#include "../file_io.hpp"
#include <algorithm>
#include <chrono>
#include <random>
#include <limits>
#include <cstdint>

// Use int64 for large numbers.
using int64 = long long;

// ============================================================================
// Result Struct
// ============================================================================

// Struct to hold the results of the memetic algorithm.
struct Result {
    int64 maxValue;                     // The maximum value found for the knapsack.
    std::vector<size_t> selectedItems;  // The indices of the items selected to achieve the max value.
    int64 executionTime;                // Total execution time in microseconds.
    size_t memoryUsed;                  // Approximate memory usage in bytes.
};


// ============================================================================
// Hyperparameters Struct
// ============================================================================

// Struct to hold memetic algorithm hyperparameters.
struct Hyperparameters {
    size_t populationSize = 0;                  // Size of the population. If 0, set heuristically.
    size_t maxGenerations = 0;                  // Number of generations. If 0, set heuristically.
    double crossoverRate = 0.53;                // Probability of crossover.
    double mutationRate = 0.05;                 // Base mutation rate for the entire problem.
    double reproductionRate = 0.15;             // Probability of direct reproduction.
    double coreMutationProb = 0.05;             // Adaptive mutation rate for the core problem, scaled by reduction.
    unsigned int seed = std::random_device()(); // Seed for random number generator.
};


// ============================================================================
// Gap-Based Search Parameters Struct
// ============================================================================

// Struct for parameters that adapt based on the duality gap from Lagrangian relaxation.
struct GapBasedParams {
    size_t tabuIterations = 50;         // Number of iterations for the Tabu Search local search.
    size_t tabuNeighbourhoodSize = 20;  // Number of neighbours to sample in each Tabu Search iteration.
    double localSearchProb = 0.3;       // Probability of applying local search to an individual.
};

// ============================================================================
// Preprocessing & Heuristic Data Structures
// ============================================================================

// Struct to store data related to problem reduction.
struct ReductionData {
    std::vector<size_t> coreToOriginal;     // Maps core item indices back to their original indices.
    std::vector<size_t> fixedOnesIndices;   // Indices of items fixed to 1 (must be in the solution).
    int64 fixedValue = 0;   // Total value of items fixed to 1.
    int64 fixedWeight = 0;  // Total weight of items fixed to 1.
    size_t coreSize = 0;    // Number of items in the reduced (core) problem.
};

// Struct for sorting items, typically by value-to-weight ratio.
struct SortItem {
    size_t coreId;  // The index of the item within the core problem.
    double ratio;   // The value-to-weight ratio or other metric for sorting.
};

// Struct to hold data from the Lagrangian relaxation process.
struct LagrangianData {
    double multiplier = 0.0;                                        // The optimal Lagrangian multiplier (u*).
    int64 lowerBound = 0;                                           // The best feasible solution value found (LB).
    double upperBound = std::numeric_limits<double>::infinity();    // The Lagrangian upper bound (Z_u).
    double gapRatio = 1.0;                                          // The relative duality gap: (UB - LB) / UB.
};

// Struct containing all data generated during the preprocessing phase.
struct PreprocessingData {
    // Views for Greedy/Repair operators (Contain ONLY Core items)
    std::vector<SortItem> sortedCoreItems;      // Core items sorted by value-to-weight ratio (ascending).
    std::vector<size_t> rcSortedCoreIndices;    // Core item indices sorted by reduced cost (descending).

    // Cached reduced costs for core items only.
    std::vector<double> coreReducedCosts;

    // Lagrangian relaxation data.
    LagrangianData lagrangianInfo;

    // Solves the Lagrangian subproblem for a given multiplier u.
    // Returns the objective value of the relaxed problem and the total weight of selected items.
    std::pair<double, int64> lagrangianSubproblem(double u, const KnapsackInstance &instance) {
        double z_val = 0.0;
        int64 total_w = 0;
        for (size_t i = 0; i < instance.n; ++i) {
            // Calculate reduced cost: rc_i = v_i - u * w_i
            double rc = (double)instance.values[i] - u * (double)instance.weights[i];
            // Add item if its reduced cost is positive.
            if (rc > 0.0) {
                z_val += rc;
                total_w += instance.weights[i];
            }
        }
        return { z_val, total_w };
    }

    // Performs problem reduction using Lagrangian relaxation and subgradient optimisation.
    void performPreprocessing(const KnapsackInstance &instance, Hyperparameters &params, ReductionData &reduction) {
        size_t n = instance.n;
        // --- Phase 1: Lagrangian Relaxation ---
        // A. Establish Initial Lower Bound (Simple Greedy)
        // A good lower bound is crucial for effective reduction and gap calculation.
        std::vector<std::pair<double, size_t>> temp_ratios(n);
        for (size_t i = 0; i < n; ++i) {
            double r = (instance.weights[i] > 0) ? (double)instance.values[i] / instance.weights[i] : 0.0;
            temp_ratios[i] = { r, i };
        }
        std::sort(temp_ratios.rbegin(), temp_ratios.rend());
        // Compute greedy solution value.
        int64 current_w = 0;
        int64 current_v = 0;
        for (const auto &p : temp_ratios) {
            if (current_w + instance.weights[p.second] <= instance.capacity) {
                current_w += instance.weights[p.second];
                current_v += instance.values[p.second];
            }
        }
        lagrangianInfo.lowerBound = current_v;
        // B. Subgradient Optimisation to find the best Lagrangian multiplier.
        double u = 0.0;
        double step = 2.0;
        size_t max_iter = (n > 50000) ? 100 : 150; // Fewer iterations for very large instances.
        double best_zub = std::numeric_limits<double>::infinity();
        double best_u = 0.0;
        size_t no_improvement_count = 0;
        // Temporary storage for reduced costs (computed at best_u).
        std::vector<double> tempReducedCosts(n);
        // Subgradient loop.
        for (size_t k = 0; k < max_iter; ++k) {
            auto sub_result = lagrangianSubproblem(u, instance);
            double z_u = sub_result.first + u * (double)instance.capacity;
            // If we found a better upper bound, update it.
            if (z_u < best_zub - 1e-6) {
                best_zub = z_u;
                best_u = u;
                no_improvement_count = 0;
                // Update reduced costs at the best multiplier found so far.
                for (size_t i = 0; i < n; ++i) {
                    tempReducedCosts[i] = (double)instance.values[i] - best_u * (double)instance.weights[i];
                }
            }
            else {
                no_improvement_count++;
                // If the upper bound hasn't improved for a while, reduce the step size.
                if (no_improvement_count >= 15) {
                    step *= 0.5;
                    no_improvement_count = 0;
                }
            }
            // Check for convergence (gap is very small).
            if (best_zub > 0.0 && (best_zub - (double)lagrangianInfo.lowerBound) / best_zub < 1e-8) {
                break;
            }
            // Converged if subgradient is zero.
            int64 g = sub_result.second - instance.capacity;
            if (g == 0 && k > 10) {
                break;
            }
            // Update the multiplier using the subgradient method.
            if (g != 0) {
                double numerator = z_u - (double)lagrangianInfo.lowerBound;
                // Avoid division by zero or tiny steps.
                if (numerator < 1e-6) {
                    numerator = 1e-6;
                }
                // Correct: include subgradient direction (sign-aware update).
                u = std::max(0.0, u + step * numerator / (double)g);
            }
            step *= 0.95; // Gradually decrease step size.
        }
        // Store final Lagrangian data.
        lagrangianInfo.multiplier = best_u;
        lagrangianInfo.upperBound = best_zub;
        // Compute the duality gap ratio for adaptive search intensity.
        if (best_zub > 0.0 && best_zub < std::numeric_limits<double>::infinity()) {
            lagrangianInfo.gapRatio = (best_zub - (double)lagrangianInfo.lowerBound) / best_zub;
            if (lagrangianInfo.gapRatio < 0.0) {
                lagrangianInfo.gapRatio = 0.0;
            }
        }
        // Default to max gap if bounds are invalid.
        else {
            lagrangianInfo.gapRatio = 1.0;
        }
        // C. Classification & Problem Reduction
        // Fix variables based on their reduced costs.
        reduction.fixedValue = 0;
        reduction.fixedWeight = 0;
        reduction.fixedOnesIndices.clear();
        reduction.coreToOriginal.clear();
        coreReducedCosts.clear();
        // Classify each item.
        for (size_t i = 0; i < n; ++i) {
            double rc = tempReducedCosts[i];
            // Condition to fix an item to 1.
            if (rc > 0 && (best_zub - rc < (double)lagrangianInfo.lowerBound - 1e-9)) {
                // Fixed to 1
                reduction.fixedOnesIndices.push_back(i);
                reduction.fixedValue += instance.values[i];
                reduction.fixedWeight += instance.weights[i];
            }
            // Condition to fix an item to 0.
            else if (rc < 0 && (best_zub + rc < (double)lagrangianInfo.lowerBound - 1e-9)) {
                // Fixed to 0 (do nothing, just exclude from core).
            }
            // Otherwise, the item is part of the core problem.
            else {
                reduction.coreToOriginal.push_back(i);
                coreReducedCosts.push_back(rc);
            }
        }
        // Finalize core size.
        reduction.coreSize = reduction.coreToOriginal.size();
        // --- Phase 2: Create Core-Only Sorted Views ---
        // No core problem to solve.
        if (reduction.coreSize == 0) {
            return;
        }
        // Setup sorted views for core items.
        sortedCoreItems.resize(reduction.coreSize);
        rcSortedCoreIndices.resize(reduction.coreSize);
        // Initialize sorting structures.
        for (size_t k = 0; k < reduction.coreSize; ++k) {
            size_t originalIdx = reduction.coreToOriginal[k];
            // Setup V/W Sort Item for repair operator.
            sortedCoreItems[k].coreId = k;
            if (instance.weights[originalIdx] > 0) {
                sortedCoreItems[k].ratio = static_cast<double>(instance.values[originalIdx]) / instance.weights[originalIdx];
            }
            else {
                sortedCoreItems[k].ratio = (instance.values[originalIdx] > 0) ? std::numeric_limits<double>::infinity() : 0.0;
            }
            // Setup index for reduced cost sorting.
            rcSortedCoreIndices[k] = k;
        }
        // Sort core items by V/W Ratio Ascending (for drop phase of repair - worst items first).
        std::sort(sortedCoreItems.begin(), sortedCoreItems.end(),
            [&instance, &reduction](const SortItem &a, const SortItem &b) {
                // Primary sort by ratio.
                if (a.ratio != b.ratio) {
                    return a.ratio < b.ratio;
                }
                // Tie-breaker: prefer smaller weight for removal.
                return instance.weights[reduction.coreToOriginal[a.coreId]] <
                    instance.weights[reduction.coreToOriginal[b.coreId]];
            });
        // Sort core indices by Reduced Cost Descending (Best to Worst for add phase of repair).
        std::sort(rcSortedCoreIndices.begin(), rcSortedCoreIndices.end(),
            [this](size_t a, size_t b) {
                return coreReducedCosts[a] > coreReducedCosts[b];
            });
        // Scale mutation probability based on the size of the core problem.
        // A smaller core means each item is more critical, so mutation can be higher.
        double scale = (reduction.coreSize > 0) ? (double)n / reduction.coreSize : 1.0;
        params.coreMutationProb = std::min(0.5, params.mutationRate * scale);
    }
};

// ============================================================================
// Global State
// ============================================================================

// Global knapsack instance, parameters, and auxiliary data structures.
KnapsackInstance INSTANCE;
Hyperparameters PARAMS;
PreprocessingData PREPROC_DATA;
ReductionData REDUCTION_DATA;
GapBasedParams GAP_PARAMS;

// Global random number generator.
std::mt19937 rng;


// ============================================================================
// Individual Class (Core-Based)
// ============================================================================

// Represents an individual in the population for the core problem.
// The bit string only contains items in the reduced (core) set.
class Individual {
public:
    std::vector<bool> bits;             // Bit string for core items. True means the item is included.
    int64 totalValue = 0;               // Cached total value of selected items (including fixed items).
    int64 totalWeight = 0;              // Cached total weight of selected items (including fixed items).
    mutable int64 cachedFitness = -1;   // Cached fitness to avoid re-computation.
    mutable bool fitnessValid = false;  // Flag to check if the cached fitness is valid.

    // Constructor: Initializes an individual for the core problem.
    // The value and weight from items fixed to 1 are pre-loaded.
    Individual(size_t coreSize) : bits(coreSize, false) {
        totalValue = REDUCTION_DATA.fixedValue;
        totalWeight = REDUCTION_DATA.fixedWeight;
    }

    // Calculates the fitness of the individual.
    // Fitness is the total value, or 0 if the solution is infeasible (overweight).
    int64 getFitness() const {
        if (fitnessValid) {
            return cachedFitness;
        }
        if (totalWeight > INSTANCE.capacity) {
            cachedFitness = 0;
        }
        else {
            cachedFitness = totalValue;
        }
        fitnessValid = true;
        return cachedFitness;
    }

    // Repairs an individual to be feasible using information from Lagrangian relaxation.
    // This is a two-phase process: removal of bad items, then optional addition of good items.
    void repair(bool tryGreedyAdd) {
        // Phase 1: Removal - Remove items with the lowest (worst) reduced cost until feasible.
        const auto &sortedRC = PREPROC_DATA.rcSortedCoreIndices;
        // Iterate backwards through the reduced-cost-sorted indices (worst to best).
        for (size_t i = bits.size(); i > 0 && totalWeight > INSTANCE.capacity; --i) {
            size_t coreIdx = sortedRC[i - 1];  // Get core index of the item with the worst RC.
            if (bits[coreIdx]) {
                size_t originalIdx = REDUCTION_DATA.coreToOriginal[coreIdx];
                bits[coreIdx] = false;
                totalWeight -= INSTANCE.weights[originalIdx];
                totalValue -= INSTANCE.values[originalIdx];
                fitnessValid = false;
            }
        }
        // Phase 2: Greedy Re-addition - Add high-RC items if space allows.
        // This is done periodically to explore adding good items back.
        if (tryGreedyAdd) {
            int64 remaining = INSTANCE.capacity - totalWeight;
            // Iterate forward through the reduced-cost-sorted indices (best to worst).
            for (size_t coreIdx : sortedRC) {
                if (!bits[coreIdx]) {
                    size_t originalIdx = REDUCTION_DATA.coreToOriginal[coreIdx];
                    if (INSTANCE.weights[originalIdx] <= remaining) {
                        bits[coreIdx] = true;
                        totalWeight += INSTANCE.weights[originalIdx];
                        totalValue += INSTANCE.values[originalIdx];
                        remaining -= INSTANCE.weights[originalIdx];
                        fitnessValid = false;
                    }
                }
            }
        }
    }

    // Applies mutation to the individual's core bit string.
    void mutate() {
        // No mutation if there are no core items.
        if (bits.empty()) {
            return;
        }
        const size_t coreSize = bits.size();
        // Use a threshold for mutation probability for performance.
        const uint32_t threshold = static_cast<uint32_t>(PARAMS.coreMutationProb * 65536);
        bool mutated = false;
        // Try to mutate a few times to increase chance of a flip.
        for (size_t attempt = 0; attempt < 3; ++attempt) {
            // Check probability first to save a random number generation call.
            if ((rng() & 0xFFFF) < threshold) {
                // Fast random index generation.
                size_t coreIdx = (static_cast<uint64_t>(rng()) * coreSize) >> 32;
                size_t originalIdx = REDUCTION_DATA.coreToOriginal[coreIdx];
                // Flip the bit and update metrics incrementally.
                if (bits[coreIdx]) {
                    totalValue -= INSTANCE.values[originalIdx];
                    totalWeight -= INSTANCE.weights[originalIdx];
                }
                else {
                    totalValue += INSTANCE.values[originalIdx];
                    totalWeight += INSTANCE.weights[originalIdx];
                }
                bits[coreIdx].flip();
                mutated = true;
            }
        }
        // Invalidate fitness cache if mutated.
        if (mutated) {
            fitnessValid = false;
        }
    }

    // Performs local search using Tabu Search on the core problem.
    void localSearch() {
        // No local search if there are no core items or tabu iterations is zero.
        if (bits.empty() || GAP_PARAMS.tabuIterations == 0) {
            return;
        }
        // Initialize Tabu Search structures.
        const size_t coreSize = bits.size();
        const size_t tabuTenureBase = 7;
        // Tabu list is indexed by core indices, storing the iteration when a move becomes non-tabu.
        std::vector<size_t> tabuList(coreSize, 0);
        // Keep track of the best solution found during the local search.
        int64 bestValue = totalValue;
        int64 bestWeight = totalWeight;
        size_t bestIteration = 0;
        // Store the sequence of moves to allow backtracking to the best solution.
        std::vector<size_t> moveHistory;
        moveHistory.reserve(GAP_PARAMS.tabuIterations);
        for (size_t iteration = 1; iteration <= GAP_PARAMS.tabuIterations; ++iteration) {
            size_t bestMovePos = coreSize;  // Sentinel for no move found.
            int64 bestMoveValue = std::numeric_limits<int64>::min();
            int64 bestMoveWeight = 0;
            // Sample a neighbourhood of potential moves (bit flips).
            for (size_t sample = 0; sample < GAP_PARAMS.tabuNeighbourhoodSize; ++sample) {
                size_t coreIdx = (static_cast<uint64_t>(rng()) * coreSize) >> 32;
                size_t originalIdx = REDUCTION_DATA.coreToOriginal[coreIdx];
                // Evaluate the move (delta evaluation).
                int64 weightDelta = bits[coreIdx] ? -INSTANCE.weights[originalIdx] : INSTANCE.weights[originalIdx];
                int64 valueDelta = bits[coreIdx] ? -INSTANCE.values[originalIdx] : INSTANCE.values[originalIdx];
                int64 newWeight = totalWeight + weightDelta;
                int64 newValue = totalValue + valueDelta;
                // A move is valid if it's feasible.
                if (newWeight > INSTANCE.capacity) {
                    continue;
                }
                bool isTabu = (tabuList[coreIdx] > iteration);
                // Aspiration criterion: a tabu move is allowed if it leads to a new best-ever solution.
                bool aspirationMet = (newValue > bestValue);
                if (!isTabu || aspirationMet) {
                    if (newValue > bestMoveValue) {
                        bestMoveValue = newValue;
                        bestMoveWeight = newWeight;
                        bestMovePos = coreIdx;
                    }
                }
            }
            // If sampling didn't find a move, do a fallback full search.
            if (bestMovePos == coreSize) {
                for (size_t coreIdx = 0; coreIdx < coreSize; ++coreIdx) {
                    size_t originalIdx = REDUCTION_DATA.coreToOriginal[coreIdx];
                    int64 weightDelta = bits[coreIdx] ? -INSTANCE.weights[originalIdx] : INSTANCE.weights[originalIdx];
                    int64 newWeight = totalWeight + weightDelta;
                    // First non-tabu improving move.
                    if (newWeight <= INSTANCE.capacity && tabuList[coreIdx] <= iteration) {
                        int64 valueDelta = bits[coreIdx] ? -INSTANCE.values[originalIdx] : INSTANCE.values[originalIdx];
                        bestMoveValue = totalValue + valueDelta;
                        bestMoveWeight = newWeight;
                        bestMovePos = coreIdx;
                        break;
                    }
                }
            }
            // If still no move is found, terminate local search.
            if (bestMovePos == coreSize) {
                break;
            }
            // Apply the best move found.
            bits[bestMovePos] = !bits[bestMovePos];
            totalWeight = bestMoveWeight;
            totalValue = bestMoveValue;
            fitnessValid = false;
            moveHistory.push_back(bestMovePos);
            // Update the tabu list for the move, adding a randomized tenure.
            size_t tenure = tabuTenureBase + (rng() & 3);
            tabuList[bestMovePos] = iteration + tenure;
            // Update the best-so-far solution if this move improved it.
            if (totalValue > bestValue) {
                bestValue = totalValue;
                bestWeight = totalWeight;
                bestIteration = moveHistory.size();
            }
        }
        // Restore the best solution found during the search by undoing moves.
        if (bestIteration < moveHistory.size()) {
            for (size_t i = moveHistory.size(); i > bestIteration; --i) {
                size_t pos = moveHistory[i - 1];
                bits[pos] = !bits[pos]; // Undo the flip.
            }
            totalValue = bestValue;
            totalWeight = bestWeight;
            fitnessValid = false;
        }
    }
};

// ============================================================================
// Population Generation
// ============================================================================

// Generates a random individual for the core problem.
Individual generateRandomIndividual() {
    Individual ind(REDUCTION_DATA.coreSize);
    size_t i = 0;
    // Set bits randomly using batched random bits from rng() for efficiency.
    while (i < REDUCTION_DATA.coreSize) {
        uint32_t randBits = rng();
        for (int b = 0; b < 32 && i < REDUCTION_DATA.coreSize; ++b, ++i) {
            if (randBits & (1u << b)) {
                size_t originalIdx = REDUCTION_DATA.coreToOriginal[i];
                ind.bits[i] = true;
                ind.totalWeight += INSTANCE.weights[originalIdx];
                ind.totalValue += INSTANCE.values[originalIdx];
            }
        }
    }
    return ind;
}

// Generates a greedy individual based on value-to-weight ratio.
Individual generateVWGreedyIndividual() {
    Individual ind(REDUCTION_DATA.coreSize);
    // Iterate in reverse (highest V/W first, as items are sorted ascending by ratio).
    for (size_t i = REDUCTION_DATA.coreSize; i > 0; --i) {
        size_t coreId = PREPROC_DATA.sortedCoreItems[i - 1].coreId;
        size_t originalId = REDUCTION_DATA.coreToOriginal[coreId];
        if (ind.totalWeight + INSTANCE.weights[originalId] <= INSTANCE.capacity) {
            ind.bits[coreId] = true;
            ind.totalWeight += INSTANCE.weights[originalId];
            ind.totalValue += INSTANCE.values[originalId];
        }
    }
    return ind;
}

// Generates a greedy individual based on reduced cost (RCBO - Reduced Cost Based Ordering).
Individual generateRCBOIndividual() {
    Individual ind(REDUCTION_DATA.coreSize);
    // Iterate through core items sorted by descending reduced cost (best first).
    for (size_t coreIdx : PREPROC_DATA.rcSortedCoreIndices) {
        size_t originalId = REDUCTION_DATA.coreToOriginal[coreIdx];
        if (ind.totalWeight + INSTANCE.weights[originalId] <= INSTANCE.capacity) {
            ind.bits[coreIdx] = true;
            ind.totalWeight += INSTANCE.weights[originalId];
            ind.totalValue += INSTANCE.values[originalId];
        }
    }
    return ind;
}

// Generates the initial population with a mix of heuristic and random individuals.
std::vector<Individual> generateInitialPopulation() {
    std::vector<Individual> population;
    population.reserve(PARAMS.populationSize);

    // Seed ~5% of the population with RCBO individuals.
    size_t numRCBO = std::max(size_t(1), PARAMS.populationSize / 20);
    Individual rcbo = generateRCBOIndividual();
    for (size_t p = 0; p < numRCBO; ++p) {
        population.push_back(rcbo);
    }

    // Seed ~5% of the population with V/W greedy individuals.
    size_t numVWGreedy = std::max(size_t(1), PARAMS.populationSize / 20);
    Individual vwGreedy = generateVWGreedyIndividual();
    for (size_t p = 0; p < numVWGreedy; ++p) {
        population.push_back(vwGreedy);
    }

    // Fill the rest of the population with random individuals to ensure diversity.
    while (population.size() < PARAMS.populationSize) {
        population.push_back(generateRandomIndividual());
    }

    // Repair all individuals in the initial population.
    // The first repair is aggressive (with greedy add) to ensure a strong start.
    for (auto &individual : population) {
        individual.repair(true);
    }

    return population;
}

// ============================================================================
// Genetic Operators
// ============================================================================

// Selects two parent individuals from the population using tournament selection.
std::pair<const Individual *, const Individual *> selection(const std::vector<Individual> &population) {
    // Tournament selection: select the best of two random individuals, twice.
    std::uniform_int_distribution<size_t> indexDist(0, PARAMS.populationSize - 1);
    // Select four random, distinct individuals for two tournaments.
    size_t idx1 = indexDist(rng);
    size_t idx2 = indexDist(rng);
    size_t idx3 = indexDist(rng);
    size_t idx4 = indexDist(rng);
    // Ensure indices are distinct to avoid selecting the same individual multiple times.
    while (idx2 == idx1) {
        idx2 = (idx2 + 1) % PARAMS.populationSize;
    }
    while (idx3 == idx1 || idx3 == idx2) {
        idx3 = (idx3 + 1) % PARAMS.populationSize;
    }
    while (idx4 == idx1 || idx4 == idx2 || idx4 == idx3) {
        idx4 = (idx4 + 1) % PARAMS.populationSize;
    }
    // Tournament 1: The fitter of the first two individuals wins.
    const Individual *parent1 = &population[idx1];
    const Individual *parent2 = &population[idx2];
    if (parent2->getFitness() > parent1->getFitness()) {
        parent1 = parent2;
    }
    // Tournament 2: The fitter of the second two individuals wins.
    const Individual *parent3 = &population[idx3];
    const Individual *parent4 = &population[idx4];
    if (parent4->getFitness() > parent3->getFitness()) {
        parent3 = parent4;
    }
    // Return the two selected parents.
    return { parent1, parent3 };
}

// Performs crossover on two parents to produce two children.
// This implementation uses a heuristic approach that favors genes from the fitter parent.
void crossover(const Individual &parent1, const Individual &parent2, Individual &child1, Individual &child2) {
    // References for easier access.
    const std::vector<bool> &p1 = parent1.bits;
    const std::vector<bool> &p2 = parent2.bits;
    std::vector<bool> &c1 = child1.bits;
    std::vector<bool> &c2 = child2.bits;
    // Reset children's metrics for incremental calculation.
    int64 v1 = REDUCTION_DATA.fixedValue, w1 = REDUCTION_DATA.fixedWeight;
    int64 v2 = REDUCTION_DATA.fixedValue, w2 = REDUCTION_DATA.fixedWeight;
    // Determine which parent is fitter.
    bool p1Better = (parent1.getFitness() >= parent2.getFitness());
    size_t n = REDUCTION_DATA.coreSize;
    size_t i = 0;
    // Process bits in batches for efficiency.
    while (i < n) {
        uint32_t randBits = rng();
        for (int b = 0; b < 32 && i < n; ++b, ++i) {
            bool bit1, bit2;
            // If parents agree on a bit, both children inherit it.
            if (p1[i] == p2[i]) {
                bit1 = p1[i];
                bit2 = p1[i];
            }
            // If parents disagree, the fitter parent's bit is favored.
            else {
                if (p1Better) {
                    bit1 = p1[i]; bit2 = p2[i];
                }
                else {
                    bit1 = p2[i]; bit2 = p1[i];
                }
                // Introduce a chance to swap the bits to maintain diversity.
                if (randBits & (1u << b)) {
                    std::swap(bit1, bit2);
                }
            }
            c1[i] = bit1;
            c2[i] = bit2;
            // Update metrics incrementally.
            if (bit1) {
                size_t oIdx = REDUCTION_DATA.coreToOriginal[i];
                v1 += INSTANCE.values[oIdx];
                w1 += INSTANCE.weights[oIdx];
            }
            if (bit2) {
                size_t oIdx = REDUCTION_DATA.coreToOriginal[i];
                v2 += INSTANCE.values[oIdx];
                w2 += INSTANCE.weights[oIdx];
            }
        }
    }
    // Set children's total value and weight.
    child1.totalValue = v1; child1.totalWeight = w1; child1.fitnessValid = false;
    child2.totalValue = v2; child2.totalWeight = w2; child2.fitnessValid = false;
}

// Evolves the population to the next generation.
void nextGeneration(const std::vector<Individual> &currentPop, std::vector<Individual> &nextPop, size_t genCounter) {
    // Distribution for probabilistic decisions.
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    // Elitism: The best individual from the current generation is guaranteed to survive.
    auto bestIt = std::max_element(currentPop.begin(), currentPop.end(),
        [](const Individual &a, const Individual &b) { return a.getFitness() < b.getFitness(); });
    nextPop[0] = *bestIt;
    // Determine if the repair function should be aggressive (use greedy add) this generation.
    // This is done periodically to re-introduce good items.
    bool greedyRepair = (genCounter % 5 == 0);
    // Fill the rest of the next population.
    for (size_t i = 1; i < PARAMS.populationSize; ) {
        // Parent Selection
        auto [parent1, parent2] = selection(currentPop);
        // Reproduction: A small portion of the population is copied directly.
        if (probDist(rng) < PARAMS.reproductionRate) {
            nextPop[i] = *parent1;
            nextPop[i].repair(greedyRepair);
            i++;
            if (i < PARAMS.populationSize) {
                nextPop[i] = *parent2;
                nextPop[i].repair(greedyRepair);
                i++;
            }
        }
        // Crossover and Mutation
        else {
            // Create children references.
            Individual &child1 = nextPop[i];
            Individual &child2 = (i + 1 < PARAMS.populationSize) ? nextPop[i + 1] : child1;
            // Crossover
            if (probDist(rng) < PARAMS.crossoverRate) {
                crossover(*parent1, *parent2, child1, child2);
            }
            // If no crossover, children are clones of parents before mutation.
            else {
                child1 = *parent1;
                if (i + 1 < PARAMS.populationSize) child2 = *parent2;
            }
            // Apply mutation, repair, and potentially local search to the first child.
            child1.mutate();
            child1.repair(greedyRepair);
            if (probDist(rng) < GAP_PARAMS.localSearchProb) {
                child1.localSearch();
            }
            i++;
            // Do the same for the second child if there is space.
            if (i < PARAMS.populationSize && &child1 != &child2) {
                child2.mutate();
                child2.repair(greedyRepair);
                if (probDist(rng) < GAP_PARAMS.localSearchProb) {
                    child2.localSearch();
                }
                i++;
            }
        }
    }
}

// ============================================================================
// Gap-Based Scaling
// ============================================================================

// Configures the intensity of the local search based on the duality gap.
// A larger gap suggests the problem is harder, so a more intensive search is warranted.
void configureSearchParameters() {
    // Retrieve gap ratio and core size.
    double gapRatio = PREPROC_DATA.lagrangianInfo.gapRatio;
    size_t coreSize = REDUCTION_DATA.coreSize;
    // Scale local search probability and intensity based on the gap size.
    if (gapRatio < 0.001) {
        // Very small gap: problem is nearly solved, minimal local search needed.
        GAP_PARAMS.localSearchProb = 0.05;
        GAP_PARAMS.tabuIterations = 20;
        GAP_PARAMS.tabuNeighbourhoodSize = 10;
    }
    else if (gapRatio < 0.01) {
        // Small gap: moderate search.
        GAP_PARAMS.localSearchProb = 0.15;
        GAP_PARAMS.tabuIterations = 30;
        GAP_PARAMS.tabuNeighbourhoodSize = 15;
    }
    else if (gapRatio < 0.05) {
        // Moderate gap: more intensive search.
        GAP_PARAMS.localSearchProb = 0.25;
        GAP_PARAMS.tabuIterations = 50;
        GAP_PARAMS.tabuNeighbourhoodSize = 20;
    }
    else {
        // Large gap: problem is likely hard, use most intensive search settings.
        GAP_PARAMS.localSearchProb = 0.35;
        GAP_PARAMS.tabuIterations = 70;
        GAP_PARAMS.tabuNeighbourhoodSize = 30;
    }
    // Scale down parameters for very large core problems to keep runtime manageable.
    if (coreSize > 50000) {
        // Large core: reduce local search intensity.
        GAP_PARAMS.localSearchProb *= 0.5;
        GAP_PARAMS.tabuIterations = std::min(GAP_PARAMS.tabuIterations, size_t(30));
        GAP_PARAMS.tabuNeighbourhoodSize = std::min(GAP_PARAMS.tabuNeighbourhoodSize, size_t(15));
    }
    else if (coreSize > 20000) {
        // Moderately large core: slightly reduce local search intensity.
        GAP_PARAMS.localSearchProb *= 0.7;
        GAP_PARAMS.tabuIterations = std::min(GAP_PARAMS.tabuIterations, size_t(40));
    }
    // If the gap is effectively zero, the problem is likely solved, so disable local search.
    if (gapRatio < 1e-8) {
        // Set local search parameters to zero.
        GAP_PARAMS.localSearchProb = 0.0;
        GAP_PARAMS.tabuIterations = 0;
    }
}


// ============================================================================
// Main Solver
// ============================================================================

// Main memetic algorithm function to solve the knapsack problem.
Result solveKnapsackGenetic() {
    Result result;
    auto start = std::chrono::high_resolution_clock::now();

    // 1. Preprocessing and Problem Reduction.
    // This phase uses Lagrangian relaxation to fix variables and identify a smaller "core" problem.
    PREPROC_DATA.performPreprocessing(INSTANCE, PARAMS, REDUCTION_DATA);

    // If the core problem is empty, the solution is just the sum of items fixed to 1.
    if (REDUCTION_DATA.coreSize == 0) {
        result.maxValue = REDUCTION_DATA.fixedValue;
        result.selectedItems = REDUCTION_DATA.fixedOnesIndices;
        std::sort(result.selectedItems.begin(), result.selectedItems.end());
        auto end = std::chrono::high_resolution_clock::now();
        result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        result.memoryUsed = 0;
        return result;
    }

    // 2. Configure adaptive search parameters based on the duality gap.
    configureSearchParameters();

    // Check if a simple greedy solution on the core problem is already optimal.
    // This can happen if the Lagrangian relaxation provides a very tight upper bound.
    Individual greedySeed = generateRCBOIndividual();
    greedySeed.repair(true);
    // If the greedy solution matches or exceeds the upper bound, return it.
    if (PREPROC_DATA.lagrangianInfo.upperBound < std::numeric_limits<double>::infinity() &&
        greedySeed.getFitness() >= (int64)std::ceil(PREPROC_DATA.lagrangianInfo.upperBound)) {
        // Get the greedy solution as the final result.
        result.maxValue = greedySeed.getFitness();
        // Reconstruct solution from the greedy individual.
        result.selectedItems = REDUCTION_DATA.fixedOnesIndices;
        for (size_t i = 0; i < REDUCTION_DATA.coreSize; ++i) {
            if (greedySeed.bits[i]) {
                result.selectedItems.push_back(REDUCTION_DATA.coreToOriginal[i]);
            }
        }
        // Items must be sorted for output.
        std::sort(result.selectedItems.begin(), result.selectedItems.end());
        auto end = std::chrono::high_resolution_clock::now();
        result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        result.memoryUsed = sizeof(Individual) + (REDUCTION_DATA.coreSize + 7) / 8;
        return result;
    }

    // 3. Evolution on the Core Problem.
    std::vector<Individual> population = generateInitialPopulation();
    std::vector<Individual> nextPopulation(PARAMS.populationSize, Individual(REDUCTION_DATA.coreSize));

    for (size_t gen = 0; gen < PARAMS.maxGenerations; ++gen) {
        nextGeneration(population, nextPopulation, gen + 1);
        population.swap(nextPopulation);
    }

    // Find the best individual in the final population.
    Individual bestIndividual = *std::max_element(population.begin(), population.end(),
        [](const Individual &a, const Individual &b) { return a.getFitness() < b.getFitness(); });

    auto end = std::chrono::high_resolution_clock::now();
    result.maxValue = bestIndividual.getFitness();
    result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // 4. Reconstruct Final Solution.
    // Combine the items fixed to 1 with the solution found for the core problem.
    result.selectedItems = REDUCTION_DATA.fixedOnesIndices;
    for (size_t i = 0; i < REDUCTION_DATA.coreSize; ++i) {
        if (bestIndividual.bits[i]) {
            result.selectedItems.push_back(REDUCTION_DATA.coreToOriginal[i]);
        }
    }
    std::sort(result.selectedItems.begin(), result.selectedItems.end());

    // Approximate memory usage.
    size_t populationMemory = 0;
    for (const auto &ind : population) populationMemory += sizeof(Individual) + (ind.bits.capacity() + 7) / 8;
    result.memoryUsed = populationMemory + (sizeof(int64) * INSTANCE.weights.size() * 2);

    return result;
}


// ============================================================================
// Entry Point
// ============================================================================

// Parses command-line arguments to override default hyperparameters.
std::string parseArguments(int argc, char *argv[]) {
    std::string filepath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--population_size" && i + 1 < argc) {
            PARAMS.populationSize = std::stoul(argv[++i]);
        }
        else if (arg == "--max_generations" && i + 1 < argc) {
            PARAMS.maxGenerations = std::stoul(argv[++i]);
        }
        else if (arg == "--crossover_rate" && i + 1 < argc) {
            PARAMS.crossoverRate = std::stod(argv[++i]);
        }
        else if (arg == "--mutation_rate" && i + 1 < argc) {
            PARAMS.mutationRate = std::stod(argv[++i]);
        }
        else if (arg == "--reproduction_rate" && i + 1 < argc) {
            PARAMS.reproductionRate = std::stod(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc) {
            PARAMS.seed = static_cast<unsigned int>(std::stoi(argv[++i]));
        }
        else if (arg[0] != '-' && filepath.empty()) {
            filepath = arg;
        }
    }
    return filepath;
}

int main(int argc, char *argv[]) {
    // Use fast I/O.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Parse command-line arguments.
    std::string filepath = parseArguments(argc, argv);
    if (filepath.empty()) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [options]" << std::endl;
        return 1;
    }

    // Seed the random number generator.
    rng.seed(PARAMS.seed);

    // Load problem instance from file.
    if (!loadKnapsackInstance(filepath, INSTANCE)) {
        return 1;
    }

    // Set default population size if not provided.
    if (PARAMS.populationSize == 0) {
        PARAMS.populationSize = 60;
    }
    // Set default max generations if not provided.
    if (PARAMS.maxGenerations == 0) {
        PARAMS.maxGenerations = 30;
    }

    // Solve the knapsack problem.
    Result result = solveKnapsackGenetic();

    // Print the results in the required format.
    std::cout << result.maxValue << "\n";
    std::cout << result.selectedItems.size() << "\n";
    if (!result.selectedItems.empty()) {
        for (size_t i = 0; i < result.selectedItems.size(); ++i) {
            std::cout << result.selectedItems[i] << (i == result.selectedItems.size() - 1 ? "" : " ");
        }
        std::cout << "\n";
    }
    std::cout << result.executionTime << "\n";
    std::cout << result.memoryUsed << "\n";

    return 0;
}
