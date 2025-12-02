#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <numeric>
#include <cstring>
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
// Global State
// ============================================================================

// Global knapsack instance.
KnapsackInstance INSTANCE;

// Global hyperparameters.
Hyperparameters PARAMS;

// Global random number generator.
std::mt19937 rng;

// Helper vector for repair function.
std::vector<size_t> included_indices;


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
    // It randomly removes items until the individual is valid.
    void repair() {
        // Calculate the current weight of the individual.
        if (totalWeight <= INSTANCE.capacity) {
            return;
        }
        // Collect indices of items that are currently included in the knapsack.
        size_t included_count = 0;
        for (size_t i = 0; i < INSTANCE.n; ++i) {
            if (bits[i]) {
                included_indices[included_count++] = i;
            }
        }
        // Randomly remove items until the individual is within capacity.
        while (totalWeight > INSTANCE.capacity && included_count > 0) {
            // Distribution for generating random indices.
            std::uniform_int_distribution<size_t> dist(0, included_count - 1);
            // Select a random item to remove.
            size_t random_vector_index = dist(rng);
            size_t item_index = included_indices[random_vector_index];

            // Remove the item from the individual.
            included_indices[random_vector_index] = included_indices[--included_count];

            // Update individual's bit string and metrics.
            bits[item_index] = 0;
            totalWeight -= INSTANCE.weights[item_index];
            totalValue -= INSTANCE.values[item_index];
        }
        // Invalidate the fitness cache as the individual has been modified.
        fitnessValid = false;
    }

    // Applies mutation to the individual.
    // Each bit in the individual's bit string has a chance to be flipped.
    void mutate() {
        // Use geometric distribution to skip bits that don't mutate.
        // This is much faster when mutation rate is low (e.g. 1/N).
        std::geometric_distribution<size_t> skipDist(PARAMS.mutationRate);

        bool mutated = false;
        size_t pos = skipDist(rng);

        while (pos < INSTANCE.n) {
            // Flip the bit at pos
            if (bits[pos]) {
                // Was true, becoming false (remove item)
                totalValue -= INSTANCE.values[pos];
                totalWeight -= INSTANCE.weights[pos];
                bits[pos] = false;
            }
            else {
                // Was false, becoming true (add item)
                totalValue += INSTANCE.values[pos];
                totalWeight += INSTANCE.weights[pos];
                bits[pos] = true;
            }
            mutated = true;

            // Move to next mutation
            pos += 1 + skipDist(rng);
        }

        // Invalidate fitness cache if mutation occurred.
        if (mutated) {
            fitnessValid = false;
        }
    }
};


// ============================================================================
// Population Generation Functions
// ============================================================================

