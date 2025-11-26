#!/usr/bin/env python3
"""
Script to process knapsack problem instances and generate individual text files.
Processes all testcases from data/gh_knapsack-01-instances directory.

Output format:
    /data/knapsack_hard2/<category>/<testid>.txt
    where category is H2pisingerlarge, H2pisingerlowdim, or H2xiang
    and testid increments from 1 within each category

Text file format:
    Line 1: <n> <capacity> <max_weight> <min_weight> <optimum_value>
            (optimum_value may be blank if unknown)
    Line 2: <optimal_pick_1> <optimal_pick_2> ...
            (may be blank if unknown)
    Lines 3+: <weight_i> <value_i>
"""

import shutil
from pathlib import Path


def parse_instance_file(instance_path, has_solution=False):
    """Parse instance file and extract n, weights, prices, capacity, and optional solution.
    Format: First line is 'n capacity', followed by n lines of 'price weight'.
    Large-scale files have an additional line with binary solution (space-separated 0s and 1s).
    """
    with open(instance_path, "r") as f:
        lines = f.readlines()

    # First line: n capacity
    first_line = lines[0].strip().split()
    n = int(first_line[0])
    capacity = int(first_line[1])

    weights = []
    prices = []

    # Next n lines: price weight
    for i in range(1, n + 1):
        parts = lines[i].strip().split()
        try:
            # Handle both integer and float values (convert floats to ints)
            price = int(float(parts[0]))
            weight = int(float(parts[1]))
        except (ValueError, IndexError) as e:
            raise ValueError(
                f"Error parsing line {i + 1} in {instance_path}: {lines[i].strip()}"
            ) from e
        prices.append(price)
        weights.append(weight)

    # Check if there's a solution line (binary string)
    best_picks = []
    if has_solution and len(lines) > n + 1:
        solution_line = lines[n + 1].strip()
        if solution_line:  # Not empty
            binary_solution = solution_line.split()
            # Convert to list of indices where value is 1
            best_picks = [i for i, val in enumerate(binary_solution) if val == "1"]

    return n, weights, prices, capacity, best_picks


def parse_optimum_file(optimum_path):
    """Parse optimum file and extract the best price value.
    Returns the optimal value if file exists, otherwise None.
    """
    try:
        with open(optimum_path, "r") as f:
            return int(f.read().strip())
    except (ValueError, FileNotFoundError):
        return None


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
    instances_dir = base_dir / "gh_knapsack-01-instances"
    output_dir = base_dir / "knapsack_hard2"

    # Create output directory with category subdirectories
    if output_dir.exists():
        print(f"Removing existing directory: {output_dir}")
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "H2pisingerlarge").mkdir(parents=True, exist_ok=True)
    (output_dir / "H2pisingerlowdim").mkdir(parents=True, exist_ok=True)
    (output_dir / "H2xiang").mkdir(parents=True, exist_ok=True)
    print(f"Created output directory: {output_dir}")

    # Track test_id per category
    large_scale_test_id = 0
    low_dim_test_id = 0
    xiang_test_id = 0
    large_scale_count = 0
    low_dim_count = 0
    xiang_count = 0

    # Process Pisinger instances
    print("Processing Pisinger instances...")
    pisinger_dir = instances_dir / "pisinger_instances_01_KP"

    # Process large_scale instances
    large_scale_dir = pisinger_dir / "large_scale"
    large_scale_optimum_dir = pisinger_dir / "large_scale-optimum"

    if large_scale_dir.exists():
        instance_files = sorted([f for f in large_scale_dir.iterdir() if f.is_file()])
        for idx, instance_file in enumerate(instance_files, 1):
            if idx % 10 == 0:
                print(
                    f"  Processed {idx}/{len(instance_files)} large-scale instances..."
                )

            # Parse instance (large-scale files have binary solution)
            n, weights, prices, capacity, best_picks = parse_instance_file(
                instance_file, has_solution=True
            )

            # Check for optimum
            optimum_file = large_scale_optimum_dir / instance_file.name
            best_price = parse_optimum_file(optimum_file)

            large_scale_test_id += 1
            filepath = output_dir / "H2pisingerlarge" / f"{large_scale_test_id}.txt"
            write_instance_file(
                filepath, n, weights, prices, capacity, best_picks, best_price
            )
            large_scale_count += 1

    # Process low-dimensional instances
    low_dim_dir = pisinger_dir / "low-dimensional"
    low_dim_optimum_dir = pisinger_dir / "low-dimensional-optimum"

    if low_dim_dir.exists():
        instance_files = sorted([f for f in low_dim_dir.iterdir() if f.is_file()])
        for idx, instance_file in enumerate(instance_files, 1):
            # Parse instance (low-dimensional files don't have solution)
            n, weights, prices, capacity, best_picks = parse_instance_file(
                instance_file, has_solution=False
            )

            # Check for optimum
            optimum_file = low_dim_optimum_dir / instance_file.name
            best_price = parse_optimum_file(optimum_file)

            low_dim_test_id += 1
            filepath = output_dir / "H2pisingerlowdim" / f"{low_dim_test_id}.txt"
            write_instance_file(filepath, n, weights, prices, capacity, [], best_price)
            low_dim_count += 1

    print(
        f"Processed {large_scale_count} large-scale + {low_dim_count} low-dimensional Pisinger instances"
    )

    # Process Xiang instances
    print("\nProcessing Xiang instances...")
    xiang_dir = instances_dir / "xiang_instances_01_KP"

    if xiang_dir.exists():
        instance_files = sorted([f for f in xiang_dir.iterdir() if f.is_file()])
        for idx, instance_file in enumerate(instance_files, 1):
            # Parse instance (Xiang files don't have solution)
            n, weights, prices, capacity, best_picks = parse_instance_file(
                instance_file, has_solution=False
            )

            xiang_test_id += 1
            filepath = output_dir / "H2xiang" / f"{xiang_test_id}.txt"
            write_instance_file(filepath, n, weights, prices, capacity, [], None)
            xiang_count += 1

    print(f"Processed {xiang_count} Xiang instances")

    # Write metadata file
    total_count = large_scale_count + low_dim_count + xiang_count
    metadata_path = output_dir / "metadata.txt"
    with open(metadata_path, "w") as f:
        f.write("source=gh_knapsack-01-instances\n")
        f.write(f"total={total_count}\n")
        f.write(f"H2pisingerlarge={large_scale_count}\n")
        f.write(f"H2pisingerlowdim={low_dim_count}\n")
        f.write(f"H2xiang={xiang_count}\n")

    print("\nDone!")
    print(f"Generated {total_count} files in: {output_dir}")
    print(
        f"  H2pisingerlarge/{large_scale_count}, H2pisingerlowdim/{low_dim_count}, H2xiang/{xiang_count}"
    )


if __name__ == "__main__":
    main()
