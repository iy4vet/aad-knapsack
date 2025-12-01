#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <numeric>
#include <cstring>
#include <deque>
#include "../file_io.hpp"

// Use int64 for large numbers.
using int64 = long long;


// ============================================================================
// Result Struct
// ============================================================================

// Struct to hold the results of the genetic algorithm.
struct Result {
    int64 maxValue;                     // The maximum value found for the knapsack.
    std::vector<size_t> selectedItems;  // The indices of the items selected to achieve the max value.
    int64 executionTime;                // Total execution time in microseconds.
    size_t memoryUsed;                  // Approximate memory usage in bytes.
};


// ============================================================================
// Hyperparameters Struct
// ============================================================================

// Struct to hold genetic algorithm hyperparameters.
struct Hyperparameters {
    size_t populationSize = 0;                  // Size of the population. If 0, set heuristically.
    size_t maxGenerations = 0;                  // Number of generations. If 0, set heuristically.
    double crossoverRate = 0.53;                // Probability of crossover.
    double mutationRate = 0.013;                // Probability of mutation.
    double reproductionRate = 0.15;             // Probability of direct reproduction.
    unsigned int seed = std::random_device()(); // Seed for random number generator.
};


// ============================================================================
// Pre-sorting and Greedy-related Struct
// ============================================================================

// Struct to hold item properties for sorting.
struct ItemProperty {
    size_t id;
    double ratio;
};

// Struct for pre-sorted items and greedy-related data.
struct SortedItemsData {
    std::vector<ItemProperty> items;    // Pre-sorted list of items by value-to-weight ratio.

    // Pre-sorts items by their value-to-weight ratio in ascending order.
    void preSortItems(const KnapsackInstance &instance) {
        size_t n = instance.n;
        items.resize(n);

        // Compute value-to-weight ratio for each item. Handle zero weight case.
        for (size_t i = 0; i < n; ++i) {
            items[i].id = i;
            if (instance.weights[i] > 0) {
                items[i].ratio = static_cast<double>(instance.values[i]) / instance.weights[i];
            }
            else if (instance.values[i] > 0) {
                items[i].ratio = std::numeric_limits<double>::infinity();
            }
            else {
                items[i].ratio = 0.0;
            }
        }

        // Sort items by their value-to-weight ratio in ascending order.
        // In case of a tie, prefer items with less weight.
        std::sort(
            items.begin(),
            items.end(),
            [&instance](const ItemProperty &a, const ItemProperty &b) {
                // Primary: compare by value-to-weight ratio
                if (a.ratio != b.ratio) {
                    return a.ratio < b.ratio;
                }
                // Tiebreaker: prefer lower weight
                return instance.weights[a.id] < instance.weights[b.id];
            }
        );
    }
};


// ============================================================================
// Selection Data Struct
// ============================================================================

// Struct for selection-related data structures.
struct SelectionData {
    std::vector<size_t> populationRanks;    // Indices sorted by fitness (best first).

    // Pre-computes ranking data structures.
    // Called once per generation. O(n log n) for sorting.
    template <typename Population>
    void precomputeRanks(const Population &population) {
        size_t popSize = population.size();
        populationRanks.resize(popSize);
        std::iota(populationRanks.begin(), populationRanks.end(), 0);

        // Sort indices by fitness in descending order (best first).
        std::sort(populationRanks.begin(), populationRanks.end(),
            [&](size_t a, size_t b) {
                return population[a].getFitness() > population[b].getFitness();
            });
    }
};


// ============================================================================
// Diversity Tracking Struct
// ============================================================================

// Struct for diversity tracking variables.
struct DiversityData {
    std::vector<size_t> itemFrequency;  // How many individuals include each item.
    static constexpr size_t LOW_FREQ = 2;   // Threshold for low frequency items.
    static constexpr size_t HIGH_FREQ = 8;  // Threshold for high frequency items.

