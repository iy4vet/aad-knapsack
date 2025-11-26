// dual_knapsack.cpp
// Dual-descent 0/1 knapsack simulator compatible with simulations.py
// Outputs:
// 1) total_value (int64)
// 2) num_selected (int)
// 3) indices (0-based) space separated (may be empty line)
// 4) execution_time_microseconds (int64)
// 5) memory_bytes (int64)
//
// Compile:
//   g++ -O3 -std=c++17 -fopenmp -o dual_knapsack dual_knapsack.cpp
//
// Notes:
// - Reads from file: n capacity on line 1, optimal picks on line 2 (may be empty), then weight value pairs.
// - Uses multiplicative dual update with optional OpenMP parallelization.
// - Performs a postprocessing pass to ensure final solution is feasible.

#include <bits/stdc++.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "../file_io.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;
using int64 = long long;

static int64 get_peak_memory_bytes() {
    // getrusage.ru_maxrss: on Linux it's in kilobytes, on macOS it's bytes.
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
    long rss = usage.ru_maxrss;
#if defined(__APPLE__)
    // macOS: ru_maxrss is in bytes
    return (int64)rss;
#else
    // Linux: ru_maxrss is in kilobytes
    return (int64)rss * 1024LL;
#endif
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

    size_t n = instance.n;
    int64 W = instance.capacity;
    if (n == 0) {
        // print empty solution
        cout << 0 << "\n" << 0 << "\n\n" << 0 << "\n" << 0 << "\n";
        return 0;
    }

    const vector<int64> &weights = instance.weights;
    const vector<int64> &profits = instance.values;

    // Hyperparameters (tuneable inside code or by editing)
    // OPTIMIZATION: Increased learning rate for faster convergence on hard instances
    double alpha = 0.15;        // learning rate for multiplicative update (increased from 0.05)
    // OPTIMIZATION: Relaxed tolerance for better performance on large-scale problems
    double tol = 1e-3;          // relative tolerance for capacity (relaxed from 1e-4)
    // OPTIMIZATION: Increased max iterations for harder problem instances
    int max_iters = 5000;       // safety cap (increased from 1000)
    double lambda = 0.0;        // initial lambda; will set below heuristically

    // OPTIMIZATION: Better initial lambda using median profit/weight ratio
    // This works better for hard instances with similar ratios
    vector<double> ratios;
    ratios.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        if (weights[i] > 0) {
            ratios.push_back(static_cast<double>(profits[i]) / static_cast<double>(weights[i]));
        }
    }
    if (!ratios.empty()) {
        // Use median ratio instead of mean for better robustness
        sort(ratios.begin(), ratios.end());
        lambda = ratios[ratios.size() / 2];
    }
    else {
        lambda = 0.0;
    }

    vector<char> x(n, 0); // selected flags (0/1)

    // Timing start
    auto t0 = chrono::high_resolution_clock::now();

    int64 iter = 0;
    double rel_err = 1e9;
    int64 total_weight = 0;
    int64 total_value = 0;

    // Main dual-descent loop
    for (iter = 0; iter < max_iters; ++iter) {
        // compute provisional x_i at current lambda
        int64 local_weight = 0;
        int64 local_value = 0;

#ifdef _OPENMP
        // parallel local arrays for reduction
        int64 weight_acc = 0;
        int64 value_acc = 0;
#pragma omp parallel
        {
            int64 wsum = 0;
            int64 vsum = 0;
#pragma omp for schedule(static)
            for (size_t i = 0; i < n; ++i) {
                double thr = lambda * static_cast<double>(weights[i]);
                if (static_cast<double>(profits[i]) > thr) {
                    x[i] = 1;
                    wsum += weights[i];
                    vsum += profits[i];
                }
                else {
                    x[i] = 0;
                }
            }
#pragma omp atomic
            weight_acc += wsum;
#pragma omp atomic
            value_acc += vsum;
        }
        local_weight = weight_acc;
        local_value = value_acc;
#else
        local_weight = 0;
        local_value = 0;
        for (size_t i = 0; i < n; ++i) {
            double thr = lambda * static_cast<double>(weights[i]);
            if (static_cast<double>(profits[i]) > thr) {
                x[i] = 1;
                local_weight += weights[i];
                local_value += profits[i];
            }
            else {
                x[i] = 0;
            }
        }
#endif

        total_weight = local_weight;
        total_value = local_value;

        // Relative error on weight vs capacity
        double denom = (W == 0) ? 1.0 : static_cast<double>(W);
        rel_err = fabs(static_cast<double>(total_weight) - static_cast<double>(W)) / denom;
        if (rel_err <= tol) break;

        // Update lambda multiplicatively; ensure lambda >= 0
        double frac = (static_cast<double>(total_weight) - static_cast<double>(W)) / denom;
        double mult = 1.0 + alpha * frac;
        if (mult < 0.0) mult = 0.0; // safeguard
        lambda = lambda * mult;
        if (lambda < 0.0) lambda = 0.0;
    }

    // Post-processing: if overweight, drop selected items with smallest profit/weight ratio
    total_weight = 0;
    total_value = 0;
    vector<size_t> selected_indices;
    selected_indices.reserve(n);
    for (size_t i = 0; i < n; ++i) if (x[i]) {
        total_weight += weights[i];
        total_value += profits[i];
        selected_indices.push_back(i);
    }

    // OPTIMIZATION: Improved postprocessing for better solution quality on hard instances
    if (total_weight > W) {
        // Strategy: Remove items with smallest profit first (preserves high-value items)
        vector<tuple<int64, size_t, int64>> sel; // (profit, index, weight)
        sel.reserve(selected_indices.size());
        for (size_t idx : selected_indices) {
            sel.emplace_back(profits[idx], idx, weights[idx]);
        }
        // sort by profit ascending (remove smallest profit first)
        sort(sel.begin(), sel.end());

        // remove items until feasible
        for (auto &t : sel) {
            if (total_weight <= W) break;
            size_t idx = get<1>(t);
            total_weight -= get<2>(t);
            total_value -= get<0>(t);
            x[idx] = 0;
        }
    }

    // OPTIMIZATION: Greedy improvement pass - try adding back items that fit
    if (total_weight <= W) {
        vector<tuple<double, size_t, int64, int64>> available; // (ratio, index, weight, profit)
        for (size_t i = 0; i < n; ++i) {
            if (!x[i] && weights[i] > 0 && total_weight + weights[i] <= W) {
                double ratio = static_cast<double>(profits[i]) / static_cast<double>(weights[i]);
                available.emplace_back(ratio, i, weights[i], profits[i]);
            }
        }
        // sort by ratio descending (best items first)
        sort(available.begin(), available.end(), [](const auto &a, const auto &b) {
            return get<0>(a) > get<0>(b);
            });

        // add items that fit
        for (auto &t : available) {
            size_t idx = get<1>(t);
            int64 w = get<2>(t);
            int64 p = get<3>(t);
            if (total_weight + w <= W) {
                x[idx] = 1;
                total_weight += w;
                total_value += p;
            }
        }
    }

    // finalize selected list
    selected_indices.clear();
    for (size_t i = 0; i < n; ++i) if (x[i]) selected_indices.push_back(i);

    // Timing end
    auto t1 = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(t1 - t0).count();

    // Peak memory in bytes
    int64 peak_mem = get_peak_memory_bytes();

    // Output as expected by simulations.py
    cout << total_value << "\n";
    cout << (int)selected_indices.size() << "\n";
    if (!selected_indices.empty()) {
        for (size_t i = 0; i < selected_indices.size(); ++i) {
            if (i) cout << " ";
            cout << selected_indices[i];
        }
        cout << "\n";
    }
    else {
        cout << "\n";
    }
    cout << elapsed << "\n";
    cout << peak_mem << "\n";

    return 0;
}
