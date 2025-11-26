#!/usr/bin/env python3
"""Generate knapsack problems dataset using a C++ worker.

This script orchestrates the generation of a dataset of 0/1 knapsack problems.
It can generate "easy", "trap", or "random" problems via a command-line argument.

"Easy" problems: have a known optimal solution where the selected items have
much better price/weight ratios.

"Trap" problems: are hard by construction with a known optimal solution but
are designed to "trap" greedy (price/weight ratio) algorithms into finding
a sub-optimal one.

"Random" problems: use a class-based structure with items centered around
fractions of capacity (1/2, 1/4, 1/8, etc.) plus noise, along with small items.
No pre-computed optimal solution.

This script's roles:
1.  Parse command-line arguments (mode, total, level, seed, etc.).
2.  Compile the appropriate C++ worker program.
3.  Create output directory structure.
4.  Loop `total` times, calling the C++ worker for each problem.

The C++ worker's role:
1.  Receive params from this script.
        easy: filepath, n, seed
        trap: filepath, n, capacity, seed
        random: filepath, n, capacity, seed
2.  Generate a single instance.
3.  Write that single instance to a text file.

Output format:
    /data/<datasetname>/<category>/<testid>.txt
    where category is Tiny/Small/Medium/Large/Massive
    and testid increments from 1 within each category

Text file format:
    Line 1: <n> <capacity> <max_weight> <min_weight> <optimum_value>
            (optimum_value may be blank if unknown)
    Line 2: <optimal_pick_1> <optimal_pick_2> ...
            (may be blank if unknown)
    Lines 3+: <weight_i> <value_i>

Command-line arguments (all provided as --arg value):
    --mode    : problem mode - "easy", "trap", or "random" (default: easy)
    --name    : dataset name (default: knapsack_easy, knapsack_trap, or knapsack_random)
    --total   : total number of problems to generate (default: 100)
    --level   : maximum difficulty level to include (default: 2)
                    0 => Tiny only
                    1 => Tiny + Small
                    2 => Tiny + Small + Medium
                    3 => ONLY Large
                    4 => ONLY Massive
                    5 => Tiny + Small + Medium + Large
    --seed    : optional integer seed for reproducible sampling of (category,n)
"""

from __future__ import annotations

import argparse
import math
import random
import gc
import subprocess
import sys
import shutil
from pathlib import Path
from typing import Dict, List, Optional


def log_uniform_int(low: int, high: int, rng: random.Random) -> int:
    """Sample integer between low..high (inclusive) on a log-uniform scale."""
    if low >= high:
        return low
    log_low = math.log10(low)
    log_high = math.log10(high)
    u = rng.random() * (log_high - log_low) + log_low
    return max(low, min(high, int(round(10**u))))


