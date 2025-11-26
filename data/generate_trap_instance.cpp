/**
 * Knapsack Problem Instance Generator (C++ Worker)
 *
 * This program generates a single, hard-to-solve 0/1 knapsack problem
 * instance and writes it to a specified text file.
 *
 * It creates an instance with a known optimal solution that will "trap"
 * a standard greedy (price/weight ratio) algorithm.
 *
 * COMPILE (from Python or manually):
 * g++ -O3 -std=c++17 -o generate_trap_instance generate_trap_instance.cpp
 *
 * USAGE (called by Python):
 * ./generate_trap_instance <filepath> <n> <capacity> <seed>
 *
 * OUTPUT FILE FORMAT:
 * Line 1: <n> <capacity> <max_weight> <min_weight> <optimum_value>
 * Line 2: <optimal_pick_1> <optimal_pick_2> ... (space-separated)
 * Lines 3+: <weight_i> <value_i>
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <numeric>
#include <algorithm>
#include <limits>

 // Use 64-bit integers for weights, profits, and capacity
using int64 = long long;

int main(int argc, char *argv[]) {
    // --- 1. Parse Command-Line Arguments ---
    if (argc != 5) {
        std::cerr << "Error: Expected 4 arguments: <filepath> <n> <capacity> <seed>\n";
        return 1;
    }

    std::string filepath = argv[1];
    int64 n = std::stoll(argv[2]);
    int64 base_capacity = std::stoll(argv[3]);
    int seed = std::stoi(argv[4]);

    if (n < 3) {
        std::cerr << "Error: n must be at least 3 to create the trap.\n";
        return 1;
    }

    // --- 2. Initialize RNG ---
    std::mt19937 rng(seed);

    // --- 3. Generate varied capacity based on base_capacity ---
    // Vary capacity randomly within ±20% of base_capacity to add diversity
    std::uniform_real_distribution<double> capacity_variation(0.8, 1.2);
    double capacity_factor = capacity_variation(rng);
    int64 capacity = (int64)(base_capacity * capacity_factor);
    capacity = std::max(100LL, capacity); // Ensure minimum capacity

    // --- 4. Define the "Greedy Trap" Structure by Construction ---
    // This method creates a provably correct trap instance without needing to solve it.
    // We create 3 special items and n-3 "filler" items that are impossible to pack.

    std::vector<int64> weights(n);
    std::vector<int64> profits(n);
    std::vector<int> best_picks;

    // Get 3 unique random indices for our special items
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    int idx_optimal = indices[0];
    int idx_trap_bait = indices[1];
    int idx_trap_blocker = indices[2];

    // 1. The "Optimal" Item
    // This single item fills the knapsack perfectly and provides the best possible value.
    // p/w ratio = 2
    weights[idx_optimal] = capacity;
    profits[idx_optimal] = capacity * 2;
    best_picks.push_back(idx_optimal);
    int64 best_price = capacity * 2;

    // 2. The "Trap Bait" Item
    // This item has a very high p/w ratio, making it irresistible to a greedy algorithm.
    // p/w ratio = (capacity + 1) / 2, which is very high.
    weights[idx_trap_bait] = 2;
    profits[idx_trap_bait] = capacity + 1;

    // 3. The "Trap Blocker" Item
    // This item is chosen by a greedy algorithm after the bait, but the combination
    // is suboptimal and invalid (overweight).
    // p/w ratio = capacity / (capacity - 1), which is slightly > 1.
    weights[idx_trap_blocker] = capacity - 1;
    profits[idx_trap_blocker] = capacity;

    // 4. "Filler" Items
    // These items are all impossible to pack, ensuring they don't interfere with the trap.
    std::uniform_int_distribution<int64> filler_w_dist(capacity + 1, capacity + 1000);
    for (int i = 3; i < n; ++i) {
        int idx_fill = indices[i];
        weights[idx_fill] = filler_w_dist(rng);
        profits[idx_fill] = 1; // Profit doesn't matter as they can't be picked.
    }

    // --- 5. Calculate max/min weights ---
    int64 maxWeight = 0;
    int64 minWeight = std::numeric_limits<int64>::max();
    for (int i = 0; i < n; ++i) {
        maxWeight = std::max(maxWeight, weights[i]);
        minWeight = std::min(minWeight, weights[i]);
    }

    // --- 6. Write to Output File ---
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open output file: " << filepath << "\n";
        return 2;
    }

    // Line 1: n capacity max_weight min_weight optimum_value
    outfile << n << " " << capacity << " " << maxWeight << " " << minWeight << " " << best_price << "\n";

    // Line 2: optimal picks (space-separated)
    for (size_t i = 0; i < best_picks.size(); ++i) {
        if (i > 0) outfile << " ";
        outfile << best_picks[i];
    }
    outfile << "\n";

    // Lines 3+: weight value pairs
    for (int i = 0; i < n; ++i) {
        outfile << weights[i] << " " << profits[i] << "\n";
    }

    outfile.close();
    return 0;
}
