#!/usr/bin/env python3
"""
Script to process knapsack problem instances and generate individual text files.
Processes all testcases from data/gh_knapsackProblemInstances/problemInstances directory.

Output format:
    /data/knapsack_hard1/<category>/<testid>.txt
    where category is H1known or H1unknown
    and testid increments from 1 within each category

Text file format:
    Line 1: <n> <capacity> <max_weight> <min_weight> <optimum_value>
            (optimum_value may be blank if unknown)
    Line 2: <optimal_pick_1> <optimal_pick_2> ...
            (may be blank if unknown)
    Lines 3+: <weight_i> <value_i>
"""

import csv
import shutil
from pathlib import Path


def parse_test_input(test_in_path):
    """Parse test.in file and extract n, weights, prices, and capacity."""
    with open(test_in_path, "r") as f:
        lines = f.readlines()

    n = int(lines[0].strip())
    weights = []
    prices = []

    for i in range(1, n + 1):
        parts = lines[i].strip().split()
        # parts[0] is item_id, which we don't need to store
        price = int(parts[1])
        weight = int(parts[2])
        weights.append(weight)
        prices.append(price)

    capacity = int(lines[n + 1].strip())

    return n, weights, prices, capacity


def parse_output(outp_out_path):
    """Parse outp.out file and extract best_price and best_picks."""
    try:
        with open(outp_out_path, "r") as f:
            lines = f.readlines()

        if (
            not lines
            or lines[0].strip().startswith("ERROR")
            or lines[0].strip().startswith("Error")
        ):
            return None, None

        # First line is the total profit
        best_price = int(lines[0].strip())

        # Subsequent lines contain item descriptions (profit weight)
        # We need to find which items from the original list match these
        selected_items = []
        for i in range(1, len(lines)):
            line = lines[i].strip()
            if line:
                parts = line.split()
                if len(parts) == 2:
                    price = int(parts[0])
                    weight = int(parts[1])
                    selected_items.append((price, weight))

        return best_price, selected_items
    except (ValueError, FileNotFoundError):
        return None, None


def find_item_indices(weights, prices, selected_items):
    """Find the indices of selected items in the original lists."""
    indices = []
    used = [False] * len(weights)

    for sel_price, sel_weight in selected_items:
        # Find matching item that hasn't been used yet
        for i in range(len(weights)):
            if not used[i] and weights[i] == sel_weight and prices[i] == sel_price:
                indices.append(i)
                used[i] = True
                break

    return sorted(indices)


def load_optima(optima_path):
    """Load the optima.csv file and return a dictionary mapping name to optimum."""
    optima = {}
    with open(optima_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            name = row["name"]
            optimum = int(row["optimum"])
            optima[name] = optimum
    return optima


def write_instance_file(filepath, n, weights, prices, capacity, best_picks, best_price):
    """Write a single instance to a text file in the new format."""
    max_weight = max(weights) if weights else 0
    min_weight = min(weights) if weights else 0

    with open(filepath, "w") as f:
        # Line 1: n capacity max_weight min_weight [optimum_value]
        if best_price is not None and best_price != "":
            f.write(f"{n} {capacity} {max_weight} {min_weight} {best_price}\n")
        else:
            f.write(f"{n} {capacity} {max_weight} {min_weight}\n")

        # Line 2: optimal picks (space-separated)
        if best_picks:
            f.write(" ".join(map(str, best_picks)) + "\n")
        else:
            f.write("\n")

        # Lines 3+: weight value pairs
        for i in range(n):
            f.write(f"{weights[i]} {prices[i]}\n")


def main():
    # Setup paths
    base_dir = Path(__file__).parent
    instances_dir = base_dir / "gh_knapsackProblemInstances" / "problemInstances"
    optima_path = base_dir / "gh_knapsackProblemInstances" / "optima.csv"

    output_dir = base_dir / "knapsack_hard1"

    # Load optima
    print("Loading optima.csv...")
    optima = load_optima(optima_path)
    print(f"Loaded {len(optima)} entries from optima.csv")

    # Create output directory with category subdirectories
    if output_dir.exists():
        print(f"Removing existing directory: {output_dir}")
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "H1known").mkdir(parents=True, exist_ok=True)
    (output_dir / "H1unknown").mkdir(parents=True, exist_ok=True)
    print(f"Created output directory: {output_dir}")

    # Process all subdirectories
    print("\nProcessing problem instances...")
    instance_folders = sorted([d for d in instances_dir.iterdir() if d.is_dir()])

    # Track test_id per category
    known_test_id = 0
    unknown_test_id = 0
    known_count = 0
    unknown_count = 0

    for idx, folder in enumerate(instance_folders, 1):
        folder_name = folder.name
        test_in_path = folder / "test.in"
        outp_out_path = folder / "outp.out"

        if idx % 100 == 0:
            print(f"Processed {idx}/{len(instance_folders)} instances...")

        if not test_in_path.exists():
            print(f"Warning: {folder_name} missing test.in")
            continue

        # Parse test input
        n, weights, prices, capacity = parse_test_input(test_in_path)

        # Check if optimum is known
        optimum = optima.get(folder_name, -1)

        if optimum != -1:
            # Parse output to get best picks
            best_price, selected_items = parse_output(outp_out_path)

            if best_price is not None and best_price == optimum:
                # Find indices of selected items
                best_picks = find_item_indices(weights, prices, selected_items)
                known_test_id += 1
                filepath = output_dir / "H1known" / f"{known_test_id}.txt"
                write_instance_file(
                    filepath, n, weights, prices, capacity, best_picks, best_price
                )
                known_count += 1
            else:
                print(
                    f"Warning: {folder_name} - could not parse output or mismatch with optima"
                )
                unknown_test_id += 1
                filepath = output_dir / "H1unknown" / f"{unknown_test_id}.txt"
                write_instance_file(filepath, n, weights, prices, capacity, [], None)
                unknown_count += 1
        else:
            # Optimum not known
            unknown_test_id += 1
            filepath = output_dir / "H1unknown" / f"{unknown_test_id}.txt"
            write_instance_file(filepath, n, weights, prices, capacity, [], None)
            unknown_count += 1

    # Write metadata file
    total_count = known_count + unknown_count
    metadata_path = output_dir / "metadata.txt"
    with open(metadata_path, "w") as f:
        f.write("source=gh_knapsackProblemInstances\n")
        f.write(f"total={total_count}\n")
        f.write(f"H1known={known_count}\n")
        f.write(f"H1unknown={unknown_count}\n")

    print(f"\nProcessed {len(instance_folders)} instances")
    print(f"H1known: {known_count}")
    print(f"H1unknown: {unknown_count}")

    print("\nDone!")
    print(f"Generated {total_count} files in: {output_dir}")
    print(f"  H1known/{known_count} files, H1unknown/{unknown_count} files")


if __name__ == "__main__":
    main()
