#include <bits/stdc++.h>
#include <chrono>
#include "../file_io.hpp"

using namespace std;
using namespace std::chrono;
using int64 = long long;

// Global const references to avoid copying
const vector<int64> *g_weights;
const vector<int64> *g_values;
vector<vector<int64>> memo;
size_t n;
int64 capacity;

// Accessor helpers for cleaner code
inline const vector<int64> &weights() { return *g_weights; }
inline const vector<int64> &values() { return *g_values; }

// Recursive function with memoization
int64 knapsack_memoization(size_t i, int64 remaining_capacity) {
    if (i == 0 || remaining_capacity == 0)
        return 0;

    if (memo[i][remaining_capacity] != -1)
        return memo[i][remaining_capacity];

    if (weights()[i - 1] > remaining_capacity)
        return memo[i][remaining_capacity] = knapsack_memoization(i - 1, remaining_capacity);
    else {
        int64 include_item = values()[i - 1] + knapsack_memoization(i - 1, remaining_capacity - weights()[i - 1]);
        int64 exclude_item = knapsack_memoization(i - 1, remaining_capacity);
        return memo[i][remaining_capacity] = max(include_item, exclude_item);
    }
}

// Function to reconstruct the chosen items
vector<size_t> reconstruct_solution() {
    vector<size_t> selected_items;
    size_t i = n;
    int64 w = capacity;

    while (i > 0 && w > 0) {
        if (memo[i][w] != memo[i - 1][w]) {
            selected_items.push_back(i - 1);
            w -= weights()[i - 1];
        }
        i--;
    }
    reverse(selected_items.begin(), selected_items.end());
    return selected_items;
}


int main(int argc, char *argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    n = instance.n;
    capacity = instance.capacity;
    g_weights = &instance.weights;
    g_values = &instance.values;

    auto start = high_resolution_clock::now();

    memo.assign(n + 1, vector<int64>(capacity + 1, -1));
    int64 max_value = knapsack_memoization(n, capacity);
    vector<size_t> selected_items = reconstruct_solution();
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();

    // Approximate memory used (arrays + memo table)
    size_t memory_used = sizeof(int64) * ((n + 1) * (capacity + 1)) + sizeof(int64) * n * 2;

    cout << max_value << "\n";
    cout << selected_items.size() << "\n";
    for (size_t i = 0; i < selected_items.size(); i++) {
        cout << selected_items[i];
        if (i + 1 < selected_items.size()) cout << " ";
    }
    cout << "\n";
    cout << duration << "\n";
    cout << memory_used << "\n";

    return 0;
}
