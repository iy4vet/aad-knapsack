/* ======================================================================
             combo.cpp
             Modernized C++ version of the COMBO algorithm
             Original Authors: S.Martello, D.Pisinger, P.Toth (1997)
   ====================================================================== */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <chrono>
#include "../file_io.hpp"

   // Modernized Type Definitions
using int64 = long long;
using boolean = bool;
using ntype = size_t;
using itype = int64;
using stype = int64;
using btype = uint64_t;
using prod = double;

// Constants
constexpr int MINRUDI = 1000;    /* parameter M1 from paper */
constexpr int MINSET = 2000;     /* parameter M2 from paper */
constexpr int MINHEUR = 10000;   /* parameter M3 from paper */
constexpr int MAXSTATES = 1500000;

// Replaced undef/define logic with constexpr check
constexpr bool HASCHANCE = false;

constexpr int SYNC = 5;
constexpr int SORTSTACK = 200;
constexpr int MINMED = 1000;
constexpr int MAXV = 64; // Fixed to 64 bits for uint64_t

constexpr int LEFT = 1;
constexpr int RIGHT = 2;

constexpr int PARTITION = 1;
constexpr int SORTALL = 2;

// Debug counters (global as per original design, but static to file)
static int64 simpreduced = 0;
static int64 iterates = 0;
static int64 maxstates = 0;
static int64 coresize = 0;
static int64 optsur = 0;
static int64 relaxations = 0;
static int64 relfeasible = 0;
static int64 reltime = 0;
static int64 pitested = 0;
static int64 pireduced = 0;
static int64 dynheur = 0;

// Structures
struct Item {
    itype p; // profit
    itype w; // weight
    boolean x; // solution variable
    int id; // original index
};

struct Interval {
    Item *f;
    Item *l;
};

struct State {
    stype psum;
    stype wsum;
    btype vect;
};

struct PartSet {
    ntype size;
    State *fset; // Points inside set1 vector
    State *lset; // Points inside set1 vector

    // Instead of raw malloc, we will use a std::vector to hold the data,
    // but the algorithm relies heavily on pointer arithmetic.
    // We will allocate a large vector and let pointers traverse it.
    std::vector<State> storage;
    State *set1; // Start of storage
    State *setm; // End of storage

    btype vno;
    Item *vitem[MAXV];
    Item *ovitem[MAXV];
    btype ovect;

    PartSet() : size(0), fset(nullptr), lset(nullptr), set1(nullptr), setm(nullptr), vno(0), ovect(0) {
        for (int i = 0; i < MAXV; ++i) { vitem[i] = nullptr; ovitem[i] = nullptr; }
    }
};

struct AllInfo {
    Item *fitem;
    Item *litem;
    Item *s;
    Item *t;
    Item *b;
    Item *fpart;
    Item *lpart;
    stype wfpart;
    Item *fsort;
    Item *lsort;
    stype wfsort;
    stype c;
    stype z;
    stype zwsum;
    stype lb;

    boolean fullsol;
    Item *fsol;
    Item *lsol;

    // Full solution storage
    std::vector<Item> full_solution_storage;
    Item *ffull;
    Item *lfull;

    PartSet d;

    stype dantzig;
    stype ub;
    stype psumb;
    stype wsumb;

    stype ps, ws, pt, wt;

    // Interval stack storage - Split into two vectors for safety
    std::vector<Interval> stack_left;
    std::vector<Interval> stack_right;

    boolean relx;
    boolean master;
    int coresize_debug; // Renamed to avoid shadowing global

    AllInfo() : fitem(nullptr), litem(nullptr), s(nullptr), t(nullptr), b(nullptr),
        fpart(nullptr), lpart(nullptr), wfpart(0), fsort(nullptr), lsort(nullptr),
        wfsort(0), c(0), z(0), zwsum(0), lb(0), fullsol(false), fsol(nullptr),
        lsol(nullptr), ffull(nullptr), lfull(nullptr), dantzig(0), ub(0),
        psumb(0), wsumb(0), ps(0), ws(0), pt(0), wt(0),
        relx(false), master(false), coresize_debug(0) {
    }
};

// Inline helper functions
inline double DET(int64 a1, int64 a2, int64 b1, int64 b2) {
    return (static_cast<double>(a1) * static_cast<double>(b2) - static_cast<double>(a2) * static_cast<double>(b1));
}

inline int NO(Item *a, Item *p) {
    return static_cast<int>(p - a); // Not directly used in C logic logic mostly, but preserved logic
}

