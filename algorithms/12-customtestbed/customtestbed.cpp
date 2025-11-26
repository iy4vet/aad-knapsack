#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <numeric>
#include <cstring>
#include <cmath>
#include <limits>
#include <set>
#include <cstdlib>
#include "../file_io.hpp"

// Use int64 for large numbers
using int64 = long long;

struct Result {
    int64 maxValue;
    std::vector<int> selectedItems;
    long long executionTime;
    size_t memoryUsed;
};

struct ItemProperty {
    int id;
    float ratio;
    int64 weight;
    int64 value;
};

// --- Adaptive Hyperparameters ---
size_t POPULATION_SIZE = 0;
size_t MAX_GENERATIONS = 0;
size_t NUM_ISLANDS = 1;
size_t ISLAND_SIZE = 0;
size_t MIGRATION_INTERVAL = 0;
unsigned int SEED = std::random_device()();

// Tuning
size_t LAGRANGIAN_ITERATIONS = 500;
double BASE_SA_TEMP = 50.0;
double SA_COOLING_RATE = 0.90;
size_t SA_ITERATIONS = 0;

// Controls how frequently repair is called across generations (to save time)
size_t REPAIR_INTERVAL = 3;

double BASE_MUTATION_RATE = 0.05;
double CORE_MUTATION_PROB;
double NON_CORE_MUTATION_PROB;
// Local search control
double LOCAL_SEARCH_PROB = 0.0;

// --- Global Data ---
int64 FULL_KNAPSACK_CAPACITY;
std::vector<int64> FULL_ITEM_WEIGHTS;
std::vector<int64> FULL_ITEM_VALUES;
size_t FULL_NUM_ITEMS;

int64 KNAPSACK_CAPACITY;
std::vector<int64> ITEM_WEIGHTS;
std::vector<int64> ITEM_VALUES;
size_t NUM_ITEMS; // N'
int64 GLOBAL_LOWER_BOUND = 0;
int64 GLOBAL_UPPER_BOUND = std::numeric_limits<int64>::max();

std::vector<double> REDUCED_COSTS;
enum ItemType { CORE, X1_HIGH, X0_LOW };
std::vector<uint8_t> ITEM_CLASSIFICATION;

std::vector<int> v_w_order_indices;
std::vector<int> rcbo_order_indices;

std::vector<size_t> core_to_original_index_map;
std::vector<int> fixed_one_items;
int64 fixed_items_value = 0;
int64 fixed_items_weight = 0;

std::mt19937 rng;

class Individual {
public:
    std::vector<int> selectedItemIndices;
    std::vector<bool> active_mask;

    int64 totalValue = 0;
    int64 totalWeight = 0;
    mutable int64 cachedFitness = -1;
    mutable bool fitnessValid = false;

    Individual() {}

    void init(size_t n) {
        active_mask.assign(n, false);
        // Minimal reservation to save memory on large N
        if (n < 1000) selectedItemIndices.reserve(n / 2);
    }

    void calculateMetrics() {
        totalValue = 0;
        totalWeight = 0;
        for (int idx : selectedItemIndices) {
            totalValue += ITEM_VALUES[idx];
            totalWeight += ITEM_WEIGHTS[idx];
        }
        fitnessValid = false;
    }

    int64 getFitness() const {
        if (fitnessValid) return cachedFitness;
        if (totalWeight > KNAPSACK_CAPACITY) cachedFitness = 0;
        else cachedFitness = totalValue;
        fitnessValid = true;
        return cachedFitness;
    }

    void add(int item_index) {
        if (!active_mask[item_index]) {
            active_mask[item_index] = true;
            selectedItemIndices.push_back(item_index);
        }
    }

    void remove_fast(int item_index) {
        if (active_mask[item_index]) {
            active_mask[item_index] = false;
            // O(1) remove order-independent
            for (size_t i = 0; i < selectedItemIndices.size(); ++i) {
                if (selectedItemIndices[i] == item_index) {
                    selectedItemIndices[i] = selectedItemIndices.back();
                    selectedItemIndices.pop_back();
                    break;
                }
            }
        }
    }

