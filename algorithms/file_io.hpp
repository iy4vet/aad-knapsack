/**
 * Common file I/O helper for knapsack algorithms
 *
 * File format:
 * Line 1: <n> <capacity> <max_weight> <min_weight> <optimum_value>
 *         (optimum_value may be empty if unknown)
 * Line 2: <optimal_pick_1> <optimal_pick_2> ...
 *         (may be empty if unknown)
 * Lines 3+: <weight_i> <value_i>
 *           One line per item
 *
 * Usage:
 *   KnapsackInstance instance;
 *   if (!loadKnapsackInstance(argv[1], instance)) {
 *       return 1;
 *   }
 */

#ifndef FILE_IO_HPP
#define FILE_IO_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <limits>

using int64 = long long;

struct KnapsackInstance {
    size_t n;
    int64 capacity;
    int64 maxWeight;
    int64 minWeight;
    int64 optimumValue;  // -1 if unknown
    std::vector<int64> weights;
    std::vector<int64> values;
    std::vector<size_t> optimalPicks;  // empty if unknown
};

/**
 * @brief Load a knapsack instance from a text file
 * @param filepath Path to the text file
 * @param instance Output instance to populate
 * @return true if successful, false on error
 */
inline bool loadKnapsackInstance(const std::string &filepath, KnapsackInstance &instance) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filepath << std::endl;
        return false;
    }

    // Line 1: n capacity max_weight min_weight [optimum_value]
    std::string line1;
    if (!std::getline(file, line1)) {
        std::cerr << "Error: Could not read first line from file" << std::endl;
        return false;
    }

    std::istringstream iss1(line1);
    iss1 >> instance.n >> instance.capacity >> instance.maxWeight >> instance.minWeight;

    // Optimum value is optional (may be empty/missing)
    instance.optimumValue = -1;
    std::string optimumStr;
    if (iss1 >> optimumStr && !optimumStr.empty()) {
        try {
            instance.optimumValue = std::stoll(optimumStr);
        }
        catch (...) {
            instance.optimumValue = -1;
        }
    }

    // Line 2: optimal picks (space-separated indices, may be empty)
    std::string line2;
    if (!std::getline(file, line2)) {
        std::cerr << "Error: Could not read second line from file" << std::endl;
        return false;
    }

    instance.optimalPicks.clear();
    if (!line2.empty()) {
        std::istringstream iss2(line2);
        int idx;
        while (iss2 >> idx) {
            instance.optimalPicks.push_back(idx);
        }
    }

    // Lines 3+: weight value pairs
    instance.weights.resize(instance.n);
    instance.values.resize(instance.n);

    for (size_t i = 0; i < instance.n; ++i) {
        if (!(file >> instance.weights[i] >> instance.values[i])) {
            std::cerr << "Error: Could not read item " << i << " from file" << std::endl;
            return false;
        }
    }

    file.close();
    return true;
}

#endif // FILE_IO_HPP