inline int DIFF(void *a, void *b) {
    // Returns number of elements between pointers
    return static_cast<int>((static_cast<State *>(b) + 1) - static_cast<State *>(a));
}
inline int DIFF_ITEM(Item *a, Item *b) {
    return static_cast<int>(b - a + 1);
}

// Error handling
static void error(const char *str) {
    std::cerr << str << std::endl;
    std::cerr << "PROGRAM TERMINATED !!!" << std::endl << std::endl;
    exit(-1);
}

// Forward Declarations
stype combo(Item *f, Item *l, stype c, stype lb, stype ub, boolean def, boolean relx);

// ======================================================================
// Stack Operations
// ======================================================================

static void push(AllInfo *a, int side, Item *f, Item *l) {
    Interval interval = { f, l };
    if (side == LEFT) {
        a->stack_left.push_back(interval);
    }
    else {
        a->stack_right.push_back(interval);
    }
}

static void pop(AllInfo *a, int side, Item **f, Item **l) {
    Interval interval;
    if (side == LEFT) {
        if (a->stack_left.empty()) error("pop left");
        interval = a->stack_left.back();
        a->stack_left.pop_back();
    }
    else {
        if (a->stack_right.empty()) error("pop right");
        interval = a->stack_right.back();
        a->stack_right.pop_back();
    }
    *f = interval.f;
    *l = interval.l;
}

// ======================================================================
// Logic
// ======================================================================

static void improvesol(AllInfo *a, State *v) {
    if (v->wsum > a->c) error("wrong improvesol");
    if (v->psum <= a->z) error("not improved solution");

    a->z = v->psum;
    a->zwsum = v->wsum;
    a->fsol = a->s;
    a->lsol = a->t;
    a->d.ovect = v->vect;
    a->fullsol = false;

    // Copy the pointer array
    std::copy(std::begin(a->d.vitem), std::end(a->d.vitem), std::begin(a->d.ovitem));
}

static void definesolution(AllInfo *a) {
    Item *i, *h, *m;
    stype psum, wsum, ws;
    Item *f, *l;
    btype j, k;

    /* full solutions are easy to update */
    if (a->fullsol) {
        for (i = a->fitem, m = a->litem + 1, h = a->ffull; i != m; i++, h++) *i = *h;
        return;
    }

    /* initialize break solution */
    for (i = a->fitem, m = a->b; i != m; i++) i->x = true;
    for (i = a->b, m = a->litem + 1; i != m; i++) i->x = false;

    /* backtrack vector */
    psum = a->z - a->psumb;
    wsum = a->zwsum - a->wsumb;
    f = a->fsol;
    l = a->lsol;

    /* backtrack */
    for (j = 0; j < MAXV; j++) {
        i = a->d.ovitem[j];
        if (i == nullptr) continue;
        k = a->d.ovect & (static_cast<btype>(1) << j);
        if (i->x) {
            if (i > f) f = i;
            if (k) { psum += i->p; wsum += i->w; i->x = false; }
        }
        else {
            if (i < l) l = i;
            if (k) { psum -= i->p; wsum -= i->w; i->x = true; }
        }
    }
    if ((psum == 0) && (wsum == 0)) return;

    f++; l--; /* new core */
    psum = a->z; wsum = a->zwsum; iterates++;
    if (f > l) error("wrong backtrack");

    for (i = a->fitem, m = f; i != m; i++) if (i->x) { psum -= i->p; wsum -= i->w; }
    for (i = l + 1, m = a->litem + 1; i != m; i++) if (i->x) { psum -= i->p; wsum -= i->w; }
    for (i = f, m = l + 1, ws = 0; i != m; i++) ws += i->w;

    if (ws == wsum) {
        for (i = f, m = l + 1; i != m; i++) i->x = true;
    }
    else {
        combo(f, l, wsum, psum - 1, psum, true, true);
    }
}

static void rudidiv(AllInfo *a) {
    Item *i, *m, *b;
    itype x, y, r;
    prod pb, wb, q;
    stype ws;

    b = a->b; pb = b->p; wb = b->w;
    q = DET(a->z + 1 - a->psumb, a->c - a->wsumb, static_cast<int64>(pb), static_cast<int64>(wb));
    x = a->fitem->w; ws = 0;

    for (i = a->fitem, m = a->litem + 1; i != m; i++) {
        if ((i < b) && (DET(-i->p, -i->w, static_cast<int64>(pb), static_cast<int64>(wb)) < q)) { ws += i->w; continue; }
        if ((i > b) && (DET(i->p, i->w, static_cast<int64>(pb), static_cast<int64>(wb)) < q)) { continue; }
        y = x; x = i->w;
        while (y != 0) { r = x % y; x = y; y = r; }
        if (x == 1) return;
    }
    a->c = ws + x * ((a->c - ws) / x);
}

