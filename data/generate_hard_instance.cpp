/**
 * Knapsack "Hard" Problem Instance Generator (C++ Worker)
 * Adapted from genhard.c by David Pisinger (2005)
 * http://www.diku.dk/~pisinger/genhard.c
 *
 * This generator produces instances that are statistically "hard" for
 * knapsack solvers, including strongly correlated instances and
 * spanner instances.
 *
 * COMPILE (from Python or manually):
 * g++ -O3 -std=c++17 -o generate_hard_instance generate_hard_instance.cpp
 *
 * USAGE (called by Python):
 * ./generate_hard_instance <filepath> <n> <r> <type> <seed>
 *
 * OUTPUT FILE FORMAT:
 * Line 1: <n> <capacity> <max_weight> <min_weight> (optimum unknown, left blank)
 * Line 2: (empty - optimal picks unknown)
 * Lines 3+: <weight_i> <value_i>
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>

using int64 = long long;

// =======================================================================
//                                Random
// =======================================================================
// Using Pisinger's portable generator to ensure faithful reproduction
// of the "hard" instances.

unsigned long _h48, _l48;

void srand48x(long s) {
    _h48 = s;
    _l48 = 0x330E;
}

long lrand48x(void) {
    _h48 = (_h48 * 0xDEECE66D) + (_l48 * 0x5DEEC);
    _l48 = _l48 * 0xE66D + 0xB;
    _h48 = _h48 + (_l48 >> 16);
    _l48 = _l48 & 0xFFFF;
    return (_h48 >> 1);
}

long randm(long x) {
    return lrand48x() % x;
}

// =======================================================================
//                                Generator
// =======================================================================

#define SPAN  10
#define SPAN2  5

int64 generate(int64 n, std::vector<int64>& p_vec, std::vector<int64>& w_vec, int type, int r, int seed) {
    int64 i;
    int64 p, w, r1, r2, k1, k2;
    int64 wsum, c;
    int64 sp[100], sw[100], span;

    // Initialize RNG with seed
    srand48x(seed);

    r1 = r / 10;
    r2 = r / 2;

    // Setup for spanner instances
    span = 0;
    if (type == 11) span = 2;
    if (type == 12) span = 2;
    if (type == 13) span = 2;

    for (i = 0; i < span; i++) {
        sw[i] = randm(r) + 1;
        if (type == 11) sp[i] = randm(r) + 1;                /* uncorr */
        if (type == 12) sp[i] = randm(2 * r1 + 1) + sw[i] - r1;      /* wekcorr */
        if (type == 13) sp[i] = sw[i] + r1;                  /* strcorr */
        if (sp[i] <= 0) sp[i] = 1;
        sw[i] = (sw[i] + SPAN2 - 1) / SPAN2;
        sp[i] = (sp[i] + SPAN2 - 1) / SPAN2;
    }

    for (i = 0; i < n; ) {
        w = randm(r) + 1;
        switch (type) {
        case  1: p = randm(r) + 1; /* uncorrelated */
            break;
        case  2: p = randm(2 * r1 + 1) + w - r1; /* weakly corr */
            if (p <= 0) p = 1;
            break;
        case  3: p = w + r1;   /* strongly corr */
            break;
        case  4: p = w; /* inverse strongly corr */
            w = p + r1;
            break;
        case  5: p = w + r1 + randm(2 * r / 1000 + 1) - r / 1000; /* alm str.corr */
            break;
        case  6: p = w; /* subset sum */
            break;
        case  7: w = 2 * ((w + 1) / 2); /* even-odd */
            p = w;
            break;
        case  8: w = 2 * ((w + 1) / 2); /* even-odd knapsack */
            p = w + r1;
            break;
        case  9: p = w; /* uncorrelated, similar weights */
            w = randm(r1) + 100 * r;
            break;

        case 11:
        case 12:
        case 13:
            k1 = randm(10) + 1;
            k2 = randm(span);
            w = k1 * sw[k2];
            p = k1 * sp[k2];
            break;
        case 14: w = randm(r) + 1; p = w; /* slightly difficult */
            if (w % 6 == 0) { p += 3 * r1; break; }
            p += 2 * r1;
            break;
        case 15: w = randm(r) + 1; p = 3 * ((w + 2) / 3);    /* even-odd like profits */
            break;
        case 16: w = randm(r) + 1; p = (int64)(2 * sqrt(4.0 * r * r - (w - 2.0 * r) * (w - 2.0 * r)) / 3.0);
            break;

        default:
             // Fallback to uncorrelated if unknown type
             p = randm(r) + 1;
             break;
        }

        p_vec[i] = p;
        w_vec[i] = w;
        i++;
    }

    wsum = 0;
    for (i = 0; i < n; i++) {
        wsum += w_vec[i];
    }

    // Capacity calculation
    // In genhard.c: c = (v * wsum) / (tests + 1)
    // We simulate this by using the seed to determine the "tightness"
    // Let's use the last 2 digits of seed as 'v' and 100 as 'tests'
    // This gives a capacity ratio between 0.0 and 1.0
    int v = seed % 100;
    int S = 100;
    // Avoid 0 capacity if possible, though genhard allows it.
    // Let's shift v to be 1..S to avoid 0 capacity.
    v = (v % S) + 1;

    c = (v * wsum) / (S + 1);

    // Ensure capacity is at least max weight to allow at least one item
    for (i = 0; i < n; i++) {
        if (w_vec[i] > c) c = w_vec[i];
    }

    // Adjust capacity based on type (from genhard.c)
    switch (type) {
        case  1: return c;
        case  2: return c;
        case  3: return c;
        case  4: return c;
        case  5: return c;
        case  6: return c;
        case  7: return 2 * (c / 2) + 1;
        case  8: return 2 * (c / 2) + 1;
        case  9: return c;

        case 11: return c;
        case 12: return c;
        case 13: return c;
        case 14: return c;
        case 15: return c;
        case 16: return c;

        default: return c;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Error: Expected 5 arguments: <filepath> <n> <r> <type> <seed>\n";
        return 1;
    }

    std::string filepath = argv[1];
    int64 n = std::stoll(argv[2]);
    int r = std::stoi(argv[3]);
    int type = std::stoi(argv[4]);
    int seed = std::stoi(argv[5]);

    std::vector<int64> weights(n);
    std::vector<int64> profits(n);

    int64 capacity = generate(n, profits, weights, type, r, seed);

    // Calculate max/min weights for header
    int64 maxWeight = 0;
    int64 minWeight = std::numeric_limits<int64>::max();
    for (int64 i = 0; i < n; ++i) {
        maxWeight = std::max(maxWeight, weights[i]);
        minWeight = std::min(minWeight, weights[i]);
    }

    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open output file: " << filepath << "\n";
        return 2;
    }

    // Line 1: n capacity max_weight min_weight (optimum unknown)
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
