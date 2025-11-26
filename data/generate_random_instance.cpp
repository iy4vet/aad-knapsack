/**
 * Knapsack "Random" Problem Instance Generator (C++ Worker)
 *
 * This program generates a single, random 0/1 knapsack problem instance
 * based on the algorithm from generator.cpp and writes it to a specified text file.
 *
 * The instance uses a class-based structure where items are divided into
 * multiple classes with weights/profits centered around fractions of capacity
 * (1/2, 1/4, 1/8, etc.) plus noise, along with a fraction of small items.
 *
 * COMPILE (from Python or manually):
 * g++ -O3 -std=c++17 -o generate_random_instance generate_random_instance.cpp
 *
 * USAGE (called by Python):
 * ./generate_random_instance <filepath> <n> <capacity> <seed>
 *
 * OUTPUT FILE FORMAT:
 * Line 1: <n> <capacity> <max_weight> <min_weight> (optimum unknown, left blank)
 * Line 2: (empty - optimal picks unknown)
 * Lines 3+: <weight_i> <value_i>
 *
 * Note: This generator does NOT compute the optimal solution.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
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
    int64 capacity = std::stoll(argv[3]);
    int seed = std::stoi(argv[4]);

    // --- 2. Initialize RNG ---
    std::mt19937 rng(seed);

    // --- 3. Define Generation Parameters ---
    // These match the original generator.cpp logic
    int classes = 3;  // Results in 2 classes (classes - 1), so we'll use 2
    double frac = 0.1;  // 10% of items will be small
    double eps = 0.0001;  // Small epsilon for perturbation
    int64 small = 100;  // Range for small random values [1, small]

    // --- 4. Generate Items ---
    std::vector<int64> weights(n);
    std::vector<int64> profits(n);

    std::uniform_int_distribution<int64> small_dist(1, small);

    int64 amount_small = (int64)(n * frac);
    int64 amount_per_class = (n - amount_small) / classes;

    double denominator = 2.0;
    int64 item_idx = 0;

    // Generate items for each class
    for (int j = 0; j < classes; ++j) {
        int64 class_count = (j < classes - 1) ? amount_per_class : (n - amount_small - item_idx);

        for (int64 i = 0; i < class_count && item_idx < n - amount_small; ++i) {
            int64 num1 = small_dist(rng);
            int64 num2 = small_dist(rng);

            // Weight and profit centered around (1/denominator + eps) * capacity + noise
            int64 base_value = (int64)((1.0 / denominator + eps) * capacity);
            weights[item_idx] = base_value + num1;
            profits[item_idx] = base_value + num2;

            item_idx++;
        }

        denominator *= 2.0;
    }

    // Generate small items
    for (int64 i = item_idx; i < n; ++i) {
        int64 num1 = small_dist(rng);
        int64 num2 = small_dist(rng);
        weights[i] = num1;
        profits[i] = num2;
    }

    // --- 5. Calculate max/min weights ---
    int64 maxWeight = 0;
    int64 minWeight = std::numeric_limits<int64>::max();
    for (int64 i = 0; i < n; ++i) {
        maxWeight = std::max(maxWeight, weights[i]);
        minWeight = std::min(minWeight, weights[i]);
    }

    // --- 6. Write to Output File ---
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open output file: " << filepath << "\n";
        return 2;
    }

    // Line 1: n capacity max_weight min_weight (optimum unknown, left blank)
    outfile << n << " " << capacity << " " << maxWeight << " " << minWeight << "\n";

    // Line 2: optimal picks (empty - unknown)
    outfile << "\n";

    // Lines 3+: weight value pairs
    for (int64 i = 0; i < n; ++i) {
        outfile << weights[i] << " " << profits[i] << "\n";
    }

    outfile.close();
    return 0;
}