static void partsort(AllInfo *a, Item *f, Item *l, stype ws, stype c, int what) {
    // Modernized: Use std::sort
    // Note: The original algorithm used a partial sort/partitioning scheme to populate the stacks.
    // By sorting the entire range, we simplify the logic. The stacks will be populated
    // by findcore() which pushes the intervals [f, b-1] and [b+1, l].
    // Since the array is fully sorted, these intervals are also sorted, which is fine (actually better).

    std::sort(f, l + 1, [](const Item &a, const Item &b) {
        return DET(a.p, a.w, b.p, b.w) > 0;
        });

    if (what == PARTITION) {
        a->fpart = f;
        a->lpart = l;
        a->wfpart = ws; // Not strictly needed with full sort but kept for structure
    }
}

static Item *minweights(Item *f, Item *l, stype c) {
    // Find items with smallest weights that fit in c
    std::sort(f, l + 1, [](const Item &a, const Item &b) {
        return a.w < b.w;
        });

    while (f <= l && f->w <= c) {
        c -= f->w;
        f++;
    }
    return f;
}

static Item *maxprofits(Item *f, Item *l, stype z) {
    // Find items with largest profits that fit in z
    std::sort(f, l + 1, [](const Item &a, const Item &b) {
        return a.p > b.p;
        });

    while (f <= l && f->p <= z) {
        z -= f->p;
        f++;
    }
    return f;
}

static void sursort(Item *f, Item *l, itype sur, stype c, stype *p1, stype *w1, Item **b) {
    // Sort by surrogate relaxed ratio
    std::sort(f, l + 1, [sur](const Item &a, const Item &b) {
        return DET(a.p, a.w + sur, b.p, b.w + sur) > 0;
        });

    stype psum = 0;
    // c is the capacity

    for (; f <= l; f++) {
        if (f->w + sur > c) {
            *p1 = psum;
            *w1 = c;
            *b = f;
            return;
        }
        c -= f->w + sur;
        psum += f->p;
    }

    static Item nn;
    nn.p = 0; nn.w = 1;
    *p1 = psum;
    *w1 = c;
    *b = &nn;
}

static boolean haschance(AllInfo *a, Item *i, int side) {
    itype p, w;
    State *j, *m;
    stype pp, ww;

    if (a->d.size == 0) return false;

    if (HASCHANCE) {
        if (side == RIGHT) {
            if (a->d.fset->wsum <= a->c - i->w) return true;
            p = a->ps; w = a->ws; pitested++;
            pp = i->p - a->z - 1; ww = i->w - a->c;
            for (j = a->d.fset, m = a->d.lset + 1; j != m; j++) {
                if (DET(j->psum + pp, j->wsum + ww, p, w) >= 0) return true;
            }
        }
        else {
            if (a->d.lset->wsum > a->c + i->w) return true;
            p = a->pt; w = a->wt; pitested++;
            pp = -i->p - a->z - 1; ww = -i->w - a->c;
            for (j = a->d.lset, m = a->d.fset - 1; j != m; j--) {
                if (DET(j->psum + pp, j->wsum + ww, p, w) >= 0) return true;
            }
        }
        pireduced++;
        return false;
    }
    else {
        p = a->b->p; w = a->b->w;
        if (side == LEFT) {
            return (DET(a->psumb - i->p - a->z - 1, a->wsumb - i->w - a->c, p, w) >= 0);
        }
        else {
            return (DET(a->psumb + i->p - a->z - 1, a->wsumb + i->w - a->c, p, w) >= 0);
        }
    }
}

static void moveset(AllInfo *a) {
    State *i, *j, *m;
    PartSet *d;

    /* move array to end if necessary */
    d = &a->d;
    if (d->lset != d->setm - 1) {
        for (i = d->setm, j = d->lset, m = d->fset - 1; j != m; j--) {
            i--; *i = *j;
        }
        d->fset = i; d->lset = d->setm - 1;
    }
}

