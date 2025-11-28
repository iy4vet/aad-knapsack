#include <bits/stdc++.h>
#include "../file_io.hpp"
using namespace std;
using namespace std::chrono;

using int64 = long long;

struct Item {
    int64 weight;
    int64 value;
    size_t index;
};

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

    size_t n = instance.n;
    int64 W = instance.capacity;
    int64 wmax = instance.maxWeight;  // Use pre-computed max weight from instance
    const vector<int64> &weights = instance.weights;
    const vector<int64> &values = instance.values;

    // --- Start timer ---
    auto start = high_resolution_clock::now();

    vector<Item> items(n);
    for (size_t i = 0; i < n; ++i) {
        items[i] = { weights[i], values[i], i };
    }

    // --- Random permutation ---
    random_device rd;
    mt19937_64 rng(rd());
    shuffle(items.begin(), items.end(), rng);

    // --- DP computation ---
    const int64 NEG_INF = -1e15;
    vector<int64> prev(W + 1, NEG_INF), curr(W + 1, NEG_INF);
    prev[0] = 0;

    // Parent tracking for reconstruction
    vector<vector<int64>> parent(n + 1, vector<int64>(W + 1, -1));

    for (size_t i = 1; i <= n; ++i) {
        double mu = (1.0 * i / n) * W;
        double delta = sqrt(i * log(max(size_t(2), n))) * wmax;
        int64 low = max(int64(0), static_cast<int64>(floor(mu - delta)));
        int64 high = min(W, static_cast<int64>(ceil(mu + delta)));

        fill(curr.begin() + low, curr.begin() + high + 1, NEG_INF);

        for (int64 j = low; j <= high; ++j) {
            // Option 1: don't take
            curr[j] = prev[j];
            parent[i][j] = j;

            // Option 2: take item i-1
            int64 w = items[i - 1].weight;
            int64 v = items[i - 1].value;
            if (j >= w && prev[j - w] != NEG_INF) {
                int64 candidate = prev[j - w] + v;
                if (candidate > curr[j]) {
                    curr[j] = candidate;
                    parent[i][j] = j - w;
                }
            }
        }
        swap(prev, curr);
    }

    // --- Get best value and reconstruct solution ---
    int64 best_val = 0, best_w = 0;
    for (int64 j = 0; j <= W; ++j) {
        if (prev[j] > best_val) {
            best_val = prev[j];
            best_w = j;
        }
    }

    // Backtrack selected items
    vector<size_t> chosen;
    int64 j = best_w;
    for (size_t i = n; i >= 1; --i) {
        int64 pj = parent[i][j];
        if (pj == -1) continue;
        if (pj != j) { // item was taken
            chosen.push_back(items[i - 1].index);
        }
        j = pj;
    }

    reverse(chosen.begin(), chosen.end());

    // --- Stop timer ---
    auto end = high_resolution_clock::now();
    auto exec_time = duration_cast<microseconds>(end - start).count();

    // --- Rough memory usage estimation ---
    size_t mem_bytes = sizeof(int64) * (2 * static_cast<size_t>(W + 1) + (n + 1) * static_cast<size_t>(W + 1));

    // --- Output (strict format) ---
    cout << best_val << "\n";
    cout << chosen.size() << "\n";
    for (size_t i = 0; i < chosen.size(); ++i) {
        if (i) cout << " ";
        cout << chosen[i];
    }
    cout << "\n";
    cout << exec_time << "\n";
    cout << mem_bytes << "\n";

    return 0;
}
