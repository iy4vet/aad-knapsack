/**
 * Knapsack "Easy" Problem Instance Generator (C++ Worker)
 *
 * This program generates a single, "easy-to-solve" 0/1 knapsack problem
 * instance and writes it to a specified text file.
 *
 * An "easy" instance has a pre-selected optimal solution where the chosen
 * items have a significantly better price-to-weight ratio than other items.
 *
 * COMPILE (from Python or manually):
 * g++ -O3 -std=c++17 -o generate_easy_instance generate_easy_instance.cpp
 *
 * USAGE (called by Python):
 * ./generate_easy_instance <filepath> <n> <seed>
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
#include <set>
#include <limits>

 // Use 64-bit integers for weights, profits, and capacity
using int64 = long long;

int main(int argc, char *argv[]) {
    // --- 1. Parse Command-Line Arguments ---
    if (argc != 4) {
        std::cerr << "Error: Expected 3 arguments: <filepath> <n> <seed>\n";
        return 1;
    }

    std::string filepath = argv[1];
    int64 n = std::stoll(argv[2]);
    int seed = std::stoi(argv[3]);

    // --- 2. Initialize RNGs ---
    std::mt19937 rng(seed);

    // --- 3. Determine k (number of selected items) ---
    int64 k;
    if (n <= 50) {
        int64 upper_bound = std::min((int64)n, std::max(1LL, (int64)(n * 0.5)));
        std::uniform_int_distribution<int64> k_dist(1, upper_bound);
        k = k_dist(rng);
    }
    else {
        int64 k_max = std::max(1LL, std::min((int64)n, (int64)(n * 0.1)));
        int64 upper_bound = std::max(1LL, std::min(k_max, (int64)(n * 0.02)));
        std::uniform_int_distribution<int64> k_dist(1, upper_bound);
        k = k_dist(rng);
    }
    k = std::max(1LL, k);

    // --- 4. Select Optimal Items ---
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);

    // Partial Fisher-Yates shuffle: only shuffle first k elements
    for (int64 i = 0; i < k; ++i) {
        std::uniform_int_distribution<int64> dist(i, n - 1);
        std::swap(indices[i], indices[dist(rng)]);
    }

    std::set<int> selected_indices;
    for (int64 i = 0; i < k; ++i) {
        selected_indices.insert(indices[i]);
    }

    // --- 5. Generate Weights and Calculate Capacity ---
    std::vector<int64> weights(n);
    int64 capacity = 0;
    int64 maxWeight = 0;
    int64 minWeight = std::numeric_limits<int64>::max();

    std::mt19937 r_weights(seed ^ 0xA5A5A5A5);
    std::uniform_int_distribution<int64> w_dist(1, 100);

    for (int i = 0; i < n; ++i) {
        weights[i] = w_dist(r_weights);
        maxWeight = std::max(maxWeight, weights[i]);
        minWeight = std::min(minWeight, weights[i]);
        if (selected_indices.count(i)) {
            capacity += weights[i];
        }
    }

    // --- 6. Generate Correlated Prices ---
    std::vector<int64> profits(n);
    int64 best_price = 0;
    const int M_LOW = 10;
    const int M_HIGH = 15;
    const int NOISE_RANGE = std::max(1, (int)(M_LOW * 0.1));

    std::mt19937 r_prices_noise(seed ^ 0x5A5A5A5A);
    std::uniform_int_distribution<int> noise_dist(-NOISE_RANGE, NOISE_RANGE);

    for (int i = 0; i < n; ++i) {
        int64 w = weights[i];
        int noise = noise_dist(r_prices_noise);
        int64 price;

        if (selected_indices.count(i)) {
            price = (w * M_HIGH) + noise;
            best_price += price;
        }
        else {
            price = (w * M_LOW) + noise;
        }
        profits[i] = std::max(1LL, price);
    }

    // --- 7. Prepare best picks list ---
    std::vector<int> best_picks(selected_indices.begin(), selected_indices.end());
    std::sort(best_picks.begin(), best_picks.end());

    // --- 8. Write to Output File ---
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