    // Updates item frequency tracking using sampling from top individuals.
    // O(SAMPLE_SIZE * NUM_ITEMS) instead of O(POPULATION_SIZE * NUM_ITEMS).
    template <typename Population>
    void updateFrequency(const Population &population, const SelectionData &selection, size_t numItems) {
        // Only sample top individuals - they carry the useful frequency signal.
        // Sampling the entire population is expensive and noisy.
        constexpr size_t MAX_SAMPLE = 10;
        size_t sampleSize = std::min(MAX_SAMPLE, population.size());

        itemFrequency.assign(numItems, 0);
        for (size_t s = 0; s < sampleSize; ++s) {
            const auto &ind = population[selection.populationRanks[s]];
            for (size_t i = 0; i < numItems; ++i) {
                if (ind.bits[i]) {
                    itemFrequency[i]++;
                }
            }
        }
        // Scale thresholds will be based on sampleSize, not POPULATION_SIZE.
    }
};


// ============================================================================
// Convergence/Stagnation Tracking Struct
// ============================================================================

// Struct for convergence detection variables.
struct ConvergenceData {
    std::deque<int64> fitnessHistory;       // Best fitness over last K generations.
    static constexpr size_t STAGNATION_WINDOW = 10; // Number of generations to check for stagnation.
    bool isStagnant = false;                // Flag indicating population stagnation.
    double currentMutationRate;             // Adaptive mutation rate.
    size_t generationCounter = 0;           // Current generation number.

    // Initializes convergence tracking.
    void init(double baseMutationRate) {
        fitnessHistory.clear();
        isStagnant = false;
        currentMutationRate = baseMutationRate;
        generationCounter = 1;
    }

    // Checks for stagnation and adjusts mutation rate accordingly.
    void update(int64 bestFitness, double baseMutationRate) {
        fitnessHistory.push_back(bestFitness);
        if (fitnessHistory.size() > STAGNATION_WINDOW) {
            fitnessHistory.pop_front();
        }

        // Check if fitness has improved in the last STAGNATION_WINDOW generations.
        if (fitnessHistory.size() >= STAGNATION_WINDOW) {
            int64 oldBest = fitnessHistory.front();
            int64 newBest = fitnessHistory.back();
            isStagnant = (newBest <= oldBest);
        }
        else {
            isStagnant = false;
        }

        // Adjust mutation rate: increase by 1.5× during stagnation.
        if (isStagnant) {
            currentMutationRate = std::min(baseMutationRate * 1.5, 0.1);
        }
        else {
            currentMutationRate = baseMutationRate;
        }
    }

    // Increments the generation counter.
    void nextGeneration() {
        generationCounter++;
    }
};


// ============================================================================
// Global State
// ============================================================================

// Global knapsack instance.
KnapsackInstance INSTANCE;

// Global hyperparameters.
Hyperparameters PARAMS;

// Global auxiliary data structures.
SortedItemsData SORTED_DATA;
SelectionData SELECTION_DATA;
DiversityData DIVERSITY_DATA;
ConvergenceData CONVERGENCE_DATA;

// Global random number generator.
std::mt19937 rng;


// ============================================================================
// Individual Class
// ============================================================================

// Class to represent an individual in the population.
// Each individual is a potential solution, represented by a bit string.
class Individual {
public:
    std::vector<bool> bits;             // A bit string representing item selection.
    int64 totalValue = 0;               // Cached total value of selected items.
    int64 totalWeight = 0;              // Cached total weight of selected items.
    mutable int64 cachedFitness = -1;   // Cached fitness value to avoid re-computation.
    mutable bool fitnessValid = false;  // Flag to check if the cached fitness is valid.

    // Constructor to create an individual with a given number of items, initialised to false.
    Individual(size_t n) : bits(n, false) {}