    void repair() {
        // Phase 1: Remove worst (lowest Value/Weight)
        if (totalWeight > KNAPSACK_CAPACITY) {
            // Only sort if we actually need to remove stuff
            std::vector<std::pair<float, int>> current_items;
            current_items.reserve(selectedItemIndices.size());
            for (int idx : selectedItemIndices) {
                float r = (ITEM_WEIGHTS[idx] > 0) ? (float)ITEM_VALUES[idx] / ITEM_WEIGHTS[idx] : 0.0f;
                current_items.push_back({ r, idx });
            }
            std::sort(current_items.begin(), current_items.end());

            for (const auto &p : current_items) {
                if (totalWeight <= KNAPSACK_CAPACITY) break;
                active_mask[p.second] = false;
                totalWeight -= ITEM_WEIGHTS[p.second];
                totalValue -= ITEM_VALUES[p.second];
            }

            // Rebuild index list
            selectedItemIndices.clear();
            for (size_t i = 0; i < NUM_ITEMS; ++i) {
                if (active_mask[i]) selectedItemIndices.push_back(i);
            }
        }

        // Phase 2: Greedy Fill
        if (totalWeight < KNAPSACK_CAPACITY) {
            for (int idx : v_w_order_indices) {
                if (totalWeight + ITEM_WEIGHTS[idx] <= KNAPSACK_CAPACITY) {
                    if (!active_mask[idx]) {
                        active_mask[idx] = true;
                        selectedItemIndices.push_back(idx);
                        totalWeight += ITEM_WEIGHTS[idx];
                        totalValue += ITEM_VALUES[idx];
                    }
                }
                if (totalWeight == KNAPSACK_CAPACITY) break;
            }
        }
        fitnessValid = false;
        if (totalWeight <= KNAPSACK_CAPACITY && totalValue > GLOBAL_LOWER_BOUND) {
            GLOBAL_LOWER_BOUND = totalValue;
        }
    }

    void mutate() {
        if (NUM_ITEMS == 0) return;
        std::uniform_real_distribution<double> probDist(0.0, 1.0);
        std::uniform_int_distribution<int> dist(0, NUM_ITEMS - 1);

        // Single mutation event for speed
        int item_index = dist(rng);
        double prob = (ITEM_CLASSIFICATION[item_index] == CORE) ? CORE_MUTATION_PROB : NON_CORE_MUTATION_PROB;

        if (probDist(rng) < prob) {
            if (active_mask[item_index]) {
                remove_fast(item_index);
                totalWeight -= ITEM_WEIGHTS[item_index];
                totalValue -= ITEM_VALUES[item_index];
            }
            else {
                if (totalWeight + ITEM_WEIGHTS[item_index] <= KNAPSACK_CAPACITY) {
                    add(item_index);
                    totalWeight += ITEM_WEIGHTS[item_index];
                    totalValue += ITEM_VALUES[item_index];
                }
            }
            fitnessValid = false;
        }
    }

    void localSearch() {
        if (NUM_ITEMS == 0 || SA_ITERATIONS == 0) return;
        double temp = BASE_SA_TEMP;
        std::uniform_real_distribution<double> prob(0.0, 1.0);
        std::uniform_int_distribution<int> index_dist(0, NUM_ITEMS - 1);

        int64 currentFitness = getFitness();

        for (size_t i = 0; i < SA_ITERATIONS; ++i) {
            int flip_idx = index_dist(rng);
            bool was_selected = active_mask[flip_idx];
            int64 w_delta = was_selected ? -ITEM_WEIGHTS[flip_idx] : ITEM_WEIGHTS[flip_idx];
            int64 v_delta = was_selected ? -ITEM_VALUES[flip_idx] : ITEM_VALUES[flip_idx];

            if (!was_selected && totalWeight + w_delta > KNAPSACK_CAPACITY) continue;

            int64 newFitness = currentFitness + v_delta;
            int64 delta = newFitness - currentFitness;

            if (delta > 0 || prob(rng) < std::exp(delta / temp)) {
                if (was_selected) remove_fast(flip_idx);
                else add(flip_idx);

                totalWeight += w_delta;
                totalValue += v_delta;
                currentFitness = newFitness;
            }
            temp *= SA_COOLING_RATE;
        }
        fitnessValid = false;
    }
};