static void multiply(AllInfo *a, Item *h, int side) {
    State *i, *j, *k, *m;
    itype p, w;
    btype mask0, mask1;
    State *r1, *rm;
    PartSet *d;

    d = &a->d; if (d->size == 0) return;
    if (side == RIGHT) { p = h->p; w = h->w; }
    else { p = -h->p; w = -h->w; }

    /* keep track on solution vector */
    d->vno++; if (d->vno == MAXV) d->vno = 0;
    mask1 = (static_cast<btype>(1) << d->vno); mask0 = ~mask1;
    d->vitem[d->vno] = h;

    /* initialize limits */
    r1 = d->fset; rm = d->lset; k = d->set1; m = rm + 1;
    k->psum = -1;
    k->wsum = r1->wsum + h->w + 1;
    m->wsum = rm->wsum + h->w + 1;

    for (i = r1, j = r1; (i != m) || (j != m); ) {
        if (i->wsum <= j->wsum + w) {
            if (i->psum > k->psum) {
                if (i->wsum > k->wsum) {
                    k++;
                    if ((k == i) || (k == j)) break;
                }
                k->psum = i->psum; k->wsum = i->wsum;
                k->vect = i->vect & mask0;
            }
            i++;
        }
        else {
            if (j->psum + p > k->psum) {
                if (j->wsum + w > k->wsum) {
                    k++;
                    if ((k == i) || (k == j)) break;
                }
                k->psum = j->psum + p; k->wsum = j->wsum + w;
                k->vect = j->vect | mask1;
            }
            j++;
        }
    }
    if ((k == i) || (k == j)) error("multiply, no space");

    d->fset = d->set1;
    d->lset = k;
    d->size = DIFF(d->fset, d->lset);

    if (d->size > maxstates) maxstates = d->size;
    a->coresize_debug++;
    if (a->master) coresize++;
}

static void surbin(Item *f, Item *l, itype s1, itype s2, stype c,
    stype dantzig, ntype card, stype *sur, stype *u) {
    Item *b;
    stype csur, r, psum, s, d, suropt;
    double ua, ub, gr, e, uopt;

    /* iterate surr. multiplier */
    uopt = static_cast<double>(dantzig); suropt = 0;
    for (; s1 <= s2; ) {
        s = (s2 + s1) / 2;
        csur = c + s * static_cast<long>(card);
        if (csur < 0) csur = 0;
        sursort(f, l, s, csur, &psum, &r, &b);

        /* derive bound and gradient */
        e = 1; d = (b - f);
        ua = static_cast<double>(psum) + static_cast<double>(r) * static_cast<double>(b->p) / static_cast<double>(b->w + s);
        ub = static_cast<double>(psum) + (static_cast<double>(r) + (static_cast<double>(card) - static_cast<double>(d)) * e) * static_cast<double>(b->p) / static_cast<double>(b->w + s + e);
        gr = (ub - ua) / e;

        if (ua < uopt) { suropt = s; uopt = ua; }
        if (gr > 0) s2 = s - 1; else s1 = s + 1;
    }
    *sur = suropt; *u = static_cast<stype>(uopt);
}

static void solvesur(AllInfo *a, Item *f, Item *l, stype minsur, stype maxsur,
    ntype card, stype *ub) {
    Item *i, *k, *m;
    stype ps, csur;
    ntype no;
    stype sur, u, z1;
    boolean feasible;

    /* find optimal surrogate multiplier, and update ub */
    surbin(f, l, minsur, maxsur, a->c, a->dantzig, card, &sur, &u);
    optsur = sur;

    /* if bound <= current solution return */
    if ((u <= a->z) || (sur == 0)) {
        if (u > *ub) *ub = u;
        return;
    }

    /* add sur to weights and remove items with negative weight */
    csur = a->c + sur * static_cast<long>(card); ps = 0;
    for (i = f, k = f, m = l + 1; i != m; i++) {
        i->w += sur; i->x = false;
        if (i->w <= 0) { ps += i->p; csur -= i->w; i->x = true; std::swap(*i, *k); k++; }
    }

    /* solve problem to optimality */
    z1 = ps + combo(k, l, csur, a->z - ps, 0, true, true);

    /* subtract weight and check cardinality */
    for (i = f, m = l + 1, no = 0; i != m; i++) { i->w -= sur; if (i->x) no++; }

    /* if feasible cardinality and improved, save solution in extra array */
    feasible = (no == card); if (feasible) relfeasible++;
    if ((z1 > a->z) && (feasible)) {
        for (i = f, k = a->ffull, m = l + 1; i != m; i++, k++) *k = *i;
        a->z = z1; a->fullsol = true;
    }

    /* output: maintain global upper bound */
    if (z1 > *ub) *ub = z1;
}