def build_n_list(
    total: int, rng: random.Random, level: Optional[int] = None
) -> List[Dict]:
    """Build a list of (category, n) entries totaling `total` problems.

    Categories and sampling ranges:
      Tiny   : 20 - 40
      Small  : 10^2 - 10^3
      Medium : 10^4 - 10^5
      Large  : 10^6 - 10^7
      Massive: 10^8 - 10^9

    We distribute `total` roughly evenly across the included categories and
    sample n log-uniformly (Tiny uniformly).
    """
    categories = [
        ("Tiny", 20, 40),
        ("Small", 10**2, 10**3),
        ("Medium", 10**4, 10**5),
        ("Large", 10**6, 10**7),
        ("Massive", 10**8, 10**9),
    ]
    # level controls how many categories to include: 0..5
    if level is not None:
        if level < 0 or level > 5:
            raise ValueError("level must be 0, 1, 2, 3, 4, 5, or omitted")
        if level <= 2:  # 0, 1, 2 are cumulative
            categories = categories[: level + 1]
        elif level == 3:  # Large only
            categories = [categories[3]]
        elif level == 4:  # Massive only
            categories = [categories[4]]
        elif level == 5:  # Tiny + Small + Medium + Large
            categories = categories[:4]

    # Distribute `total` evenly across the number of included categories.
    num_cats = len(categories)
    per_cat = [total // num_cats] * num_cats
    for i in range(total % num_cats):
        per_cat[i] += 1

    result = []
    # Use per-category counters to avoid rescanning `result` on every loop
    per_cat_counts = {c[0]: 0 for c in categories}
    for (cat, low, high), count in zip(categories, per_cat):
        # For very small ranges (like Tiny) it's possible the user requests more
        # entries than there are unique n values. In that case allow duplicates
        # instead of spinning forever trying to deduplicate.
        unique_possible = (high - low + 1) if cat == "Tiny" else None
        seen = set()
        attempts = 0
        while per_cat_counts[cat] < count and attempts < max(1000, count * 10):
            attempts += 1
            if cat == "Tiny":
                n = rng.randint(low, high)
            else:
                n = log_uniform_int(low, high, rng)
            # If duplicates are possible but the requested count exceeds the
            # unique possibilities, don't deduplicate.
            if unique_possible is not None and count <= unique_possible:
                if (cat, n) in seen:
                    continue
                seen.add((cat, n))
            result.append({"category": cat, "n": int(n)})
            per_cat_counts[cat] += 1

    # Shuffle to avoid grouped categories
    rng.shuffle(result)
    return result


def compile_cpp_worker(cpp_src: Path, cpp_exe: Path) -> bool:
    """Compile the C++ worker if it's missing or outdated."""
    # Ensure bin directory exists
    cpp_exe.parent.mkdir(parents=True, exist_ok=True)

    if not cpp_exe.exists() or (
        cpp_src.exists() and cpp_src.stat().st_mtime > cpp_exe.stat().st_mtime
    ):
        print(f"Compiling C++ worker: {cpp_src} -> {cpp_exe}")
        compile_cmd = ["g++", "-O3", "-std=c++17", str(cpp_src), "-o", str(cpp_exe)]

        try:
            subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
            print("Compilation successful.")
            return True
        except FileNotFoundError:
            print("Error: g++ compiler not found. Please install g++.", file=sys.stderr)
            return False
        except subprocess.CalledProcessError as e:
            print("Error: C++ compilation failed.", file=sys.stderr)
            print("STDOUT:", file=sys.stderr)
            print(e.stdout, file=sys.stderr)
            print("STDERR:", file=sys.stderr)
            print(e.stderr, file=sys.stderr)
            return False
    else:
        # print(f"C++ worker '{cpp_exe}' is up to date.")
        return True


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "--mode",
        choices=["easy", "trap", "random"],
        default="easy",
        help="Problem mode: 'easy', 'trap', or 'random' (default: easy)",
    )
    parser.add_argument(
        "--name",
        default=None,
        help="Dataset name (default: knapsack_easy, knapsack_trap, or knapsack_random)",
    )
    parser.add_argument(
        "--total",
        type=int,
        default=100,
        help="Total number of problems to generate (default 100)",
    )
    parser.add_argument(
        "--level",
        type=int,
        choices=[0, 1, 2, 3, 4, 5],
        default=2,
        help="Which difficulty levels to include (default 2)\n",
    )
    parser.add_argument("--seed", type=int, default=None)
    args = parser.parse_args()

    # Set default dataset name based on mode if not specified
    if args.name is None:
        if args.mode == "easy":
            args.name = "knapsack_easy"
        elif args.mode == "trap":
            args.name = "knapsack_trap"
        elif args.mode == "random":
            args.name = "knapsack_random"
        else:
            print(f"Error: Unknown mode '{args.mode}'", file=sys.stderr)
            sys.exit(1)

    script_dir = Path(__file__).parent
    output_dir = script_dir / args.name

    # --- 1. Compile C++ Worker ---
    if args.mode == "easy":
        cpp_src_file = "generate_easy_instance.cpp"
        cpp_exe_file = "generate_easy_instance"
    elif args.mode == "trap":
        cpp_src_file = "generate_trap_instance.cpp"
        cpp_exe_file = "generate_trap_instance"
    elif args.mode == "random":
        cpp_src_file = "generate_random_instance.cpp"
        cpp_exe_file = "generate_random_instance"
    else:
        print(f"Error: Unknown mode '{args.mode}'", file=sys.stderr)
        sys.exit(1)

    if sys.platform == "win32":
        cpp_exe_file += ".exe"

    cpp_src_path = script_dir / cpp_src_file
    if not cpp_src_path.exists():
        print(
            f"Error: C++ source file '{cpp_src_file}' not found in the same directory.",
            file=sys.stderr,
        )
        sys.exit(1)

    bin_dir = script_dir / "bin"
    cpp_exe_path = bin_dir / cpp_exe_file
    if not compile_cpp_worker(cpp_src_path, cpp_exe_path):
        sys.exit(1)

    # --- 2. Build Problem Specs ---
    rng = random.Random(args.seed)
    specs = build_n_list(args.total, rng, level=args.level)
    if not specs:
        print("No problems to generate. Exiting.")
        return

    # --- 3. Create Output Directory ---
    # Remove existing directory if it exists
    if output_dir.exists():
        print(f"Removing existing directory: {output_dir}")
        shutil.rmtree(output_dir)

    output_dir.mkdir(parents=True, exist_ok=True)
    print(f"Created output directory: {output_dir}")

    # --- 4. Create category subdirectories and count per category ---
    # Prefix categories with mode letter: E=easy, T=trap, R=random
    mode_prefix = {"easy": "E", "trap": "T", "random": "R"}[args.mode]

    category_counts = {}
    for spec in specs:
        cat = spec["category"]
        prefixed_cat = f"{mode_prefix}{cat}"
        spec["prefixed_category"] = prefixed_cat  # Store for later use
        category_counts[prefixed_cat] = category_counts.get(prefixed_cat, 0) + 1

    for cat in category_counts:
        (output_dir / cat).mkdir(parents=True, exist_ok=True)

    # Track test_id per category
    category_test_ids = {cat: 0 for cat in category_counts}

    # --- 5. Write metadata file ---
    metadata_path = output_dir / "metadata.txt"
    with open(metadata_path, "w") as f:
        f.write(f"mode={args.mode}\n")
        f.write(f"total={len(specs)}\n")
        f.write(f"level={args.level}\n")
        f.write(f"seed={args.seed}\n")
        for cat, count in sorted(category_counts.items()):
            f.write(f"{cat}={count}\n")

    # --- 6. Main Generation Loop (Call C++) ---
    mode_str = args.mode
    print(f"Generating {len(specs)} {mode_str} problem instances into {output_dir}...")

    for i, spec in enumerate(specs):
        cat = spec["prefixed_category"]  # Use prefixed category
        n = spec["n"]
        seed = rng.randint(0, 2**31 - 1)

        # Test ID increments per category
        category_test_ids[cat] += 1
        test_id = category_test_ids[cat]
        filepath = output_dir / cat / f"{test_id}.txt"

        # Build command based on mode
        if args.mode == "easy":
            # Easy mode does not require capacity
            cmd = [str(cpp_exe_path), str(filepath), str(n), str(seed)]
        elif args.mode == "trap" or args.mode == "random":
            capacity = 0
            # Trap and random modes require capacity parameter
            # Use original category (without prefix) for capacity lookup
            orig_cat = spec["category"]
            if orig_cat == "Tiny":
                capacity = 100_000
            elif orig_cat == "Small":
                capacity = 1_000_000
            elif orig_cat == "Medium":
                capacity = 10_000_000
            elif orig_cat == "Large":
                capacity = 100_000_000
            elif orig_cat == "Massive":
                capacity = 1_000_000_000
            cmd = [
                str(cpp_exe_path),
                str(filepath),
                str(n),
                str(capacity),
                str(seed),
            ]
        else:
            print(f"Error: Unknown mode '{args.mode}'", file=sys.stderr)
            sys.exit(1)

        try:
            subprocess.run(cmd, check=True, text=True, capture_output=True)
        except subprocess.CalledProcessError as e:
            print(
                f"\nError: C++ worker failed for instance {test_id} (n={n}, seed={seed})",
                file=sys.stderr,
            )
            print("STDERR:", file=sys.stderr)
            print(e.stderr, file=sys.stderr)
            print("Aborting.", file=sys.stderr)
            sys.exit(1)

        # Simple progress bar
        print(
            f"  ... Wrote problem {i + 1}/{len(specs)} ({cat}/{test_id}, n={n})",
            end="\r",
            flush=True,
        )

        # Hint to GC
        if (i + 1) % 100 == 0:
            gc.collect()

    print(
        f"\nSuccessfully wrote {len(specs)} problems to {output_dir} (mode={args.mode}, seed={args.seed}, level={args.level})"
    )
    for cat, count in sorted(category_counts.items()):
        print(f"  {cat}: {count} instances")


if __name__ == "__main__":
    main()