    // Calculates total value and weight from scratch. To be called after major changes.
    void calculateMetrics() {
        totalValue = 0;
        totalWeight = 0;
        for (size_t i = 0; i < INSTANCE.n; ++i) {
            if (bits[i]) {
                totalValue += INSTANCE.values[i];
                totalWeight += INSTANCE.weights[i];
            }
        }
        // Invalidate fitness cache as metrics have been recalculated.
        fitnessValid = false;
    }

    // Calculates the fitness of the individual.
    // Fitness is the total value of selected items or 0 if over capacity.
    int64 getFitness() const {
        // Return cached fitness if it's valid.
        if (fitnessValid) {
            return cachedFitness;
        }
        // Use pre-computed metrics to determine fitness.
        if (totalWeight > INSTANCE.capacity) {
            cachedFitness = 0;
        }
        else {
            cachedFitness = totalValue;
        }
        // Cache the fitness value.
        fitnessValid = true;
        return cachedFitness;
    }

    // Repairs an individual that is overweight (total weight > knapsack capacity).
    // Uses a two-phase approach: first removes low-ratio items, then greedily adds high-ratio items.
    void repair() {
        // Phase 1: Remove the worst items till the weight is within capacity.
        // Iterate through items sorted by value-to-weight ratio (worst to best).
        for (size_t i = 0; i < INSTANCE.n && totalWeight > INSTANCE.capacity; ++i) {
            // Remove the item if it is included in the individual.
            size_t itemId = SORTED_DATA.items[i].id;
            if (bits[itemId]) {
                bits[itemId] = false;                   // Remove item by setting its bit to false.
                totalWeight -= INSTANCE.weights[itemId];// Update the total weight.
                totalValue -= INSTANCE.values[itemId];  // Update the total value.
                fitnessValid = false;                   // Invalidate fitness cache.
            }
        }
        // Phase 2: Greedily add items with the best value-to-weight ratio that fit every 5 generations.
        if (CONVERGENCE_DATA.generationCounter % 5 == 0) {
            // Iterate through items sorted by value-to-weight ratio (best to worst).
            int64 remainingCapacity = INSTANCE.capacity - totalWeight;
            for (size_t i = INSTANCE.n; i > 0 && remainingCapacity >= INSTANCE.minWeight; --i) {
                // Try to add the item if it's not already included and fits within capacity.
                size_t itemId = SORTED_DATA.items[i - 1].id;
                if (!bits[itemId] && INSTANCE.weights[itemId] <= remainingCapacity) {
                    bits[itemId] = true;                            // Add item by setting its bit to true.
                    totalWeight += INSTANCE.weights[itemId];        // Update the total weight.
                    totalValue += INSTANCE.values[itemId];          // Update the total value.
                    remainingCapacity -= INSTANCE.weights[itemId];  // Update remaining capacity.
                    fitnessValid = false;                           // Invalidate fitness cache.
                }
            }
        }
    }

    // Applies diversity-preserving mutation to the individual.
    // Biases mutation toward underrepresented items in the population.
    void mutate() {
        // Use adaptive mutation rate (increased during stagnation).
        std::geometric_distribution<size_t> skipDist(CONVERGENCE_DATA.currentMutationRate);
        // Track if any mutation occurred.
        bool mutated = false;
        size_t pos = skipDist(rng);
        // Iterate through the bit string, mutating bits based on frequency.
        while (pos < INSTANCE.n) {
            // Diversity-aware mutation: skip mutations that reduce diversity.
            bool shouldMutate = true;
            size_t freq = DIVERSITY_DATA.itemFrequency[pos];
            // Protect rare items we have; skip adding already-common items.
            if ((bits[pos] && freq < DiversityData::LOW_FREQ) ||
                (!bits[pos] && freq > DiversityData::HIGH_FREQ)) {
                shouldMutate = false;
            }
            // Perform mutation if allowed.
            if (shouldMutate) {
                if (bits[pos]) {
                    totalValue -= INSTANCE.values[pos];
                    totalWeight -= INSTANCE.weights[pos];
                    bits[pos] = false;
                }
                else {
                    totalValue += INSTANCE.values[pos];
                    totalWeight += INSTANCE.weights[pos];
                    bits[pos] = true;
                }
                mutated = true;
            }
            // Skip to next mutation position.
            pos += 1 + skipDist(rng);
        }
        // Invalidate fitness cache if mutated.
        if (mutated) {
            fitnessValid = false;
        }
    }