static void surrelax(AllInfo *a) {
    Item *i, *j, *m;
    Item *f, *l, *b;
    ntype n, card1, card2, b1;
    stype u, minsur, maxsur, wsum;
    itype minw, maxp, maxw;
    int64 t1, t2;

    /* copy table */
    relaxations++;
    n = DIFF_ITEM(a->fitem, a->litem);

    // We need a temporary vector for the relaxed items
    std::vector<Item> temp_items(n);
    f = temp_items.data();
    l = f + n - 1;

    minw = a->fitem->w; maxp = maxw = wsum = 0;
    for (j = f, i = a->fitem, m = l + 1; j != m; i++, j++) {
        *j = *i; wsum += i->w;
        if (i->w < minw) minw = i->w;
        if (i->w > maxw) maxw = i->w;
        if (i->p > maxp) maxp = i->p;
    }

    /* find cardinality */
    b = a->b; b1 = DIFF_ITEM(a->fitem, b - 1);
    card1 = minweights(f, l, a->c) - f;      /* sum_{j=1}^{n} x_{j} \leq card1 */
    card2 = maxprofits(f, l, a->z) - f + 1;    /* sum_{j=1}^{n} x_{j} \geq card2 */

    /* delimiters on sur.multipliers */
    maxsur = maxw;
    minsur = -maxw;

    /* choose strategy */
    u = 0;
    for (;;) {
        if (card2 == b1 + 1) {
            solvesur(a, f, l, minsur, 0, b1 + 1, &u); /* min card constr */
            if (u < a->z) u = a->z; /* since bound for IMPROVED solution */
            break;
        }
        if (card1 == b1) {
            solvesur(a, f, l, 0, maxsur, b1, &u); /* max card constr */
            break;
        }
        if (card1 == b1 + 1) { /* dichothomy: card <= b1 or card >= b1+1 */
            solvesur(a, f, l, minsur, 0, b1 + 1, &u);
            solvesur(a, f, l, 0, maxsur, b1, &u);
            break;
        }
        if (card2 == b1) { /* dichothomy: card <= b1 or card >= b1+1 */
            solvesur(a, f, l, 0, maxsur, b1, &u);
            solvesur(a, f, l, minsur, 0, b1 + 1, &u);
            break;
        }
        u = a->dantzig; break;
    }
    if (u < a->ub) a->ub = u;
    // temp_items is freed automatically by std::vector
}

static void simpreduce(int side, Item **f, Item **l, AllInfo *a) {
    Item *i, *j, *k;
    prod pb, wb;
    prod q;
    int red;

    if (a->d.size == 0) { *f = *l + 1; return; }
    if (*l < *f) return;

    pb = a->b->p; wb = a->b->w;
    q = DET(a->z + 1 - a->psumb, a->c - a->wsumb, static_cast<int64>(pb), static_cast<int64>(wb));
    i = *f; j = *l; red = 0;
    if (side == LEFT) {
        k = a->fsort - 1;
        while (i != j + 1) {
            if (DET(-j->p, -j->w, static_cast<int64>(pb), static_cast<int64>(wb)) < q) {
                std::swap(*i, *j); i++;       /* not feasible */
                red++;
            }
            else {
                std::swap(*j, *k); j--; k--;  /* feasible */
            }
        }
        *l = a->fsort - 1; *f = k + 1;
    }
    else {
        k = a->lsort + 1;
        while (i != j + 1) {
            if (DET(i->p, i->w, static_cast<int64>(pb), static_cast<int64>(wb)) < q) {
                std::swap(*i, *j); j--;       /* not feasible */
                red++;
            }
            else {
                std::swap(*i, *k); i++; k++;  /* feasible */
            }
        }
        *f = a->lsort + 1; *l = k - 1;
    }
    if (a->master) simpreduced += red;
}

static State *findvect(stype ws, State *f, State *l) {
    State *m;

    if (f->wsum > ws) return nullptr;
    if (l->wsum <= ws) return l;
    while (l - f > SYNC) {
        m = f + (l - f) / 2;
        if (m->wsum > ws) l = m - 1; else f = m;
    }
    while (l->wsum > ws) l--;
    return l;
}

