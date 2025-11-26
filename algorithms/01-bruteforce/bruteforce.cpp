#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <numeric> // For iota, though not strictly needed here
#include <utility> // For std::pair
#include "../file_io.hpp"

using namespace std;
using int64 = long long;

// A result struct to hold all output, matching your desired format
struct Result {
    int64 maxValue;
    vector<int> selectedItems;
    int64 executionTime; // in microseconds
    size_t memoryUsed; // in bytes (approximate)
};

/**
 * @brief Recursive brute-force helper function.
 * @param capacity The remaining capacity of the knapsack.
 * @param weights Const reference to the item weights vector.
 * @param values Const reference to the item values vector.
 * @param n The index of the item currently being considered (we count down from n-1 to 0).
 * @return A pair containing (maxValue, selectedItems) for this subproblem.
 */
pair<int64, vector<int>> knapsackBruteForceRecursive(int64 capacity, const vector<int64> &weights,
    const vector<int64> &values, int n) {
    // Base case: No items left to consider or no capacity
    if (n < 0 || capacity == 0) {
        return { 0, {} }; // {value 0, empty item list}
    }

    // Case 1: Exclude the current item (item at index n)
    // We simply get the result from the next item down.
    pair<int64, vector<int>> excludeResult = knapsackBruteForceRecursive(capacity, weights, values, n - 1);

    // Case 2: Try to include the current item (item at index n)
    // First, check if it even fits.
    if (weights[n] > capacity) {
        // Doesn't fit, so we *must* exclude it.
        return excludeResult;
    }

    // It fits. Get the result from including it.
    // The new capacity is reduced, and we pass to the next item (n-1).
    pair<int64, vector<int>> includeResult = knapsackBruteForceRecursive(capacity - weights[n], weights, values, n - 1);

    // Add the current item's value and its index to the result
    includeResult.first += values[n];
    includeResult.second.push_back(n); // Add the index 'n'

    // Return the better of the two results (include vs. exclude)
    if (includeResult.first > excludeResult.first) {
        return includeResult;
    }
    else {
        return excludeResult;
    }
}

/**
 * @brief Wrapper function to time the brute-force algorithm and build the Result struct.
 */
Result solveKnapsackBruteForce(int64 capacity, const vector<int64> &weights, const vector<int64> &values) {
    Result result;
    int n = weights.size();

    auto start = chrono::high_resolution_clock::now();

    // Start the recursion from the last item (index n-1)
    pair<int64, vector<int>> solveResult = knapsackBruteForceRecursive(capacity, weights, values, n - 1);

    auto end = chrono::high_resolution_clock::now();

    result.maxValue = solveResult.first;
    result.selectedItems = solveResult.second;
    // The items might be in reverse order of consideration, which is fine
    // If you need them sorted by index:
    // sort(result.selectedItems.begin(), result.selectedItems.end());

    result.executionTime = chrono::duration_cast<chrono::microseconds>(end - start).count();

    // Approximate memory used by the input vectors and the final selected items list
    // This does NOT account for the O(n) recursive stack depth, which is the main memory cost.
    result.memoryUsed = (sizeof(int64) * (weights.size() + values.size())) +
        (sizeof(int) * result.selectedItems.size());

    return result;
}

int main(int argc, char *argv[]) {
    // Use fast I/O.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    // Load instance from file
    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    // Solve the knapsack problem
    Result result = solveKnapsackBruteForce(instance.capacity, instance.weights, instance.values);

    // Output results
    cout << result.maxValue << endl;
    cout << result.selectedItems.size() << endl;
    for (int idx : result.selectedItems) {
        cout << idx << " ";
    }
    if (!result.selectedItems.empty()) {
        cout << endl;
    }
    cout << result.executionTime << endl;
    cout << result.memoryUsed << endl;

    return 0;
}
