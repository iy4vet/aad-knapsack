#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <numeric> // For std::iota
#include "../file_io.hpp"

using namespace std;
using int64 = long long;

// A result struct to hold all output, matching your desired format
struct Result {
    int64 maxValue;
    vector<size_t> selectedItems;
    int64 executionTime; // in microseconds
    size_t memoryUsed; // in bytes (approximate)
};

/**
 * @brief Solves the 0/1 Knapsack problem using bottom-up Dynamic Programming (Tabulation).
 * @param capacity The total capacity of the knapsack.
 * @param weights Const reference to the item weights vector.
 * @param values Const reference to the item values vector.
 * @return A Result struct containing the solution, time, and memory.
 */
Result solveKnapsackDP(int64 capacity, const vector<int64> &weights, const vector<int64> &values) {
    Result result;
    size_t n = weights.size();

    auto start = chrono::high_resolution_clock::now();

    // Create the DP table. dp[i][w] will store the max value using
    // the first 'i' items with a capacity of 'w'.
    // We use (n+1) and (capacity+1) to handle 0-based indexing easily.
    vector<vector<int64>> dp(n + 1, vector<int64>(capacity + 1, 0));

    // Fill the table
    for (size_t i = 1; i <= n; ++i) {
        // Get the weight and value of the *current* item (i-1 for 0-based index)
        int64 currentWeight = weights[i - 1];
        int64 currentValue = values[i - 1];

        for (int64 w = 0; w <= capacity; ++w) {
            // Case 1: The current item is too heavy to fit (w < currentWeight)
            // So, we can't include it. The max value is the same as without it.
            if (currentWeight > w) {
                dp[i][w] = dp[i - 1][w];
            }
            // Case 2: The current item fits.
            // We must decide: is it better to include it or exclude it?
            else {
                // Value if we *exclude* the item
                int64 excludeValue = dp[i - 1][w];

                // Value if we *include* the item
                // This is the item's value + the max value from the remaining capacity
                int64 includeValue = currentValue + dp[i - 1][w - currentWeight];

                // Take the maximum of the two choices
                dp[i][w] = max(includeValue, excludeValue);
            }
        }
    }

    // The final answer is in the bottom-right corner
    result.maxValue = dp[n][capacity];

    // --- Backtrack to find the selected items ---
    int64 w = capacity; // Start at the final capacity
    for (size_t i = n; i > 0; --i) {
        // Compare the current cell with the one directly above it
        // If they are the same, it means item 'i' was *not* included
        if (dp[i][w] == dp[i - 1][w]) {
            // Item (i-1) was not included, just move up
        }
        // If they are different, it means item 'i' *was* included
        else {
            result.selectedItems.push_back(i - 1); // Add the 0-based index
            w -= weights[i - 1]; // Reduce the capacity we're looking for
        }
    }

    // The items are found in reverse order, so we reverse to get 0..n order
    reverse(result.selectedItems.begin(), result.selectedItems.end());

    auto end = chrono::high_resolution_clock::now();
    result.executionTime = chrono::duration_cast<chrono::microseconds>(end - start).count();

    // Approximate memory used by the DP table + input/output vectors
    size_t dpTableMemory = sizeof(int64) * (n + 1) * static_cast<size_t>(capacity + 1);
    size_t vectorMemory = (sizeof(int64) * (weights.size() + values.size())) +
        (sizeof(size_t) * result.selectedItems.size());
    result.memoryUsed = dpTableMemory + vectorMemory;

    return result;
}

int main(int argc, char *argv[]) {
    // Use fast I/O.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    // Solve the knapsack problem
    Result result = solveKnapsackDP(instance.capacity, instance.weights, instance.values);

    // Output results
    cout << result.maxValue << endl;
    cout << result.selectedItems.size() << endl;
    for (size_t idx : result.selectedItems) {
        cout << idx << " ";
    }
    if (!result.selectedItems.empty()) {
        cout << endl;
    }
    cout << result.executionTime << endl;
    cout << result.memoryUsed << endl;

    return 0;
}
