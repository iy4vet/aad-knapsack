// Implementation of Algorithm A for the 0-1 Knapsack Problem
// Robert M. Nauss, Management Science Vol. 23, No. 1, September 1976
// "An Efficient Algorithm for the 0-1 Knapsack Problem"

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include "../file_io.hpp"

using namespace std;
using int64 = long long;

struct Item {
    int64 id;           // Original index
    int64 c;            // Value
    int64 w;            // Weight
    double ratio;       // c/w ratio

    Item(int64 i, int64 val, int64 weight) : id(i), c(val), w(weight) {
        ratio = (w > 0) ? (double)c / w : 0.0;
    }
};

bool compareByRatio(const Item &a, const Item &b) {
    return a.ratio > b.ratio;
}

class KnapsackSolver {
private:
    int64 n;
    int64 B;
    vector<Item> items;

    // Best solution
    int64 z_star;
    vector<int64> x_star;

    // Pegged variables
    vector<int64> I_1;  // Pegged to 1
    vector<int64> I_0;  // Pegged to 0

    // Step 2: Solve LP relaxation
    double solve_LP(vector<double> &x_bar, int64 &r) {
        fill(x_bar.begin(), x_bar.end(), 0.0);
        int64 used = 0;
        double value = 0.0;
        r = -1;

        for (int64 i = 0; i < n; i++) {
            if (used + items[i].w <= B) {
                x_bar[i] = 1.0;
                value += items[i].c;
                used += items[i].w;
            }
            else {
                // Fractional variable
                r = i;
                int64 remaining = B - used;
                x_bar[i] = (double)remaining / items[i].w;
                value += x_bar[i] * items[i].c;
                break;
            }
        }
        return value;
    }

    // Step 3: Find lower bound using heuristics
    void find_lower_bound(const vector<double> &x_bar, int64 r) {
        // Initialize with LP solution rounded down
        z_star = 0;
        x_star.assign(n, 0);
        int64 used = 0;

        for (int64 i = 0; i < n; i++) {
            if (i != r && x_bar[i] > 0.5) {
                x_star[i] = 1;
                z_star += items[i].c;
                used += items[i].w;
            }
        }

        // Heuristic 1: Set xr=0, add items after r
        vector<int64> x_h1 = x_star;
        int64 z_h1 = z_star;
        int64 slack = B - used;

        for (int64 i = r + 1; i < n; i++) {
            if (items[i].w <= slack) {
                x_h1[i] = 1;
                z_h1 += items[i].c;
                slack -= items[i].w;
            }
        }

        if (z_h1 > z_star) {
            z_star = z_h1;
            x_star = x_h1;
        }

        // Heuristic 2: Set xr=1, remove items before r to make feasible
        vector<int64> x_h2(n, 0);
        int64 z_h2 = 0;
        int64 weight = 0;

        // Start with all variables up to and including r set to 1
        for (int64 i = 0; i <= r; i++) {
            x_h2[i] = 1;
            z_h2 += items[i].c;
            weight += items[i].w;
        }

        // Remove items from the end (before r) until feasible
        int64 overfill = weight - B;
        for (int64 i = r - 1; i >= 0 && overfill > 0; i--) {
            if (x_h2[i] == 1) {
                x_h2[i] = 0;
                z_h2 -= items[i].c;
                overfill -= items[i].w;
            }
        }

        // Now add items after r if they fit
        slack = -overfill;
        for (int64 i = r + 1; i < n; i++) {
            if (items[i].w <= slack) {
                x_h2[i] = 1;
                z_h2 += items[i].c;
                slack -= items[i].w;
            }
        }

        if (z_h2 > z_star) {
            z_star = z_h2;
            x_star = x_h2;
        }
    }

    // Steps 4-5: Pegging tests using Lagrangean relaxation
    void peg_variables(int64 r, double v_lp) {
        if (r == -1) return;

        double lambda = items[r].ratio;  // cr/wr

        // Step 4: Try to peg variables to 1
        for (int64 i = 0; i < r; i++) {
            double v_lgr = v_lp - items[i].c + lambda * items[i].w;
            if (v_lgr <= z_star) {
                I_1.push_back(i);
            }
        }

        // Step 5: Try to peg variables to 0
        for (int64 i = r + 1; i < n; i++) {
            double v_lgr = v_lp + items[i].c - lambda * items[i].w;
            if (v_lgr <= z_star) {
                I_0.push_back(i);
            }
        }
    }

    // Steps 7-17: Branch and bound
    void branch_and_bound() {
        // Build list of free variables
        vector<bool> is_pegged(n, false);
        for (int64 i : I_1) is_pegged[i] = true;
        for (int64 i : I_0) is_pegged[i] = true;

        vector<int64> free_vars;
        for (int64 i = 0; i < n; i++) {
            if (!is_pegged[i]) {
                free_vars.push_back(i);
            }
        }

        // Initialize candidate list with root problem
        vector<int64> current_x(n, 0);
        for (int64 i : I_1) current_x[i] = 1;

        int64 base_value = 0;
        int64 base_weight = 0;
        for (int64 i : I_1) {
            base_value += items[i].c;
            base_weight += items[i].w;
        }

        int64 remaining_capacity = B - base_weight;

        // Start recursive branch and bound
        bnb_recursive(free_vars, 0, current_x, base_value, remaining_capacity);
    }