    // Applies local search to improve the individual.
    // Tries item swaps and replacements to find better solutions.
    void localSearch() {
        // TODO: remove return; to activate local search AFTER OPTIMISING - currently O(n^2)
        return;
        // Local search flag.
        bool improved = true;
        // Repeat while improvements are found.
        while (improved) {
            improved = false;
            // Try to swap items: remove one included item and add one excluded item.
            for (size_t i = 0; i < INSTANCE.n && !improved; ++i) {
                // Skip items not in the knapsack.
                if (!bits[i]) {
                    continue;
                }
                // Try swapping with each item not in the knapsack.
                for (size_t j = 0; j < INSTANCE.n && !improved; ++j) {
                    // Skip items already in the knapsack.
                    if (bits[j]) {
                        continue;
                    }
                    // Calculate the change in weight and value if we swap items i and j.
                    int64 weightChange = INSTANCE.weights[j] - INSTANCE.weights[i];
                    int64 valueChange = INSTANCE.values[j] - INSTANCE.values[i];
                    // Check if the swap is feasible and improves the solution.
                    if (totalWeight + weightChange <= INSTANCE.capacity && valueChange > 0) {
                        // Perform the swap.
                        bits[i] = false;
                        bits[j] = true;
                        totalWeight += weightChange;
                        totalValue += valueChange;
                        fitnessValid = false;
                        improved = true;
                    }
                }
            }
        }
    }
};


// ============================================================================
// Population Generation Functions
// ============================================================================

// Generates a greedy individual based on value-to-weight ratio.
Individual generateGreedyIndividual() {
    // Create an empty individual.
    Individual ind(INSTANCE.n);

    // Add items in order of best value-to-weight ratio until capacity is reached.
    for (size_t i = INSTANCE.n; i > 0; --i) {
        size_t itemId = SORTED_DATA.items[i - 1].id;
        if (ind.totalWeight + INSTANCE.weights[itemId] <= INSTANCE.capacity) {
            ind.bits[itemId] = true;                    // Add item by setting its bit to true.
            ind.totalWeight += INSTANCE.weights[itemId];// Update the total weight.
            ind.totalValue += INSTANCE.values[itemId];  // Update the total value.
        }
    }

    return ind;
}

// Generates a random individual.
Individual generateRandomIndividual() {
    // Create an empty individual.
    Individual ind(INSTANCE.n);

    // Set bits randomly using batched random bits from rng().
    size_t i = 0;
    while (i < INSTANCE.n) {
        uint32_t randBits = rng();
        for (int b = 0; b < 32 && i < INSTANCE.n; ++b, ++i) {
            if (randBits & (1u << b)) {
                ind.bits[i] = true;
                ind.totalWeight += INSTANCE.weights[i];
                ind.totalValue += INSTANCE.values[i];
            }
        }
    }

    return ind;
}

// Generates the initial population with a mix of greedy and random individuals.
std::vector<Individual> generateInitialPopulation() {
    // Init population vector.
    std::vector<Individual> population;
    population.reserve(PARAMS.populationSize);

    // Determine how many greedy individuals to include (about 5% of population).
    size_t numGreedy = std::max(size_t(1), PARAMS.populationSize / 20);
    // Create greedy individuals.
    if (numGreedy) {
        Individual greedyInd = generateGreedyIndividual();
        for (size_t p = 0; p < numGreedy; ++p) {
            population.push_back(greedyInd);
        }
    }

    // Create random individuals.
    for (size_t p = numGreedy; p < PARAMS.populationSize; ++p) {
        Individual randInd = generateRandomIndividual();
        population.push_back(randInd);
    }

    // Repair any individuals in the initial population that are overweight.
    for (auto &individual : population) {
        individual.repair();
    }

    return population;
}


