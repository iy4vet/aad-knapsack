

#include <bits/stdc++.h>
#include <chrono>
#include "../file_io.hpp"
using namespace std;
using namespace std::chrono;
using int64 = long long;

struct SubsetL {
    int64 weight, value;
    int mask;
};
struct SubsetR {
    int64 weight, value;
};



int main(int argc, char *argv[]) {
    // Use fast I/O.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    size_t n = instance.n;
    int64 capacity = instance.capacity;
    const vector<int64> &weights = instance.weights;
    const vector<int64> &values = instance.values;

    // Hard limit for n1/n2 to avoid time/memory blowup
    size_t n1 = n / 2, n2 = n - n1;
    if (n1 > 24 || n2 > 24) {
        // Output error code for simulation harness
        cout << -1 << endl;
        cout << 0 << endl;
        cout << endl;
        cout << 0 << endl;
        cout << 0 << endl;
        return 69;
    }

    auto start = high_resolution_clock::now();

    vector<SubsetL> left;
    vector<SubsetR> right;

    // Generate all subsets for left half (store mask)
    for (int mask = 0; mask < (1 << n1); ++mask) {
        int64 w = 0, v = 0;
        for (size_t i = 0; i < n1; ++i) {
            if (mask & (1 << i)) {
                w += weights[i];
                v += values[i];
            }
        }
        left.push_back({ w, v, mask });
    }

    // Generate all subsets for right half (no mask)
    for (int mask = 0; mask < (1 << n2); ++mask) {
        int64 w = 0, v = 0;
        for (size_t i = 0; i < n2; ++i) {
            if (mask & (1 << i)) {
                w += weights[n1 + i];
                v += values[n1 + i];
            }
        }
        right.push_back({ w, v });
    }

    // Sort right by weight
    sort(right.begin(), right.end(), [](const SubsetR &a, const SubsetR &b) {
        return a.weight < b.weight;
        });

    // Filter dominated pairs in right
    vector<SubsetR> filtered;
    int64 maxv = LLONG_MIN;
    for (auto &s : right) {
        if (s.value > maxv) {
            filtered.push_back(s);
            maxv = s.value;
        }
    }
    right = filtered;

    // Meet in the middle
    int64 best_value = 0, best_right_weight = 0, best_right_value = 0;
    int best_left = 0;
    size_t best_right_mask = 0;
    for (auto &l : left) {
        if (l.weight > capacity) continue;
        int64 rem = capacity - l.weight;
        // Binary search for best right subset
        int lo = 0, hi = static_cast<int>(right.size()) - 1, idx = -1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (right[mid].weight <= rem) {
                idx = mid;
                lo = mid + 1;
            }
            else {
                hi = mid - 1;
            }
        }
        if (idx != -1) {
            int64 total_value = l.value + right[idx].value;
            if (total_value > best_value) {
                best_value = total_value;
                best_left = l.mask;
                best_right_weight = right[idx].weight;
                best_right_value = right[idx].value;
                best_right_mask = static_cast<size_t>(idx); // store index for reconstruction
            }
        }
    }

    // Reconstruct right half mask by brute force (since we only stored index)
    int n2_masks = 1 << n2;
    int right_mask = 0;
    for (int mask = 0; mask < n2_masks; ++mask) {
        int64 w = 0, v = 0;
        for (size_t i = 0; i < n2; ++i) {
            if (mask & (1 << i)) {
                w += weights[n1 + i];
                v += values[n1 + i];
            }
        }
        if (w == best_right_weight && v == best_right_value) {
            right_mask = mask;
            break;
        }
    }

    // Collect selected items
    vector<size_t> selected;
    for (size_t i = 0; i < n1; ++i) {
        if (best_left & (1 << i)) selected.push_back(i);
    }
    for (size_t i = 0; i < n2; ++i) {
        if (right_mask & (1 << i)) selected.push_back(n1 + i);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();

    // Estimate memory usage: vectors + subset structs
    size_t mem_used = sizeof(int64) * (weights.size() + values.size()) + selected.size() * sizeof(size_t);
    mem_used += left.size() * sizeof(SubsetL) + right.size() * sizeof(SubsetR);

    // Output as per required format
    cout << best_value << endl;
    cout << selected.size() << endl;
    for (size_t i = 0; i < selected.size(); ++i) {
        cout << selected[i];
        if (i + 1 < selected.size()) cout << " ";
    }
    cout << endl;
    cout << duration << endl;
    cout << mem_used << endl;
}