    void bnb_recursive(const vector<int64> &free_vars, int64 idx,
        vector<int64> &current_x, int64 current_value,
        int64 remaining_cap) {
        // Step 13: Check if current solution is feasible and better
        if (current_value > z_star) {
            z_star = current_value;
            x_star = current_x;
        }

        // Step 11: Base case - no more free variables
        if (idx >= (int64)free_vars.size()) {
            return;
        }

        // Step 10-12: Calculate upper bound and fathom if necessary
        double upper_bound = current_value;
        int64 temp_cap = remaining_cap;

        for (int64 i = idx; i < (int64)free_vars.size(); i++) {
            int64 var_idx = free_vars[i];
            if (items[var_idx].w <= temp_cap) {
                upper_bound += items[var_idx].c;
                temp_cap -= items[var_idx].w;
            }
            else {
                upper_bound += (double)temp_cap / items[var_idx].w * items[var_idx].c;
                break;
            }
        }

        if (upper_bound <= z_star) {
            return;  // Fathom
        }

        // Step 14-16: Branch on next variable
        int64 j = free_vars[idx];

        // Step 15: Try setting xj = 1
        if (items[j].w <= remaining_cap) {
            current_x[j] = 1;
            bnb_recursive(free_vars, idx + 1, current_x,
                current_value + items[j].c,
                remaining_cap - items[j].w);
            current_x[j] = 0;
        }

        // Step 16: Try setting xj = 0 (if upper bound is promising)
        double ub_without = current_value;
        int64 temp_cap2 = remaining_cap;

        for (int64 i = idx + 1; i < (int64)free_vars.size(); i++) {
            int64 var_idx = free_vars[i];
            if (items[var_idx].w <= temp_cap2) {
                ub_without += items[var_idx].c;
                temp_cap2 -= items[var_idx].w;
            }
            else {
                ub_without += (double)temp_cap2 / items[var_idx].w * items[var_idx].c;
                break;
            }
        }

        if (ub_without > z_star) {
            bnb_recursive(free_vars, idx + 1, current_x,
                current_value, remaining_cap);
        }
    }

public:
    pair<int64, vector<int64>> solve(const vector<int64> &values,
        const vector<int64> &weights,
        size_t items_n, int64 capacity) {
        n = items_n;
        B = capacity;

        // Build items and sort by ratio
        items.clear();
        for (int64 i = 0; i < n; i++) {
            items.emplace_back(i, values[i], weights[i]);
        }

        // Step 1: Sort by decreasing bang-for-buck
        sort(items.begin(), items.end(), compareByRatio);

        // Step 2: Solve LP relaxation
        vector<double> x_bar(n);
        int64 r;
        double v_lp = solve_LP(x_bar, r);

        // If LP solution is integer, we're done
        if (r == -1) {
            z_star = (int64)v_lp;
            x_star.assign(n, 0);
            for (int64 i = 0; i < n; i++) {
                if (x_bar[i] > 0.5) {
                    x_star[i] = 1;
                }
            }

            vector<int64> selected;
            for (int64 i = 0; i < n; i++) {
                if (x_star[i] == 1) {
                    selected.push_back(items[i].id);
                }
            }
            return { z_star, selected };
        }

        // Step 3: Find lower bound with heuristics
        find_lower_bound(x_bar, r);

        // Steps 4-5: Peg variables
        I_1.clear();
        I_0.clear();
        peg_variables(r, v_lp);

        // Step 6-17: Solve reduced problem with branch and bound
        branch_and_bound();

        // Extract solution
        vector<int64> selected;
        for (int64 i = 0; i < n; i++) {
            if (x_star[i] == 1) {
                selected.push_back(items[i].id);
            }
        }

        return { z_star, selected };
    }
};

int main(int argc, char *argv[]) {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    KnapsackInstance instance;
    if (!loadKnapsackInstance(argv[1], instance)) {
        return 1;
    }

    int64 n = static_cast<int64>(instance.n);
    int64 capacity = instance.capacity;
    const vector<int64> &weights = instance.weights;
    const vector<int64> &values = instance.values;

    auto start = chrono::high_resolution_clock::now();

    KnapsackSolver solver;
    auto [max_value, selected] = solver.solve(values, weights, n, capacity);

    auto end = chrono::high_resolution_clock::now();
    int64 duration = chrono::duration_cast<chrono::microseconds>(end - start).count();

    int64 memory_used = (sizeof(int64) * (weights.size() + values.size())) +
        (sizeof(int64) * selected.size());

    // Output
    cout << max_value << "\n";
    cout << selected.size() << "\n";
    for (int64 idx : selected) {
        cout << idx << " ";
    }
    if (!selected.empty()) {
        cout << "\n";
    }
    cout << duration << "\n";
    cout << memory_used << "\n";

    return 0;
}