// ============================================================================
// Selection Functions
// ============================================================================

// Selects two parent individuals from the population using tournament selection.
std::pair<const Individual *, const Individual *> selection(
    const std::vector<Individual> &population) {
    // Distribution for generating random indices.
    std::uniform_int_distribution<size_t> indexDist(0, PARAMS.populationSize - 1);

    // Select four random individuals from the population.
    size_t idx1 = indexDist(rng);
    size_t idx2 = indexDist(rng);
    size_t idx3 = indexDist(rng);
    size_t idx4 = indexDist(rng);

    // Ensure distinct indices (individuals) for tournament selection.
    while (idx2 == idx1) {
        idx2 = (idx2 + 1) % PARAMS.populationSize;
    }
    while (idx3 == idx1 || idx3 == idx2) {
        idx3 = (idx3 + 1) % PARAMS.populationSize;
    }
    while (idx4 == idx1 || idx4 == idx2 || idx4 == idx3) {
        idx4 = (idx4 + 1) % PARAMS.populationSize;
    }

    // Tournament 1: Select the fitter individual between the first two.
    const Individual *parent1 = &population[idx1];
    const Individual *parent2 = &population[idx2];
    if (parent2->getFitness() > parent1->getFitness()) {
        parent1 = parent2;
    }

    // Tournament 2: Select the fitter individual between the last two.
    const Individual *parent3 = &population[idx3];
    const Individual *parent4 = &population[idx4];
    if (parent4->getFitness() > parent3->getFitness()) {
        parent3 = parent4;
    }

    return { parent1, parent3 };
}


// ============================================================================
// Crossover Function
// ============================================================================

// Performs crossover creating two children with incremental metric computation.
// On agreement: both children inherit the common bit.
// On disagreement: child1 gets fitter parent's bit, child2 gets other's, then 50% swap.
// This creates actual genetic mixing rather than parent cloning.
void crossover(const Individual &parent1, const Individual &parent2,
    Individual &child1, Individual &child2) {
    const std::vector<bool> &p1 = parent1.bits;
    const std::vector<bool> &p2 = parent2.bits;
    std::vector<bool> &c1 = child1.bits;
    std::vector<bool> &c2 = child2.bits;

    // Reset children metrics for incremental computation.
    int64 v1 = 0, w1 = 0, v2 = 0, w2 = 0;

    bool p1Better = (parent1.getFitness() >= parent2.getFitness());

    // Process items in batches using random bits.
    size_t i = 0;
    while (i < INSTANCE.n) {
        uint32_t randBits = rng();
        for (int b = 0; b < 32 && i < INSTANCE.n; ++b, ++i) {
            bool bit1, bit2;
            if (p1[i] == p2[i]) {
                // Parents agree: both children inherit.
                bit1 = p1[i];
                bit2 = p1[i];
            }
            else {
                // Parents disagree: better parent's bit to child1, other to child2.
                if (p1Better) {
                    bit1 = p1[i];
                    bit2 = p2[i];
                }
                else {
                    bit1 = p2[i];
                    bit2 = p1[i];
                }
                // Swap 50% of the time to create actual mixing.
                if (randBits & (1u << b)) {
                    std::swap(bit1, bit2);
                }
            }
            c1[i] = bit1;
            c2[i] = bit2;

            // Incremental metric update.
            if (bit1) {
                v1 += INSTANCE.values[i];
                w1 += INSTANCE.weights[i];
            }
            if (bit2) {
                v2 += INSTANCE.values[i];
                w2 += INSTANCE.weights[i];
            }
        }
    }

    child1.totalValue = v1;
    child1.totalWeight = w1;
    child1.fitnessValid = false;
    child2.totalValue = v2;
    child2.totalWeight = w2;
    child2.fitnessValid = false;
}