static void expandcore(AllInfo *a, boolean *atstart, boolean *atend) {
    Item *f, *l;

    /* expand core */
    *atstart = false;
    if (a->s < a->fsort) {
        if (a->stack_left.empty()) {
            *atstart = true;
        }
        else {
            pop(a, LEFT, &f, &l); a->ps = f->p; a->ws = f->w;
            simpreduce(LEFT, &f, &l, a);
            if (f != l + 1) {
                partsort(a, f, l, 0, 0, SORTALL); a->fsort = f;
                a->ps = a->s->p; a->ws = a->s->w;
            }
        }
    }
    else { a->ps = a->s->p; a->ws = a->s->w; }

    /* expand core */
    *atend = false;
    if (a->t > a->lsort) {
        if (a->stack_right.empty()) {
            *atend = true;
        }
        else {
            pop(a, RIGHT, &f, &l); a->pt = l->p; a->wt = l->w;
            simpreduce(RIGHT, &f, &l, a);
            if (f != l + 1) {
                partsort(a, f, l, 0, 0, SORTALL); a->lsort = l;
                a->pt = a->t->p; a->wt = a->t->w;
            }
        }
    }
    else { a->pt = a->t->p; a->wt = a->t->w; }
}

static void reduceset(AllInfo *a) {
    State *i, *m, *k;
    stype c, z;
    prod p, w;
    State *v, *r1, *rm;
    boolean atstart, atend;

    if (a->d.size == 0) return;

    /* find break point and improve solution */
    r1 = a->d.fset; rm = a->d.lset;
    v = findvect(a->c, r1, rm);
    if (v == nullptr) v = r1 - 1; else if (v->psum > a->z) improvesol(a, v);

    /* expand core, and choose ps, ws, pt, wt */
    expandcore(a, &atstart, &atend);

    /* now do the reduction */
    c = a->c; z = a->z + 1; k = a->d.setm;
    if (!atstart) {
        p = a->ps; w = a->ws;
        for (i = rm, m = v; i != m; i--) {
            if (DET(i->psum - z, i->wsum - c, static_cast<int64>(p), static_cast<int64>(w)) >= 0) { k--; *k = *i; }
        }
    }
    if (!atend) {
        p = a->pt; w = a->wt;
        for (i = v, m = r1 - 1; i != m; i--) {
            if (DET(i->psum - z, i->wsum - c, static_cast<int64>(p), static_cast<int64>(w)) >= 0) { k--; *k = *i; }
        }
    }

    /* save limit */
    a->d.fset = k;
    a->d.lset = a->d.setm - 1; /* reserve one record for multiplication */
    a->d.size = DIFF(a->d.fset, a->d.lset);
}

static void initfirst(AllInfo *a, stype pb, stype wb) {
    PartSet *d;
    State *k;
    btype i;

    /* create table */
    d = &(a->d);
    d->storage.resize(MAXSTATES);
    d->set1 = d->storage.data();
    d->setm = d->set1 + MAXSTATES - 1;
    d->size = 1;
    d->fset = d->set1;
    d->lset = d->set1;

    /* init first state */
    k = d->fset;
    k->psum = pb;
    k->wsum = wb;
    k->vect = 0;

    /* init solution vector */
    for (i = 0; i < MAXV; i++) d->vitem[i] = nullptr;
    d->vno = MAXV - 1;

    /* init full solution */
    a->fullsol = false;
    a->full_solution_storage.resize(DIFF_ITEM(a->fitem, a->litem));
    a->ffull = a->full_solution_storage.data();
    a->lfull = a->ffull + DIFF_ITEM(a->fitem, a->litem);
}

static void swapout(AllInfo *a, Item *i, int side) {
    if (side == LEFT) {
        auto it = a->stack_left.begin();
        while (it != a->stack_left.end()) {
            if ((it->f <= i) && (i <= it->l)) {
                std::swap(*i, *(it->l));
                it->l--;
                it++;
                break;
            }
            it++;
        }
        while (it != a->stack_left.end()) {
            std::swap(*(it->f - 1), *(it->l));
            it->f--;
            it->l--;
            it++;
        }
        // Remove empty intervals
        a->stack_left.erase(
            std::remove_if(a->stack_left.begin(), a->stack_left.end(),
                [](const Interval &interval) { return interval.f > interval.l; }),
            a->stack_left.end());
    }
    else {
        auto it = a->stack_right.begin();
        while (it != a->stack_right.end()) {
            if ((it->f <= i) && (i <= it->l)) {
                std::swap(*i, *(it->f));
                it->f++;
                it++;
                break;
            }
            it++;
        }
        while (it != a->stack_right.end()) {
            std::swap(*(it->f), *(it->l + 1));
            it->f++;
            it->l++;
            it++;
        }
        // Remove empty intervals
        a->stack_right.erase(
            std::remove_if(a->stack_right.begin(), a->stack_right.end(),
                [](const Interval &interval) { return interval.f > interval.l; }),
            a->stack_right.end());
    }
}