size_t hammingDist(const Individual &a, const Individual &b) {
    size_t dist = 0;
    const Individual *small = (a.selectedItemIndices.size() < b.selectedItemIndices.size()) ? &a : &b;
    const Individual *large = (a.selectedItemIndices.size() < b.selectedItemIndices.size()) ? &b : &a;

    for (int idx : small->selectedItemIndices) {
        if (!large->active_mask[idx]) dist++;
    }
    for (int idx : large->selectedItemIndices) {
        if (!small->active_mask[idx]) dist++;
    }
    return dist;
}

// --- Preprocessing ---

std::pair<double, int64> lagrangianSubproblem(double u) {
    // Return fractional contribution so Lagrangian upper bound preserves precision
    double val = 0.0;
    int64 w = 0;
    for (size_t i = 0; i < FULL_NUM_ITEMS; ++i) {
        double red = (double)FULL_ITEM_VALUES[i] - u * (double)FULL_ITEM_WEIGHTS[i];
        if (red > 0.0) {
            val += red; // keep fractional part; UB will be ceiled later
            w += FULL_ITEM_WEIGHTS[i];
        }
    }

    return { val, w };
}

void preprocess() {
    // 1. Greedy Bound
    std::vector<std::pair<double, int>> ratios(FULL_NUM_ITEMS);
    for (size_t i = 0; i < FULL_NUM_ITEMS; ++i) {
        double r = (FULL_ITEM_WEIGHTS[i] > 0) ? (double)FULL_ITEM_VALUES[i] / FULL_ITEM_WEIGHTS[i] : 0;
        ratios[i] = { r, (int)i };
    }
    std::sort(ratios.rbegin(), ratios.rend());

    int64 cw = 0;
    int64 cv = 0;
    for (const auto &p : ratios) {
        if (cw + FULL_ITEM_WEIGHTS[p.second] <= FULL_KNAPSACK_CAPACITY) {
            cw += FULL_ITEM_WEIGHTS[p.second];
            cv += FULL_ITEM_VALUES[p.second];
        }
    }
    GLOBAL_LOWER_BOUND = cv;

    // 2. Lagrangian
    double u = 0.0;
    double bestZUB = std::numeric_limits<double>::infinity();
    double best_u = u;
    double step = 2.0;

    size_t max_iter = (FULL_NUM_ITEMS > 50000) ? 100 : 500;

    for (size_t iter = 0; iter < max_iter; ++iter) {
        auto subp = lagrangianSubproblem(u);
        double sub_val = subp.first;
        int64 sub_w = subp.second;
        double Z_u = sub_val + u * (double)FULL_KNAPSACK_CAPACITY;

        if (Z_u < bestZUB) {
            bestZUB = Z_u;
            best_u = u;
        }

        // Use ceiling to ensure integer upper bound
        GLOBAL_UPPER_BOUND = (bestZUB < std::numeric_limits<double>::infinity()) ? (int64)std::ceil(bestZUB) : std::numeric_limits<int64>::max();

        if (bestZUB > 0.0 && (bestZUB - (double)GLOBAL_LOWER_BOUND) / bestZUB < 1e-8) break;

        int64 g = sub_w - FULL_KNAPSACK_CAPACITY;
        if (g == 0) break;

        u = std::max(0.0, u - step * (Z_u - GLOBAL_LOWER_BOUND) / (double)(g * g + 1));
        step *= 0.90;
    }

    // 3. Variable Fixing
    fixed_items_value = 0;
    fixed_items_weight = 0;
    std::vector<int64> core_weights, core_values;

    // Use the best dual value for variable fixing
    for (size_t i = 0; i < FULL_NUM_ITEMS; ++i) {
        double reduced = (double)FULL_ITEM_VALUES[i] - best_u * (double)FULL_ITEM_WEIGHTS[i];
        if (bestZUB - reduced < (double)GLOBAL_LOWER_BOUND - 1e-9) {
            fixed_items_value += FULL_ITEM_VALUES[i];
            fixed_items_weight += FULL_ITEM_WEIGHTS[i];
            fixed_one_items.push_back(i);
        }
        else if (bestZUB + reduced < (double)GLOBAL_LOWER_BOUND - 1e-9) {
            // Fixed to 0
        }
        else {
            core_weights.push_back(FULL_ITEM_WEIGHTS[i]);
            core_values.push_back(FULL_ITEM_VALUES[i]);
            core_to_original_index_map.push_back(i);
            REDUCED_COSTS.push_back(reduced);
        }
    }

    ITEM_WEIGHTS = std::move(core_weights);
    ITEM_VALUES = std::move(core_values);
    NUM_ITEMS = ITEM_WEIGHTS.size();
    KNAPSACK_CAPACITY = FULL_KNAPSACK_CAPACITY - fixed_items_weight;

    if (KNAPSACK_CAPACITY < 0) NUM_ITEMS = 0;

    // 4. Heuristics
    if (NUM_ITEMS > 0) {
        ITEM_CLASSIFICATION.resize(NUM_ITEMS);
        v_w_order_indices.resize(NUM_ITEMS);
        rcbo_order_indices.resize(NUM_ITEMS);
        std::iota(v_w_order_indices.begin(), v_w_order_indices.end(), 0);
        std::iota(rcbo_order_indices.begin(), rcbo_order_indices.end(), 0);

        std::sort(v_w_order_indices.begin(), v_w_order_indices.end(), [&](int a, int b) {
            double r1 = (ITEM_WEIGHTS[a] > 0) ? (double)ITEM_VALUES[a] / ITEM_WEIGHTS[a] : 0;
            double r2 = (ITEM_WEIGHTS[b] > 0) ? (double)ITEM_VALUES[b] / ITEM_WEIGHTS[b] : 0;
            return r1 > r2;
            });

        std::sort(rcbo_order_indices.begin(), rcbo_order_indices.end(), [&](int a, int b) {
            return REDUCED_COSTS[a] > REDUCED_COSTS[b];
            });

        int core_cnt = 0;
        double gap = (double)GLOBAL_UPPER_BOUND - GLOBAL_LOWER_BOUND;
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            if (std::abs(REDUCED_COSTS[i]) < gap * 0.1) {
                ITEM_CLASSIFICATION[i] = CORE;
                core_cnt++;
            }
            else if (REDUCED_COSTS[i] > 0) ITEM_CLASSIFICATION[i] = X1_HIGH;
            else ITEM_CLASSIFICATION[i] = X0_LOW;
        }

        if (core_cnt == 0) core_cnt = 1;
        double scale = (double)NUM_ITEMS / core_cnt;
        CORE_MUTATION_PROB = std::min(0.5, BASE_MUTATION_RATE * scale);
        NON_CORE_MUTATION_PROB = BASE_MUTATION_RATE * 0.01;
    }
}