// ============================================================================
// Stagnation Recovery
// ============================================================================

// Reinitialises worst individuals during stagnation using aggressive mutation.
// Note: Must recompute ranks since population was swapped after nextGeneration.
void reinitialiseWorstIndividuals(std::vector<Individual> &population) {
    if (!CONVERGENCE_DATA.isStagnant) return;

    // Recompute ranks for the swapped population.
    SELECTION_DATA.precomputeRanks(population);

    // Reinitialise worst 10% of population.
    size_t numToReinit = std::max(size_t(1), PARAMS.populationSize / 10);

    // Find median-fitness individual as template.
    size_t medianIdx = SELECTION_DATA.populationRanks[PARAMS.populationSize / 2];
    const Individual &medianInd = population[medianIdx];

    // Reinitialise worst individuals (at end of sorted ranks).
    for (size_t i = 0; i < numToReinit; ++i) {
        size_t worstIdx = SELECTION_DATA.populationRanks[PARAMS.populationSize - 1 - i];
        population[worstIdx] = medianInd;
        // Apply aggressive double mutation.
        population[worstIdx].mutate();
        population[worstIdx].mutate();
        population[worstIdx].repair();
    }
}


// ============================================================================
// Generation Evolution
// ============================================================================

// Generates the next generation of the population and swaps it with the current population.
// Uses elitism, ranking-based selection, heuristic crossover, and diversity-aware mutation.
void nextGeneration(const std::vector<Individual> &currentPop, std::vector<Individual> &nextPop) {
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    // Pre-compute selection ranks for this generation (O(n log n)).
    SELECTION_DATA.precomputeRanks(currentPop);

    // Update item frequency for diversity-aware mutation.
    DIVERSITY_DATA.updateFrequency(currentPop, SELECTION_DATA, INSTANCE.n);

    // Elitism: The best individual is carried over.
    int64 bestFitness = currentPop[SELECTION_DATA.populationRanks[0]].getFitness();
    nextPop[0] = currentPop[SELECTION_DATA.populationRanks[0]];

    // Update convergence state and adaptive mutation rate.
    CONVERGENCE_DATA.update(bestFitness, PARAMS.mutationRate);

    // Fill the rest of the next population.
    for (size_t i = 1; i < PARAMS.populationSize; ) {
        // Select two parents using tournament selection.
        auto [parent1, parent2] = selection(currentPop);

        // Decide whether to reproduce directly or create offspring.
        if (probDist(rng) < PARAMS.reproductionRate) {
            // Reproduction: Directly copy parents to the next population.
            nextPop[i] = *parent1;
            nextPop[i].repair();
            i++;
            if (i < PARAMS.populationSize) {
                nextPop[i] = *parent2;
                nextPop[i].repair();
                i++;
            }
        }
        else {
            // Select two children slots in the next population.
            Individual &child1 = nextPop[i];
            Individual &child2 = (i + 1 < PARAMS.populationSize) ? nextPop[i + 1] : child1;

            // Decide whether to perform crossover.
            if (probDist(rng) < PARAMS.crossoverRate) {
                // Heuristic crossover: Create two children from the parents.
                crossover(*parent1, *parent2, child1, child2);
            }
            else {
                // No crossover: Children are copies of the parents.
                child1 = *parent1;
                if (i + 1 < PARAMS.populationSize) {
                    child2 = *parent2;
                }
            }

            // Mutate, repair, and apply local search to child1.
            child1.mutate();
            child1.repair();
            child1.localSearch();
            i++;

            // Mutate, repair, and apply local search to child2 if there's space.
            if (i < PARAMS.populationSize && &child1 != &child2) {
                child2.mutate();
                child2.repair();
                child2.localSearch();
                i++;
            }
        }
    }

    // Increment generation counter.
    CONVERGENCE_DATA.nextGeneration();
}