static void findcore(AllInfo *a) {
    Item *i, *m;
    itype p, r;
    Item *j, *s, *t, *b;

    /* all items apart from b must be in intervals */
    s = t = b = a->b;
    if (a->fpart <= b - 1) push(a, LEFT, a->fpart, b - 1);
    if (b + 1 <= a->lpart) push(a, RIGHT, b + 1, a->lpart);

    /* initial core is b-1, b, b+1 (if these exist) */
    if (b - 1 >= a->fitem) { swapout(a, b - 1, LEFT); s--; }
    if (b + 1 <= a->litem) { swapout(a, b + 1, RIGHT); t++; }

    /* forward greedy solution */
    if (b - 1 >= a->fitem) {
        p = 0; r = a->c - a->wsumb + (b - 1)->w;
        for (i = t + 1, m = a->litem + 1, j = nullptr; i != m; i++) {
            if ((i->w <= r) && (i->p > p)) { p = i->p; j = i; }
        }
        if (j != nullptr) { swapout(a, j, RIGHT); t++; }
    }

    /* second forward greedy solution */
    if (true) {
        p = 0; r = a->c - a->wsumb;
        for (i = t + 1, m = a->litem + 1, j = nullptr; i != m; i++) {
            if ((i->w <= r) && (i->p > p)) { p = i->p; j = i; }
        }
        if (j != nullptr) { swapout(a, j, RIGHT); t++; }
    }

    /* backward greedy solution */
    if (true) {
        j = nullptr; r = a->wsumb - a->c + b->w;
        for (i = a->fitem, m = s; i != m; i++) if (i->w >= r) { p = i->p + 1; break; }
        for (; i != m; i++) if ((i->w >= r) && (i->p < p)) { p = i->p; j = i; }
        if (j != nullptr) { swapout(a, j, LEFT); s--; }
    }

    /* second backward solution */
    if (b + 1 <= a->litem) {
        j = nullptr; r = a->wsumb - a->c + b->w + (b + 1)->w;
        for (i = a->fitem, m = s; i != m; i++) if (i->w >= r) { p = i->p + 1; break; }
        for (; i != m; i++) if ((i->w >= r) && (i->p < p)) { p = i->p; j = i; }
        if (j != nullptr) { swapout(a, j, LEFT); s--; }
    }

    /* add first and last item to ensure some variation in weights */
    if (a->fitem < s) { s--; swapout(a, a->fitem, LEFT); }
    if (a->litem > t) { t++; swapout(a, a->litem, RIGHT); }

    /* enumerate core: reductions are not allowed! */
    initfirst(a, a->psumb, a->wsumb); moveset(a);
    for (i = b, j = b - 1; (i <= t) || (j >= s); ) {
        if (i <= t) { multiply(a, i, RIGHT); moveset(a); i++; }
        if (j >= s) { multiply(a, j, LEFT); moveset(a); j--; }
    }
    a->s = s - 1; a->fsort = s;
    a->t = t + 1; a->lsort = t;
}

static void heuristic(AllInfo *a) {
    Item *i, *j, *m;
    stype c, z, ub;
    State *v, *r1, *rm;
    Item *red, d;

    if (a->d.size == 0) return;

    /* define limits */
    dynheur++;
    r1 = a->d.fset; rm = a->d.lset;
    c = a->c; z = a->z; ub = a->ub;

    /* forward solution with dyn prog */
    if (!a->stack_right.empty()) {
        red = a->stack_right.back().f;
        for (i = red, m = a->litem + 1, j = nullptr; i != m; i++) {
            v = findvect(c - i->w, r1, rm); if (v == nullptr) continue;
            if (v->psum + i->p > z) { j = i; z = v->psum + i->p; if (z == ub) break; }
        }
        if (j != nullptr) {
            swapout(a, j, RIGHT); d = *red;
            for (i = red, m = a->t; i != m; i--) *i = *(i - 1);
            *(a->t) = d; a->lsort++;
            multiply(a, a->t, RIGHT); a->t++;
            reduceset(a);
        }
    }

    /* backward solution with dyn prog */
    if (!a->stack_left.empty()) {
        red = a->stack_left.back().l;
        for (i = a->fitem, m = red + 1, j = nullptr; i != m; i++) {
            v = findvect(c + i->w, r1, rm); if (v == nullptr) continue;
            if (v->psum - i->p > z) { j = i; z = v->psum - i->p; if (z == ub) break; }
        }
        if (j != nullptr) {
            swapout(a, j, LEFT); d = *red;
            for (i = red, m = a->s; i != m; i++) *i = *(i + 1);
            *(a->s) = d; a->fsort--;
            multiply(a, a->s, LEFT); a->s--;
            reduceset(a);
        }
    }
}

