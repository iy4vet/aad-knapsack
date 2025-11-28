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
size_t POPULATION_SIZE = 0;                 // Size of the population. If 0, set heuristically.
size_t MAX_GENERATIONS = 0;                 // Number of generations. If 0, set heuristically.
double CROSSOVER_RATE = 0.53;               // Probability of crossover.
double MUTATION_RATE = 0.013;               // Probability of mutation.
double REPRODUCTION_RATE = 0.15;            // Probability of direct reproduction.
unsigned int SEED = std::random_device()(); // Seed for random number generator.

// Global variables for the knapsack problem instance.
int64 KNAPSACK_CAPACITY;                    // Maximum weight the knapsack can hold.
const std::vector<int64> *g_ITEM_WEIGHTS;   // Pointer to item weights (set in main, no copy).
const std::vector<int64> *g_ITEM_VALUES;    // Pointer to item values (set in main, no copy).
size_t NUM_ITEMS;                           // Total number of items.

// Global random number generator and helper vector.
std::mt19937 rng;                       // Mersenne Twister random number generator, seeded in main.
std::vector<size_t> included_indices;   // Helper vector for repair function.

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
    // It randomly removes items until the individual is valid.
    void repair() {
        // Calculate the current weight of the individual.
        if (totalWeight <= KNAPSACK_CAPACITY) {
            return;
        }
        // Collect indices of items that are currently included in the knapsack.
        size_t included_count = 0;
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            if (bits[i]) {
                included_indices[included_count++] = i;
            }
        }
        // Randomly remove items until the individual is within capacity.
        while (totalWeight > KNAPSACK_CAPACITY && included_count > 0) {
            // Distribution for generating random indices.
            std::uniform_int_distribution<size_t> dist(0, included_count - 1);
            // Select a random item to remove.
            size_t random_vector_index = dist(rng);
            size_t item_index = included_indices[random_vector_index];

            // Remove the item from the individual.
            included_indices[random_vector_index] = included_indices[--included_count];

            // Update individual's bit string and metrics.
            bits[item_index] = 0;
            totalWeight -= ITEM_WEIGHTS()[item_index];
            totalValue -= ITEM_VALUES()[item_index];
        }
        // Invalidate the fitness cache as the individual has been modified.
        fitnessValid = false;
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
};


// Generates the initial population of random individuals.
std::vector<Individual> generateInitialPopulation() {
    // Init population vector.
    std::vector<Individual> population;
    population.reserve(POPULATION_SIZE);
    // Distribution for generating random bits.
    std::uniform_int_distribution<int> bitDist(0, 1);

    // Create POPULATION_SIZE individuals with random bit strings.
    for (size_t p = 0; p < POPULATION_SIZE; ++p) {
        Individual ind(NUM_ITEMS);
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            ind.bits[i] = bitDist(rng);
        }
        // Calculate metrics once from scratch.
        ind.calculateMetrics();
        population.push_back(ind);
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


// Performs single-point crossover on two parent individuals to create two children.
// The bit strings of the parents are split at the midpoint and swapped.
void crossover(const Individual &parent1, const Individual &parent2,
    Individual &child1, Individual &child2) {
    // Crossover point is the midpoint of the bit string.
    size_t midpoint = NUM_ITEMS / 2;

    // Child 1: First half from parent1, second half from parent2.
    std::copy(parent1.bits.begin(), parent1.bits.begin() + midpoint, child1.bits.begin());
    std::copy(parent2.bits.begin() + midpoint, parent2.bits.end(), child1.bits.begin() + midpoint);

    // Child 2: First half from parent2, second half from parent1.
    std::copy(parent2.bits.begin(), parent2.bits.begin() + midpoint, child2.bits.begin());
    std::copy(parent1.bits.begin() + midpoint, parent1.bits.end(), child2.bits.begin() + midpoint);

    // Recalculate metrics from scratch for the children.
    child1.calculateMetrics();
    child2.calculateMetrics();
}


// Generates the next generation of the population and swaps it with the current population.
// Uses selection followed by reproduction or crossover, and mutation.
void nextGeneration(const std::vector<Individual> &currentPop, std::vector<Individual> &nextPop) {
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    // Fill the next population.
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

            // Mutate and repair child1.
            child1.mutate();
            child1.repair();
            i++;

            // Mutate and repair child2 if there's space in the population.
            if (i < POPULATION_SIZE && &child1 != &child2) {
                child2.mutate();
                child2.repair();
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

    // Resize helper vector for repair function (included in timing).
    included_indices.resize(NUM_ITEMS);

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
        (sizeof(size_t) * result.selectedItems.capacity());
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