// ============================================================================
// Main Genetic Algorithm
// ============================================================================

// Main Genetic Algorithm function to solve the knapsack problem.
Result solveKnapsackGenetic() {
    // Init result struct.
    Result result;

    // Start the timer.
    auto start = std::chrono::high_resolution_clock::now();

    // Pre-sort items by value-to-weight ratio.
    SORTED_DATA.preSortItems(INSTANCE);

    // Initialize convergence tracking.
    CONVERGENCE_DATA.init(PARAMS.mutationRate);

    // Create the initial population.
    std::vector<Individual> population = generateInitialPopulation();
    std::vector<Individual> nextPopulation(PARAMS.populationSize, Individual(INSTANCE.n));

    // Evolve the population over generations.
    for (size_t gen = 0; gen < PARAMS.maxGenerations; ++gen) {
        nextGeneration(population, nextPopulation);
        population.swap(nextPopulation);

        // Apply stagnation-based reinitialisation after swap.
        // Ranks are already computed in nextGeneration, reuse them.
        if (CONVERGENCE_DATA.isStagnant) {
            reinitialiseWorstIndividuals(population);
        }
    }

    // Find the best individual in the final population.
    Individual bestIndividual = *max_element(
        population.begin(),
        population.end(),
        [](const Individual &a, const Individual &b) { return a.getFitness() < b.getFitness(); });

    // Stop the timer.
    auto end = std::chrono::high_resolution_clock::now();

    // Store the results.
    result.maxValue = bestIndividual.getFitness();

    // Collect the indices of the selected items.
    for (size_t i = 0; i < INSTANCE.n; ++i) {
        if (bestIndividual.bits[i]) {
            result.selectedItems.push_back(i);
        }
    }

    // Calculate execution time in microseconds.
    result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Approximate memory usage (vector<bool> stores 1 bit per element).
    size_t populationMemory = 0;
    for (const auto &ind : population) {
        populationMemory += sizeof(Individual) + (ind.bits.capacity() + 7) / 8;
    }
    for (const auto &ind : nextPopulation) {
        populationMemory += sizeof(Individual) + (ind.bits.capacity() + 7) / 8;
    }
    size_t vectorMemory =
        (sizeof(int64) * (INSTANCE.weights.size() + INSTANCE.values.size())) +
        (sizeof(size_t) * result.selectedItems.capacity()) +
        (sizeof(ItemProperty) * SORTED_DATA.items.capacity());
    result.memoryUsed = populationMemory + vectorMemory;

    return result;
}


// ============================================================================
// Command-line Argument Parsing
// ============================================================================

// Parses command-line arguments to override default hyperparameters.
// Returns the input filepath if provided, empty string otherwise.
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
        else if (arg == "--help" || arg == "-h") {
            std::cerr << "Usage: " << argv[0] << " <input_file> [options]" << "\n";
            std::cerr << "Options:" << "\n";
            std::cerr << "  --population_size <int>      Population size" << "\n";
            std::cerr << "  --max_generations <int>      Max generations" << "\n";
            std::cerr << "  --crossover_rate <float>     Crossover rate" << "\n";
            std::cerr << "  --mutation_rate <float>      Mutation rate" << "\n";
            std::cerr << "  --reproduction_rate <float>  Reproduction rate" << "\n";
            std::cerr << "  --seed <unsigned int>        Seed for random number generator" << "\n";
            exit(0);
        }
        else if (arg[0] != '-' && filepath.empty()) {
            filepath = arg;
        }
    }
    return filepath;
}


// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char *argv[]) {
    // Use fast I/O.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Parse command-line arguments for hyperparameters.
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

    // If population size is not provided, compute it using a heuristic.
    if (PARAMS.populationSize == 0) {
        if (INSTANCE.n >= 100000) {
            PARAMS.populationSize = 60;
        }
        else {
            PARAMS.populationSize = 100;
        }
    }

    // If max generations is not provided, compute it using a heuristic.
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
