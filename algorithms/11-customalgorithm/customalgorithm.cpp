#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <numeric>
#include <cstring>
#include "../file_io.hpp"

// Use int64 for large numbers
using int64 = long long;

// Struct to hold the results of the genetic algorithm.
struct Result {
    int64 maxValue;                     // The maximum value found for the knapsack.
    std::vector<size_t> selectedItems;  // The indices of the items selected to achieve the max value.
    int64 executionTime;                // Total execution time in microseconds.
    size_t memoryUsed;                  // Approximate memory usage in bytes.
};

// Struct to hold item properties for sorting.
struct ItemProperty {
    size_t id;
    double ratio;
};


// Genetic Algorithm hyperparameters. These can be overridden by command-line arguments.
size_t POPULATION_SIZE = 0;                 // Size of the population. If -1, set heuristically.
size_t MAX_GENERATIONS = 0;                 // Number of generations. If -1, set heuristically.
double CROSSOVER_RATE = 0.53;               // Probability of crossover.
double MUTATION_RATE = 0.013;               // Probability of mutation.
double REPRODUCTION_RATE = 0.15;            // Probability of direct reproduction.
unsigned int SEED = std::random_device()(); // Seed for random number generator.

// Global variables for the knapsack problem instance.
int64 KNAPSACK_CAPACITY;                // Maximum weight the knapsack can hold.
const std::vector<int64> *g_ITEM_WEIGHTS;   // Pointer to item weights (set in main, no copy).
const std::vector<int64> *g_ITEM_VALUES;    // Pointer to item values (set in main, no copy).
size_t NUM_ITEMS;                       // Total number of items.

// Global helper variables.
std::vector<ItemProperty> SORTED_ITEMS; // Pre-sorted list of items by value-to-weight ratio.
int64 MIN_ITEM_WEIGHT;                  // Minimum item weight in the instance.

// Global random number generator.
std::mt19937 rng;   // Mersenne Twister random number generator, seeded in main.

