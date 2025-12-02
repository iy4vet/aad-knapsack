
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sys/resource.h>
#include "../file_io.hpp"

using namespace std;
using int64 = long long;

// --- Structures ---
struct Item {
    size_t id;     // Original index before sorting
    int64 w;       // Weight
    int64 v;       // Value
    double r;      // Value-to-weight ratio
};

// --- Global Variables ---
size_t n;                 // Number of items
int64 cap;                // Knapsack capacity
int64 maxP = 0;           // Maximum profit found so far
vector<Item> items;       // Items sorted by ratio (descending)
vector<bool> best;        // Best solution found (indexed by original ID)
vector<bool> curr;        // Current solution state during recursion (indexed by original ID)

// --- Comparator for Sorting ---
// Sort items by value-to-weight ratio in descending order
bool cmpI(const Item &a, const Item &b) {
    return a.r > b.r;
}

// --- Upper Bound Calculation ---
// Calculates the maximum possible value from index 'idx' onwards using fractional knapsack
// This serves as an upper bound for pruning branches that cannot improve the current best solution
double bound(size_t idx, int64 cw, int64 cv) {
    int64 rc = cap - cw;           // Remaining capacity
    double b = cv;                 // Start with current value
    size_t j = idx;

    // Greedily include full items in descending order of ratio
    while (j < n && items[j].w <= rc) {
        rc -= items[j].w;
        b += items[j].v;
        j++;
    }

    // If there's still capacity and items remaining, add fractional part of next item
    if (j < n) {
        b += rc * items[j].r;
    }

    return b;
}

// --- Recursive Branch and Bound DFS ---
// Explores the decision tree:
//   - idx: current item being considered
//   - cw: current total weight
//   - cv: current total value
void solve(size_t idx, int64 cw, int64 cv) {
    // --- Update Best Solution ---
    // If current value exceeds maximum found, record this as the new best solution
    if (cv > maxP) {
        maxP = cv;
        best = curr;  // Copy the current state (O(n) operation, but necessary)
    }

    // --- Base Case: No more items ---
    // If we've considered all items, stop recursion
    if (idx >= n) return;

    // --- Pruning: Bound Check ---
    // If the theoretical maximum (upper bound) from this point is not better than
    // the current best, prune this entire branch (no need to explore further)
    if (bound(idx, cw, cv) <= maxP) {
        return;
    }

    // --- Branch 1: INCLUDE Current Item ---
    // Try taking the current item if it fits in the knapsack
    if (cw + items[idx].w <= cap) {
        curr[items[idx].id] = 1;                                              // Mark item as taken (using original ID)
        solve(idx + 1, cw + items[idx].w, cv + items[idx].v);               // Recurse to next item
        curr[items[idx].id] = 0;                                              // Backtrack: unmark item
    }

    // --- Branch 2: EXCLUDE Current Item ---
    // Try not taking the current item (always possible, no weight constraint)
    // Note: The bound check above already ensures this branch is worth exploring
    solve(idx + 1, cw, cv);
}

// --- Memory Usage Helper ---
// Returns peak memory usage in bytes using system resource information
size_t getmem() {
    struct rusage u;
    getrusage(RUSAGE_SELF, &u);
    return static_cast<size_t>(u.ru_maxrss) * 1024;  // Convert KB (Linux) to bytes
}

// --- Main Function ---
int main(int argc, char *argv[]) {
    // --- Fast I/O Setup ---
    ios_base::sync_with_stdio(0);
    cin.tie(0);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    n = instance.n;
    cap = instance.capacity;
    const vector<int64> &w = instance.weights;
    const vector<int64> &v = instance.values;

    // --- Start Timer ---
    auto t0 = chrono::high_resolution_clock::now();

    // --- Setup Items ---
    // Create Item objects with original indices and compute ratios
    items.resize(n);
    for (size_t i = 0; i < n; i++) {
        // Handle zero-weight items: assign infinite ratio so they're processed first
        double r = w[i] == 0 ? std::numeric_limits<double>::infinity() : static_cast<double>(v[i]) / w[i];
        items[i] = { i, w[i], v[i], r };
    }

    // --- Sort by Ratio (Crucial for B&B Performance) ---
    // Sorting in descending order by ratio helps prune more branches early
    // because we encounter high-value items first, setting a high threshold
    sort(items.begin(), items.end(), cmpI);

    // --- Initialize Solution Buffers ---
    // Both are indexed by original item ID (0 to n-1), not sorted index
    curr.assign(n, 0);  // Current working solution (false = not taken)
    best.assign(n, 0);  // Best solution found (false = not taken)

    // --- Start Branch and Bound Search ---
    // Initialize: start from item 0, with 0 weight and 0 value
    size_t initial_idx = 0;      // Start considering from first item
    int64 initial_weight = 0;    // No weight used yet
    int64 initial_value = 0;     // No value gained yet
    solve(initial_idx, initial_weight, initial_value);

    // --- Stop Timer ---
    auto t1 = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::microseconds>(t1 - t0).count();

    // --- Reconstruct Solution ---
    // Collect original item IDs that were selected (best[i] == true)
    vector<size_t> res;
    for (size_t i = 0; i < n; i++) {
        if (best[i]) res.push_back(i);
    }

    // --- Output Formatting ---
    // Line 1: Maximum profit (as int64)
    cout << maxP << '\n';

    // Line 2: Number of selected items
    cout << res.size() << '\n';

    // Line 3: Item IDs (space-separated) or empty line if no items
    if (!res.empty()) {
        for (size_t i = 0; i < res.size(); i++) {
            cout << res[i] << (i == res.size() - 1 ? '\n' : ' ');
        }
    }
    else {
        cout << '\n';
    }

    // Line 4: Execution time (microseconds)
    cout << dur << '\n';

    // Line 5: Peak memory usage (bytes)
    cout << getmem() << '\n';

    return 0;
}