static void findbreak(AllInfo *a) {
    Item *i;
    stype psum, r;
    stype wsum;

    /* find break item */
    psum = 0; r = a->c;
    for (i = a->fitem; i->w <= r; i++) { psum += i->p; r -= i->w; }
    wsum = a->c - r;

    a->b = i;
    a->psumb = psum;
    a->wsumb = wsum;
    a->dantzig = psum + (r * static_cast<prod>(i->p)) / i->w;
}

stype combo(Item *f, Item *l, stype c, stype lb, stype ub,
    boolean def, boolean relx) {

    // Initialize structure locally
    AllInfo a;

    // Setup Intervals - Vectors handle memory automatically
    // a.interval_stack.resize(SORTSTACK); ... removed

    a.fitem = f;
    a.litem = l;
    a.c = c;
    a.z = lb;
    a.lb = lb;
    a.relx = relx;
    a.master = (def && !relx);
    a.coresize_debug = 0;

    boolean heur = false;
    boolean rudi = false;

    if ((ub != 0) && (lb == ub)) return lb;

    partsort(&a, a.fitem, a.litem, 0, a.c, PARTITION);
    findbreak(&a);
    a.ub = (ub == 0 ? a.dantzig : ub);

    /* find and enumerate core */
    findcore(&a);
    reduceset(&a);

    while ((a.d.size > 0) && (a.z < a.ub)) {
        if (a.t <= a.lsort) {
            if (haschance(&a, a.t, RIGHT)) multiply(&a, a.t, RIGHT);
            a.t++;
        }
        reduceset(&a);
        if (a.s >= a.fsort) {
            if (haschance(&a, a.s, LEFT)) multiply(&a, a.s, LEFT);
            a.s--;
        }
        reduceset(&a);

        /* find better lower bound when needed */
        if ((!heur) && (a.d.size > MINHEUR)) { heuristic(&a); heur = true; }

        /* find tight bound when needed */
        if ((!relx) && (a.d.size > MINSET)) { surrelax(&a); relx = true; }

        /* use rudimentary divisibility to decrease c */
        if ((!rudi) && (a.d.size > MINRUDI)) { rudidiv(&a); rudi = true; }
    }

    // Pointers are automatically handled by std::vector destructor in AllInfo.d.storage
    // and interval_stack

    if ((def) && (a.z > a.lb)) definesolution(&a);

    return a.z;
}

// Wrapper for easy C++ access
stype solve_knapsack(std::vector<Item> &items, stype capacity) {
    if (items.empty()) return 0;

    return combo(items.data(), items.data() + items.size() - 1, capacity, 0, 0, true, false);
}

int main(int argc, char *argv[]) {
    // Fast I/O
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

    // Convert KnapsackInstance to vector<Item>
    std::vector<Item> items(instance.n);
    for (size_t i = 0; i < instance.n; ++i) {
        items[i].p = instance.values[i];
        items[i].w = instance.weights[i];
        items[i].x = false;
        items[i].id = static_cast<int>(i); // Store original index
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Solve
    stype maxValue = solve_knapsack(items, instance.capacity);

    auto end = std::chrono::high_resolution_clock::now();
    int64 executionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Collect selected items
    std::vector<size_t> selectedItems;
    for (const auto& item : items) {
        if (item.x) {
            selectedItems.push_back(static_cast<size_t>(item.id));
        }
    }
    // Sort selected items by index for consistent output
    std::sort(selectedItems.begin(), selectedItems.end());

    // Memory estimation (approximate)
    // Vector of items + internal structures of combo (AllInfo has vectors)
    // The combo algorithm allocates a vector of MAXSTATES States.
    size_t memoryUsed = sizeof(Item) * items.size() + sizeof(size_t) * selectedItems.size();
    memoryUsed += sizeof(State) * MAXSTATES; 

    // Output
    std::cout << maxValue << std::endl;
    std::cout << selectedItems.size() << std::endl;
    for (size_t i = 0; i < selectedItems.size(); ++i) {
        std::cout << selectedItems[i] << (i == selectedItems.size() - 1 ? "" : " ");
    }
    if (!selectedItems.empty()) std::cout << std::endl;
    
    std::cout << executionTime << std::endl;
    std::cout << memoryUsed << std::endl;

    return 0;
}