Individual createIndividualFromOrder(const std::vector<int> &order) {
    Individual ind;
    ind.init(NUM_ITEMS);
    for (int idx : order) {
        if (ind.totalWeight + ITEM_WEIGHTS[idx] <= KNAPSACK_CAPACITY) {
            ind.add(idx);
            ind.totalWeight += ITEM_WEIGHTS[idx];
            ind.totalValue += ITEM_VALUES[idx];
        }
    }
    ind.fitnessValid = false;
    return ind;
}

void crossover(const Individual &p1, const Individual &p2, Individual &c1) {
    c1.init(NUM_ITEMS);
    // Fast Union
    for (int idx : p1.selectedItemIndices) c1.add(idx);
    for (int idx : p2.selectedItemIndices) c1.add(idx);
    c1.calculateMetrics();
}

void nextGeneration(std::vector<Individual> &pop, std::vector<Individual> &nextPop, bool doRepair) {
    // Simple elitism: carry over the best individual
    auto bestIt = std::max_element(pop.begin(), pop.end(), [](const Individual &a, const Individual &b) { return a.getFitness() < b.getFitness(); });
    if (bestIt != pop.end()) nextPop[0] = *bestIt;

    // Helper for tournament selection
    auto tournament = [&](int k = 2) -> const Individual & {
        std::uniform_int_distribution<size_t> idxDist(0, pop.size() - 1);
        const Individual *cur = &pop[idxDist(rng)];
        for (int i = 1; i < k; ++i) {
            const Individual *cand = &pop[idxDist(rng)];
            if (cand->getFitness() > cur->getFitness()) cur = cand;
        }
        return *cur;
        };

    std::uniform_real_distribution<double> localProb(0.0, 1.0);
    size_t insertPos = 1;
    while (insertPos < pop.size()) {
        const Individual &parent1 = tournament(3);
        const Individual &parent2 = tournament(3);

        // generate two children from parents (union crossover)
        Individual child1, child2;
        child1.init(NUM_ITEMS);
        child2.init(NUM_ITEMS);

        // Fast union: merge selected indices from both
        for (int idx : parent1.selectedItemIndices) child1.add(idx);
        for (int idx : parent2.selectedItemIndices) child1.add(idx);
        child1.calculateMetrics();

        child2 = child1; // copy


        child1.mutate();
        if (doRepair) child1.repair();
        if (localProb(rng) < LOCAL_SEARCH_PROB) child1.localSearch();

        if (insertPos < nextPop.size()) nextPop[insertPos++] = std::move(child1);

        if (insertPos < nextPop.size()) {
            child2.mutate();
            if (doRepair) child2.repair();
            if (localProb(rng) < LOCAL_SEARCH_PROB) child2.localSearch();
            nextPop[insertPos++] = std::move(child2);
        }
    }
}