// Generates the initial population of random individuals.
std::vector<Individual> generateInitialPopulation() {
    // Init population vector.
    std::vector<Individual> population;
    population.reserve(PARAMS.populationSize);

    // Create POPULATION_SIZE individuals with random bit strings.
    for (size_t p = 0; p < PARAMS.populationSize; ++p) {
        Individual ind(INSTANCE.n);

        // Batched random bit generation
        size_t i = 0;
        while (i < INSTANCE.n) {
            uint32_t randBits = rng(); // Generate 32 random bits
            for (int b = 0; b < 32 && i < INSTANCE.n; ++b, ++i) {
                if (randBits & (1u << b)) {
                    ind.bits[i] = true;
                    ind.totalValue += INSTANCE.values[i];
                    ind.totalWeight += INSTANCE.weights[i];
                }
            }
        }

        population.push_back(ind);
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

// Performs single-point crossover on two parent individuals to create two children.
// The bit strings of the parents are split at the midpoint and swapped.
void crossover(const Individual &parent1, const Individual &parent2,
    Individual &child1, Individual &child2) {
    // Crossover point is the midpoint of the bit string.
    size_t midpoint = INSTANCE.n / 2;

    // Reset metrics
    child1.totalValue = 0; child1.totalWeight = 0;
    child2.totalValue = 0; child2.totalWeight = 0;

    // First half: child1 gets parent1, child2 gets parent2
    for (size_t i = 0; i < midpoint; ++i) {
        bool b1 = parent1.bits[i];
        bool b2 = parent2.bits[i];

        child1.bits[i] = b1;
        if (b1) {
            child1.totalValue += INSTANCE.values[i];
            child1.totalWeight += INSTANCE.weights[i];
        }

        child2.bits[i] = b2;
        if (b2) {
            child2.totalValue += INSTANCE.values[i];
            child2.totalWeight += INSTANCE.weights[i];
        }
    }

    // Second half: child1 gets parent2, child2 gets parent1
    for (size_t i = midpoint; i < INSTANCE.n; ++i) {
        bool b2 = parent2.bits[i];
        bool b1 = parent1.bits[i];

        child1.bits[i] = b2;
        if (b2) {
            child1.totalValue += INSTANCE.values[i];
            child1.totalWeight += INSTANCE.weights[i];
        }

        child2.bits[i] = b1;
        if (b1) {
            child2.totalValue += INSTANCE.values[i];
            child2.totalWeight += INSTANCE.weights[i];
        }
    }

    child1.fitnessValid = false;
    child2.fitnessValid = false;
}


// ============================================================================
// Generation Evolution
// ============================================================================

// Generates the next generation of the population and swaps it with the current population.
// Uses selection followed by reproduction or crossover, and mutation.
void nextGeneration(const std::vector<Individual> &currentPop, std::vector<Individual> &nextPop) {
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    // Fill the next population.
    for (size_t i = 0; i < PARAMS.populationSize; ) {
        // Select two parents using tournament selection.
        auto [parent1, parent2] = selection(currentPop);

        // Decide whether to reproduce directly or create offspring.
        if (probDist(rng) < PARAMS.reproductionRate) {
            // Reproduction: Directly copy parents to the next population.
            nextPop[i] = *parent1;
            nextPop[i].repair();  // Ensure repaired
            i++;
            if (i < PARAMS.populationSize) {
                nextPop[i] = *parent2;
                nextPop[i].repair();  // Ensure repaired
                i++;
            }
        }
        else {
            // Select two children slots in the next population.
            Individual &child1 = nextPop[i];
            Individual &child2 = (i + 1 < PARAMS.populationSize) ? nextPop[i + 1] : child1;

            // Decide whether to perform crossover.
            if (probDist(rng) < PARAMS.crossoverRate) {
                // Crossover: Create two children from the parents.
                crossover(*parent1, *parent2, child1, child2);
            }
            else {
                // No crossover: Children are copies of the parents.
                child1 = *parent1;
                if (i + 1 < PARAMS.populationSize) {
                    child2 = *parent2;
                }
            }

            // Mutate and repair child1.
            child1.mutate();
            child1.repair();
            i++;

            // Mutate and repair child2 if there's space in the population.
            if (i < PARAMS.populationSize && &child1 != &child2) {
                child2.mutate();
                child2.repair();
                i++;
            }
        }
    }
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

    // Resize helper vector for repair function (included in timing).
    included_indices.resize(INSTANCE.n);

    // Create the initial population.
    std::vector<Individual> population = generateInitialPopulation();
    std::vector<Individual> nextPopulation(PARAMS.populationSize, Individual(INSTANCE.n));

    // Evolve the population over generations.
    for (size_t gen = 0; gen < PARAMS.maxGenerations; ++gen) {
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
        (sizeof(size_t) * result.selectedItems.capacity());
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
        if (INSTANCE.n < 100) {
            PARAMS.populationSize = 20;
        }
        else if (INSTANCE.n < 1000) {
            PARAMS.populationSize = 50;
        }
        else if (INSTANCE.n < 10000) {
            PARAMS.populationSize = 100;
        }
        else {
            PARAMS.populationSize = 150;
        }
    }

    // If max generations is not provided, compute it using a heuristic.
    if (PARAMS.maxGenerations == 0) {
        if (INSTANCE.n < 100) {
            PARAMS.maxGenerations = 200;
        }
        else if (INSTANCE.n < 1000) {
            PARAMS.maxGenerations = 100;
        }
        else if (INSTANCE.n < 10000) {
            PARAMS.maxGenerations = 50;
        }
        else {
            PARAMS.maxGenerations = 30;
        }
    }

    // Solve the knapsack problem (helper vector allocation happens inside timed function).
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