// Accessor helpers for cleaner code
inline const std::vector<int64> &ITEM_WEIGHTS() { return *g_ITEM_WEIGHTS; }
inline const std::vector<int64> &ITEM_VALUES() { return *g_ITEM_VALUES; }


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
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            if (bits[i]) {
                totalValue += ITEM_VALUES()[i];
                totalWeight += ITEM_WEIGHTS()[i];
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
        if (totalWeight > KNAPSACK_CAPACITY) {
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
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            // Early exit: weight is within capacity
            if (totalWeight <= KNAPSACK_CAPACITY) {
                break;
            }
            // Remove the item if it is included in the individual.
            size_t itemId = SORTED_ITEMS[i].id;
            if (bits[itemId]) {
                bits[itemId] = false;                  // Remove item by setting its bit to false.
                totalWeight -= ITEM_WEIGHTS()[itemId];   // Update the total weight.
                totalValue -= ITEM_VALUES()[itemId];     // Update the total value.
                fitnessValid = false;                  // Invalidate fitness cache.
            }
        }
        // Phase 2: Greedily add items with the best value-to-weight ratio that fit.
        // Iterate through items sorted by value-to-weight ratio (best to worst).
        for (size_t i = NUM_ITEMS; i > 0; --i) {
            // Early exit: remaining capacity is 0
            if (totalWeight == KNAPSACK_CAPACITY) {
                break;
            }
            // Early exit: remaining capacity is less than minimum item weight
            if (KNAPSACK_CAPACITY - totalWeight < MIN_ITEM_WEIGHT) {
                break;
            }
            // Try to add the item if it's not already included and fits within capacity.
            size_t itemId = SORTED_ITEMS[i - 1].id;
            if (!bits[itemId] && totalWeight + ITEM_WEIGHTS()[itemId] <= KNAPSACK_CAPACITY) {
                bits[itemId] = true;                    // Add item by setting its bit to true.
                totalWeight += ITEM_WEIGHTS()[itemId];    // Update the total weight.
                totalValue += ITEM_VALUES()[itemId];      // Update the total value.
                fitnessValid = false;                   // Invalidate fitness cache.
            }
        }
    }

    // Applies mutation to the individual.
    // Each bit in the individual's bit string has a chance to be flipped.
    void mutate() {
        // Distribution for mutation probability.
        std::uniform_real_distribution<double> probDist(0.0, 1.0);
        // Mutation flag.
        bool mutated = false;
        // Apply mutation to each bit.
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            // Flip the bit with probability MUTATION_RATE and update metrics accordingly.
            if (probDist(rng) < MUTATION_RATE) {
                // Flipping true-> false, removing item.
                if (bits[i]) {
                    totalValue -= ITEM_VALUES()[i];
                    totalWeight -= ITEM_WEIGHTS()[i];
                }
                // Flipping false-> true, adding item.
                else {
                    totalValue += ITEM_VALUES()[i];
                    totalWeight += ITEM_WEIGHTS()[i];
                }
                // Flip the bit and set mutated flag.
                bits[i].flip();
                mutated = true;
            }
        }
        // Invalidate fitness cache if mutation occurred.
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
            for (size_t i = 0; i < NUM_ITEMS && !improved; ++i) {
                // Skip items not in the knapsack.
                if (!bits[i]) {
                    continue;
                }
                // Try swapping with each item not in the knapsack.
                for (size_t j = 0; j < NUM_ITEMS && !improved; ++j) {
                    // Skip items already in the knapsack.
                    if (bits[j]) {
                        continue;
                    }
                    // Calculate the change in weight and value if we swap items i and j.
                    int64 weightChange = ITEM_WEIGHTS()[j] - ITEM_WEIGHTS()[i];
                    int64 valueChange = ITEM_VALUES()[j] - ITEM_VALUES()[i];
                    // Check if the swap is feasible and improves the solution.
                    if (totalWeight + weightChange <= KNAPSACK_CAPACITY && valueChange > 0) {
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


// Pre-sorts items by their value-to-weight ratio in ascending order.
void preSortItems() {
    // Resize SORTED_ITEMS.
    SORTED_ITEMS.resize(NUM_ITEMS);

    // Compute value-to-weight ratio for each item. Handle zero weight case.
    for (size_t i = 0; i < NUM_ITEMS; ++i) {
        SORTED_ITEMS[i].id = i;
        if (ITEM_WEIGHTS()[i] > 0) {
            SORTED_ITEMS[i].ratio = static_cast<double>(ITEM_VALUES()[i]) / ITEM_WEIGHTS()[i];
        }
        else if (ITEM_VALUES()[i] > 0) {
            SORTED_ITEMS[i].ratio = std::numeric_limits<double>::infinity();
        }
        else {
            SORTED_ITEMS[i].ratio = 0.0;
        }
    }

    // Sort items by their value-to-weight ratio in ascending order.
    std::sort(
        SORTED_ITEMS.begin(),
        SORTED_ITEMS.end(),
        [](const ItemProperty &a, const ItemProperty &b) {
            return a.ratio < b.ratio;
        }
    );
}


// Generates a greedy individual based on value-to-weight ratio.
Individual generateGreedyIndividual() {
    // Create an empty individual.
    Individual ind(NUM_ITEMS);

    // Add items in order of best value-to-weight ratio until capacity is reached.
    for (auto it = SORTED_ITEMS.rbegin(); it != SORTED_ITEMS.rend(); ++it) {
        size_t itemId = it->id;
        if (ind.totalWeight + ITEM_WEIGHTS()[itemId] <= KNAPSACK_CAPACITY) {
            ind.bits[itemId] = true;                // Add item by setting its bit to true.
            ind.totalWeight += ITEM_WEIGHTS()[itemId];// Update the total weight.
            ind.totalValue += ITEM_VALUES()[itemId];  // Update the total value.
        }
    }

    return ind;
}


// Generates a random individual.
Individual generateRandomIndividual() {
    // Create an empty individual.
    Individual ind(NUM_ITEMS);
    // Distribution for generating random bits.
    std::uniform_int_distribution<int> bitDist(0, 1);

    // Randomly set bits in the individual's bit string.
    for (size_t i = 0; i < NUM_ITEMS; ++i) {
        if (bitDist(rng)) {
            ind.bits[i] = true;                 // Add item by setting its bit to true.
            ind.totalWeight += ITEM_WEIGHTS()[i]; // Update the total weight.
            ind.totalValue += ITEM_VALUES()[i];   // Update the total value.
        }
    }

    return ind;
}


// Generates the initial population with a mix of greedy and random individuals.
std::vector<Individual> generateInitialPopulation() {
    // Init population vector.
    std::vector<Individual> population;
    population.reserve(POPULATION_SIZE);

    // Determine how many greedy individuals to include (about 5% of population).
    size_t numGreedy = std::max(size_t(1), POPULATION_SIZE / 20);
    // Create greedy individuals.
    if (numGreedy) {
        Individual greedyInd = generateGreedyIndividual();
        for (size_t p = 0; p < numGreedy; ++p) {
            population.push_back(greedyInd);
        }
    }

    // Create random individuals.
    for (size_t p = numGreedy; p < POPULATION_SIZE; ++p) {
        Individual randInd = generateRandomIndividual();
        population.push_back(randInd);
    }

    // Repair any individuals in the initial population that are overweight.
    for (auto &individual : population) {
        individual.repair();
    }

    return population;
}


// Selects two parent individuals from the population using tournament selection.
std::pair<const Individual *, const Individual *> selection(const std::vector<Individual> &population) {
    // Distribution for generating random indices.
    std::uniform_int_distribution<size_t> indexDist(0, POPULATION_SIZE - 1);

    // Select four random individuals from the population.
    size_t idx1 = indexDist(rng);
    size_t idx2 = indexDist(rng);
    size_t idx3 = indexDist(rng);
    size_t idx4 = indexDist(rng);

    // Ensure distinct indices (individuals) for tournament selection.
    while (idx2 == idx1) {
        idx2 = (idx2 + 1) % POPULATION_SIZE;
    }
    while (idx3 == idx1 || idx3 == idx2) {
        idx3 = (idx3 + 1) % POPULATION_SIZE;
    }
    while (idx4 == idx1 || idx4 == idx2 || idx4 == idx3) {
        idx4 = (idx4 + 1) % POPULATION_SIZE;
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


// Performs uniform crossover on two parent individuals to create two children.
// Each bit is independently inherited from either parent with equal probability.
void crossover(const Individual &parent1, const Individual &parent2,
    Individual &child1, Individual &child2) {
    // Distribution for selecting parent for each bit.
    std::uniform_int_distribution<int> parentDist(0, 1);

    // For each bit position, randomly choose which parent to inherit from.
    for (size_t i = 0; i < NUM_ITEMS; ++i) {
        if (parentDist(rng) == 0) {
            // Child 1 inherits from parent1, child 2 from parent2.
            child1.bits[i] = parent1.bits[i];
            child2.bits[i] = parent2.bits[i];
        }
        else {
            // Child 1 inherits from parent2, child 2 from parent1.
            child1.bits[i] = parent2.bits[i];
            child2.bits[i] = parent1.bits[i];
        }
    }

    // Recalculate metrics from scratch for the children.
    child1.calculateMetrics();
    child2.calculateMetrics();
}


// Generates the next generation of the population and swaps it with the current population.
// Uses elitism, selection followed by reproduction or crossover, and mutation.
void nextGeneration(const std::vector<Individual> &currentPop, std::vector<Individual> &nextPop) {
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    // Elitism: The best individual from the current population is carried over to the next generation.
    auto bestIt = max_element(
        currentPop.begin(),
        currentPop.end(),
        [](const Individual &a, const Individual &b) { return a.getFitness() < b.getFitness(); });
    nextPop[0] = *bestIt;

    // Fill the rest of the next population.
    for (size_t i = 1; i < POPULATION_SIZE; ) {
        // Select two parents using tournament selection.
        auto [parent1, parent2] = selection(currentPop);

        // Decide whether to reproduce directly or create offspring.
        if (probDist(rng) < REPRODUCTION_RATE) {
            // Reproduction: Directly copy parents to the next population.
            nextPop[i++] = *parent1;
            if (i < POPULATION_SIZE) {
                nextPop[i++] = *parent2;
            }
        }
        else {
            // Select two children slots in the next population.
            Individual &child1 = nextPop[i];
            Individual &child2 = (i + 1 < POPULATION_SIZE) ? nextPop[i + 1] : child1;

            // Decide whether to perform crossover.
            if (probDist(rng) < CROSSOVER_RATE) {
                // Crossover: Create two children from the parents.
                crossover(*parent1, *parent2, child1, child2);
            }
            else {
                // No crossover: Children are copies of the parents.
                child1 = *parent1;
                if (i + 1 < POPULATION_SIZE) {
                    child2 = *parent2;
                }
            }

            // Mutate, repair, and apply local search to child1.
            child1.mutate();
            child1.repair();
            child1.localSearch();
            i++;

            // Mutate, repair, and apply local search to child2 if there's space in the population.
            if (i < POPULATION_SIZE && &child1 != &child2) {
                child2.mutate();
                child2.repair();
                child2.localSearch();
                i++;
            }
        }
    }
}


// Main Genetic Algorithm function to solve the knapsack problem.
Result solveKnapsackGenetic() {
    // Init result struct.
    Result result;

    // Start the timer.
    auto start = std::chrono::high_resolution_clock::now();

    // Pre-sort items by value-to-weight ratio.
    preSortItems();

    // MIN_ITEM_WEIGHT is already set in main from instance.minWeight

    // Create the initial population.
    std::vector<Individual> population = generateInitialPopulation();
    std::vector<Individual> nextPopulation(POPULATION_SIZE, Individual(NUM_ITEMS));

    // Evolve the population over generations.
    for (size_t gen = 0; gen < MAX_GENERATIONS; ++gen) {
        nextGeneration(population, nextPopulation);
        population.swap(nextPopulation);
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
    for (size_t i = 0; i < NUM_ITEMS; ++i) {
        if (bestIndividual.bits[i]) {
            result.selectedItems.push_back(i);
        }
    }

    // Calculate execution time in microseconds.
    result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Approximate memory usage.
    size_t populationMemory = 0;
    for (const auto &ind : population) {
        populationMemory += sizeof(Individual) + ((ind.bits.capacity() + 7) / 8);
    }
    for (const auto &ind : nextPopulation) {
        populationMemory += sizeof(Individual) + ((ind.bits.capacity() + 7) / 8);
    }
    size_t vectorMemory =
        (sizeof(int64) * (ITEM_WEIGHTS().size() + ITEM_VALUES().size())) +
        (sizeof(size_t) * result.selectedItems.capacity()) +
        (sizeof(ItemProperty) * SORTED_ITEMS.capacity());
    result.memoryUsed = populationMemory + vectorMemory;

    return result;
}

// Parses command-line arguments to override default hyperparameters.
// Returns the input filepath if provided, empty string otherwise.
std::string parseArguments(int argc, char *argv[]) {
    std::string filepath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--population_size" && i + 1 < argc) {
            POPULATION_SIZE = std::stoul(argv[++i]);
        }
        else if (arg == "--max_generations" && i + 1 < argc) {
            MAX_GENERATIONS = std::stoul(argv[++i]);
        }
        else if (arg == "--crossover_rate" && i + 1 < argc) {
            CROSSOVER_RATE = std::stod(argv[++i]);
        }
        else if (arg == "--mutation_rate" && i + 1 < argc) {
            MUTATION_RATE = std::stod(argv[++i]);
        }
        else if (arg == "--reproduction_rate" && i + 1 < argc) {
            REPRODUCTION_RATE = std::stod(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc) {
            SEED = static_cast<unsigned int>(std::stoi(argv[++i]));
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
    rng.seed(SEED);

    // Load problem instance from file.
    KnapsackInstance instance;
    if (!loadKnapsackInstance(filepath, instance)) {
        return 1;
    }

    // Set instance variables.
    NUM_ITEMS = instance.n;
    KNAPSACK_CAPACITY = instance.capacity;
    MIN_ITEM_WEIGHT = instance.minWeight;  // Use pre-computed min weight from instance

    // If population size is not provided, compute it using a heuristic.
    if (POPULATION_SIZE == 0) {
        if (NUM_ITEMS < 100) {
            POPULATION_SIZE = 20;
        }
        else if (NUM_ITEMS < 1000) {
            POPULATION_SIZE = 50;
        }
        else if (NUM_ITEMS < 10000) {
            POPULATION_SIZE = 100;
        }
        else {
            POPULATION_SIZE = 150;
        }
    }

    // If max generations is not provided, compute it using a heuristic.
    if (MAX_GENERATIONS == 0) {
        if (NUM_ITEMS < 100) {
            MAX_GENERATIONS = 200;
        }
        else if (NUM_ITEMS < 1000) {
            MAX_GENERATIONS = 100;
        }
        else if (NUM_ITEMS < 10000) {
            MAX_GENERATIONS = 50;
        }
        else {
            MAX_GENERATIONS = 30;
        }
    }

    // Set pointers to item data (no copy).
    g_ITEM_WEIGHTS = &instance.weights;
    g_ITEM_VALUES = &instance.values;

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