void migrate(std::vector<std::vector<Individual>> &islands) {
    for (size_t i = 0; i < NUM_ISLANDS; ++i) {
        size_t next = (i + 1) % NUM_ISLANDS;
        auto best_it = std::max_element(islands[i].begin(), islands[i].end(),
            [](auto &a, auto &b) { return a.getFitness() < b.getFitness(); });
        auto worst_it = std::min_element(islands[next].begin(), islands[next].end(),
            [](auto &a, auto &b) { return a.getFitness() < b.getFitness(); });
        *worst_it = *best_it;
    }
}

Result solve() {
    Result result;
    auto start = std::chrono::high_resolution_clock::now();

    preprocess();

    // --- CALIBRATED SCALING ---
    // N' = NUM_ITEMS (Core size)

    // 1. Instant Win Check
    // If pre-processing solved it or gap is zero, we are done.
    // We construct the seeds first to check their quality.
    Individual greedy = createIndividualFromOrder(v_w_order_indices);
    Individual rcbo = createIndividualFromOrder(rcbo_order_indices);
    int64 seedMax = std::max(greedy.getFitness(), rcbo.getFitness());

    // The gap might not be zero, but if our seed hits the UB, it's optimal.
    // Or if problem was fully reduced (N=0).
    if (NUM_ITEMS == 0 || (GLOBAL_UPPER_BOUND > 0 && seedMax + fixed_items_value >= GLOBAL_UPPER_BOUND)) {
        // Found optimal immediately
        result.maxValue = seedMax + fixed_items_value;
        Individual &best = (greedy.getFitness() > rcbo.getFitness()) ? greedy : rcbo;

        for (int idx : fixed_one_items) result.selectedItems.push_back(idx);
        for (int idx : best.selectedItemIndices) result.selectedItems.push_back(core_to_original_index_map[idx]);
        std::sort(result.selectedItems.begin(), result.selectedItems.end());
        auto end = std::chrono::high_resolution_clock::now();
        result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        result.memoryUsed = sizeof(Individual) * 2 + NUM_ITEMS * 8;
        return result;
    }

    // 2. Gap-based scaling
    double gap_ratio = 0.0;
    if (GLOBAL_UPPER_BOUND > 0) gap_ratio = (double)(GLOBAL_UPPER_BOUND - (GLOBAL_LOWER_BOUND + fixed_items_value)) / GLOBAL_UPPER_BOUND;

    // 3. Islands: Only for MASSIVE problems.
    // Previously we switched at ~2000. Now we switch at 50,000.
    // This keeps overhead zero for all test cases in your image (max N=10k).
    // Use a smaller island threshold so we don't add unnecessary parallelism overhead
    NUM_ISLANDS = 1 + (NUM_ITEMS / 20000);
    if (NUM_ISLANDS > 4) NUM_ISLANDS = 4;

    // 4. Population
    // Heuristic population sizing (match customalgorithm style for runtime parity)
    size_t target_pop = POPULATION_SIZE > 0 ? POPULATION_SIZE : 0;
    if (target_pop == 0) {
        if (NUM_ITEMS < 100) target_pop = 20;
        else if (NUM_ITEMS < 1000) target_pop = 50;
        else if (NUM_ITEMS < 10000) target_pop = 100;
        else target_pop = 150;
    }
    if (target_pop > 150) target_pop = 150;
    // Cap population for very large cores for runtime stability
    if (NUM_ITEMS > 20000) target_pop = std::min(target_pop, (size_t)80);
    ISLAND_SIZE = std::max((size_t)10, target_pop / NUM_ISLANDS);

    // 5. SA Iterations: Only if gap is significant
    SA_ITERATIONS = 0;
    if (gap_ratio > 0.005) { // If gap > 0.5%, use SA
        // Use SA only for moderate-size cores; it's expensive on extremely large cores
        if (NUM_ITEMS <= 50000) {
            SA_ITERATIONS = 2 + (size_t)(10.0 * (1.0 - std::exp(-((double)NUM_ITEMS) / 5000.0)));
        }
        else {
            SA_ITERATIONS = 0; // disable SA for very large cores to keep runtime low
        }
    }

    // 6. Generations
    MAX_GENERATIONS = 30 + (size_t)(6.0 * std::log(NUM_ITEMS + 1.0));
    if (MAX_GENERATIONS > 100) MAX_GENERATIONS = 100;

    if (POPULATION_SIZE > 0) ISLAND_SIZE = POPULATION_SIZE / std::max((size_t)1, NUM_ISLANDS);

    // Initialize islands and pick elites/greedy seeds
    std::vector<std::vector<Individual>> islands(NUM_ISLANDS);
    for (size_t i = 0; i < NUM_ISLANDS; ++i) {
        islands[i].resize(ISLAND_SIZE);
        for (size_t j = 0; j < ISLAND_SIZE; ++j) {
            islands[i][j].init(NUM_ITEMS);
            // Seed first two, rest mutations
            if (j == 0) islands[i][j] = greedy;
            else if (j == 1) islands[i][j] = rcbo;
            else {
                islands[i][j] = (j % 2 == 0) ? greedy : rcbo;
                islands[i][j].mutate();
                islands[i][j].repair();
            }
        }
    }
    std::vector<std::vector<Individual>> nextIslands = islands;

    // Pre-allocate random distributions for selection/local search
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    std::uniform_int_distribution<int> selectDist(0, ISLAND_SIZE - 1);

    // Local search probability: only run localSearch occasionally, tuned by gap_ratio
    if (SA_ITERATIONS == 0) {
        LOCAL_SEARCH_PROB = 0.0;
    }
    else {
        // Increase prob for small cores and noticeable gaps
        LOCAL_SEARCH_PROB = std::min(0.30, 0.02 + 0.30 * std::min(1.0, gap_ratio * 100.0));
    }

    for (size_t g = 0; g < MAX_GENERATIONS; ++g) {
        for (size_t i = 0; i < NUM_ISLANDS; ++i) {
            bool doRepair = (g % REPAIR_INTERVAL) == 0; // run repair every few generations
            // Implement tournament selection + deterministic crowding + elitism
            // We'll pass along selection helpers through globals
            nextGeneration(islands[i], nextIslands[i], doRepair);
            islands[i] = nextIslands[i];
        }
        if (NUM_ISLANDS > 1 && g % 10 == 0) migrate(islands);
    }

    int64 maxVal = -1;
    Individual bestInd;
    for (auto &isle : islands) {
        for (auto &ind : isle) {
            if (ind.getFitness() > maxVal) {
                maxVal = ind.getFitness();
                bestInd = ind;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    result.maxValue = maxVal + fixed_items_value;
    result.selectedItems.reserve(bestInd.selectedItemIndices.size() + fixed_one_items.size());

    for (int idx : fixed_one_items) result.selectedItems.push_back(idx);
    for (int idx : bestInd.selectedItemIndices) result.selectedItems.push_back(core_to_original_index_map[idx]);
    std::sort(result.selectedItems.begin(), result.selectedItems.end());

    // Final validation: ensure full solution (fixed + core) respects the full capacity
    int64 full_weight = 0, full_value = 0;
    std::set<int> fixedSet(fixed_one_items.begin(), fixed_one_items.end());
    for (int idx : result.selectedItems) {
        full_weight += FULL_ITEM_WEIGHTS[idx];
        full_value += FULL_ITEM_VALUES[idx];
    }

    if (full_weight > FULL_KNAPSACK_CAPACITY) {
        // Deterministic repair: remove the lowest value/weight items (not prefixed to fixed ones)
        std::vector<std::pair<double, int>> removable; // ratio, global index
        for (int idx : result.selectedItems) {
            if (fixedSet.count(idx)) continue; // prefer not to remove fixed items
            double r = (FULL_ITEM_WEIGHTS[idx] > 0) ? (double)FULL_ITEM_VALUES[idx] / (double)FULL_ITEM_WEIGHTS[idx] : 0.0;
            removable.push_back({ r, idx });
        }
        std::sort(removable.begin(), removable.end()); // ascending, remove lowest ratio first

        size_t rem_pos = 0;
        while (full_weight > FULL_KNAPSACK_CAPACITY && rem_pos < removable.size()) {
            int gidx = removable[rem_pos++].second;
            // remove from selected items
            auto it = std::find(result.selectedItems.begin(), result.selectedItems.end(), gidx);
            if (it != result.selectedItems.end()) result.selectedItems.erase(it);
            full_weight -= FULL_ITEM_WEIGHTS[gidx];
            full_value -= FULL_ITEM_VALUES[gidx];
        }

        // If still overweight, reluctantly remove fixed items (should not happen if fixing is correct)
        if (full_weight > FULL_KNAPSACK_CAPACITY) {
            std::vector<std::pair<double, int>> fixedRem; // ratio, idx
            for (int idx : result.selectedItems) {
                double r = (FULL_ITEM_WEIGHTS[idx] > 0) ? (double)FULL_ITEM_VALUES[idx] / (double)FULL_ITEM_WEIGHTS[idx] : 0.0;
                fixedRem.push_back({ r, idx });
            }
            std::sort(fixedRem.begin(), fixedRem.end());
            size_t pos = 0;
            while (full_weight > FULL_KNAPSACK_CAPACITY && pos < fixedRem.size()) {
                int gidx = fixedRem[pos++].second;
                auto it = std::find(result.selectedItems.begin(), result.selectedItems.end(), gidx);
                if (it != result.selectedItems.end()) result.selectedItems.erase(it);
                full_weight -= FULL_ITEM_WEIGHTS[gidx];
                full_value -= FULL_ITEM_VALUES[gidx];
            }
        }

        // Update maxValue to reflect repair
        result.maxValue = full_value;
        if (std::getenv("DEBUG_CUSTOMTESTBED") != nullptr) {
            std::cerr << "[customtestbed] Final repair applied: new_value=" << result.maxValue
                << " new_weight=" << full_weight << " capacity=" << FULL_KNAPSACK_CAPACITY << "\n";
        }
    }

    result.memoryUsed = (sizeof(Individual) + NUM_ITEMS / 8 + NUM_ITEMS * 4) * ISLAND_SIZE * NUM_ISLANDS;
    return result;
}

std::string parseArguments(int argc, char *argv[]) {
    std::string filepath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--population_size" && i + 1 < argc) POPULATION_SIZE = std::stoul(argv[++i]);
        else if (arg == "--max_generations" && i + 1 < argc) MAX_GENERATIONS = std::stoul(argv[++i]);
        else if (arg == "--seed" && i + 1 < argc) SEED = std::stoul(argv[++i]);
        else if (arg[0] != '-' && filepath.empty()) filepath = arg;
    }
    return filepath;
}

int main(int argc, char *argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::string filepath = parseArguments(argc, argv);

    if (filepath.empty()) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [options]" << std::endl;
        return 1;
    }

    rng.seed(SEED);

    // Load problem instance from file.
    KnapsackInstance instance;
    if (!loadKnapsackInstance(filepath, instance)) {
        return 1;
    }

    FULL_NUM_ITEMS = instance.n;
    FULL_KNAPSACK_CAPACITY = instance.capacity;
    FULL_ITEM_WEIGHTS = std::move(instance.weights);
    FULL_ITEM_VALUES = std::move(instance.values);

    Result res = solve();

    std::cout << res.maxValue << "\n";
    std::cout << res.selectedItems.size() << "\n";
    for (size_t i = 0; i < res.selectedItems.size(); ++i) {
        std::cout << res.selectedItems[i] << (i == res.selectedItems.size() - 1 ? "" : " ");
    }
    std::cout << "\n" << res.executionTime << "\n" << res.memoryUsed << "\n";
    return 0;
}
