#!/usr/bin/env python3
"""
Knapsack Algorithm Simulation and Analysis
Runs various knapsack algorithms and generates comprehensive visualisations
"""

import asyncio
import logging
import platform
import re
import shutil
import subprocess
import time
from datetime import datetime
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

# Set style for better visualisations
sns.set_style("whitegrid")
plt.rcParams["figure.figsize"] = (12, 8)
plt.rcParams["font.size"] = 10


# Global logger
logger = logging.getLogger(__name__)


class KnapsackSimulator:
    def __init__(self, base_path):
        self.base_path = Path(base_path)
        self.algorithms_path = self.base_path / "algorithms"
        self.data_path = self.base_path / "data"
        self.simulation_path = self.base_path / "simulation"
        self.results_path = self.simulation_path / "results"
        self.visualisation_path = self.simulation_path / "visualisations"
        self.logs_path = self.simulation_path / "logs"

        # Create necessary directories
        self.results_path.mkdir(exist_ok=True)
        self.visualisation_path.mkdir(exist_ok=True)
        self.logs_path.mkdir(exist_ok=True)

        # Algorithm configurations
        # Add an entry for each executable placed in algorithms/bin
        self.algorithms = {
            "bruteforce": {
                "executable": self.algorithms_path / "bin" / "bruteforce",
                "name": "Brute Force",
                "sort_key": lambda n, w: n,  # 2**n,
                "millionscale": False,
            },
            "memoization": {
                "executable": self.algorithms_path / "bin" / "memoization",
                "name": "Memoization",
                "sort_key": lambda n, w: n * w,
                "millionscale": False,
            },
            "dynamicprogramming": {
                "executable": self.algorithms_path / "bin" / "dynamicprogramming",
                "name": "Dynamic Programming",
                "sort_key": lambda n, w: n * w,
                "millionscale": False,
            },
            "branchandbound": {
                "executable": self.algorithms_path / "bin" / "branchandbound",
                "name": "Branch and Bound",
                "sort_key": lambda n, w: n,
                "millionscale": False,
            },
            "meetinthemiddle": {
                "executable": self.algorithms_path / "bin" / "meetinthemiddle",
                "name": "Meet in the Middle",
                "sort_key": lambda n, w: n,  # (2 ** (n / 2)) * n,
                "millionscale": False,
            },
            "greedyheuristic": {
                "executable": self.algorithms_path / "bin" / "greedyheuristic",
                "name": "Greedy Heuristic",
                "sort_key": lambda n, w: n,  # n * np.log(n),
                "millionscale": True,
            },
            "randompermutation": {
                "executable": self.algorithms_path / "bin" / "randompermutation",
                "name": "Random Permutation",
                "sort_key": lambda n, w: (n**1.5) * w,
                "millionscale": False,
            },
            "efficientalgo": {
                "executable": self.algorithms_path / "bin" / "efficientalgo",
                "name": "Efficient Algorithm",
                "sort_key": lambda n, w: n,  # n * np.log(n),
                "millionscale": False,
            },
            "billionscale": {
                "executable": self.algorithms_path / "bin" / "billionscale",
                "name": "Billion Scale",
                "sort_key": lambda n, w: n,  # (n * M) + n * np.log(n),
                "millionscale": True,
            },
            "geneticalgorithm": {
                "executable": self.algorithms_path / "bin" / "geneticalgorithm",
                "name": "Genetic Algorithm",
                "sort_key": lambda n, w: n,  # n * P * G,
                "millionscale": True,
            },
            "customalgorithm": {
                "executable": self.algorithms_path / "bin" / "customalgorithm",
                "name": "Custom Algorithm",
                "sort_key": lambda n, w: n,
                "millionscale": True,
            },
            "customtestbed": {
                "executable": self.algorithms_path / "bin" / "customtestbed",
                "name": "Custom Testbed",
                "sort_key": lambda n, w: n,
                "millionscale": True,
            },
        }
        # Base timeout (seconds) used as part of adaptive timeout calculation
        self.base_timeout_seconds = 10.0
        # Memory limit for subprocesses in GB. Set to None to disable.
        self.memory_limit_gb = None

    def _log_multiline_error(self, message, error_text):
        """Helper to log multi-line error messages"""
        logger.error(message)
        for line in error_text.split("\n"):
            if line.strip():
                logger.error(f"\t{line.strip()}")

    def _calculate_timeout(self, n, custom_timeout=None):
        """Calculate adaptive timeout based on problem size"""
        per_item = 0.5  # seconds per item (heuristic)
        timeout_seconds = min(
            120.0, max(2.0, self.base_timeout_seconds + per_item * float(n))
        )
        if custom_timeout is not None:
            timeout_seconds = float(custom_timeout)
        return timeout_seconds

    def _get_executable_path(self, algo_name):
        """Get the executable path for an algorithm, handling Windows .exe extension"""
        executable = self.algorithms[algo_name]["executable"]

        if platform.system() == "Windows" and not executable.exists():
            exe_path = executable.with_suffix(".exe")
            if exe_path.exists():
                executable = exe_path

        if not executable.exists():
            raise FileNotFoundError(f"Executable not found: {executable}")

        return executable

    def _build_command(self, executable):
        """Build command for execution, handling Windows+WSL compatibility"""
        if platform.system() == "Windows" and shutil.which("wsl"):
            path_str = str(executable).replace("\\", "/")
            m = re.match(r"^([A-Za-z]):(.*)$", path_str)
            if m:
                drive = m.group(1).lower()
                rest = m.group(2)
                wsl_path = f"/mnt/{drive}{rest}"
            else:
                wsl_path = path_str
            return ["wsl", wsl_path]
        return [str(executable)]

    def _get_resource_limiter(self, memory_divisor=1):
        """Get resource limiting function for Unix systems"""
        if platform.system() == "Windows" or self.memory_limit_gb is None:
            return None

        try:
            import resource

            def limit_resources():
                mem_bytes = int(
                    (self.memory_limit_gb * 1024 * 1024 * 1024) / memory_divisor  # pyright: ignore[reportOptionalOperand]
                )
                resource.setrlimit(resource.RLIMIT_AS, (mem_bytes, mem_bytes))

            return limit_resources
        except ImportError:
            logger.warning("resource module not available, memory limiting disabled")
            return None

    def _parse_algorithm_output(self, output_str, algo_name, elapsed_us):
        """Parse algorithm output into structured result"""
        if not output_str:
            logger.warning(f"No output from {algo_name}")
            return None

        lines = output_str.strip().split("\n")

        try:
            max_value = int(lines[0].strip())
        except (ValueError, IndexError) as e:
            logger.error(f"Unexpected output format from {algo_name}: {e}")
            return None

        # Parse optional fields with defaults
        num_selected = 0
        selected_items = []
        execution_time = elapsed_us
        memory_used = 0

        if len(lines) > 1:
            try:
                num_selected = int(lines[1].strip())
            except (ValueError, IndexError):
                pass

        if num_selected > 0 and len(lines) > 2:
            try:
                selected_items = list(map(int, lines[2].split()))
            except (ValueError, IndexError):
                pass

        if len(lines) > 3:
            try:
                execution_time = int(lines[3])
            except (ValueError, IndexError):
                pass

        if len(lines) > 4:
            try:
                memory_used = int(lines[4])
            except (ValueError, IndexError):
                pass

        return {
            "max_value": max_value,
            "selected_items": selected_items,
            "execution_time": execution_time,
            "memory_used": memory_used,
            "success": True,
        }

    def load_dataset(self, dataset_name, categories=None):
        """Load knapsack dataset from individual text files in a directory.

        Directory structure: /data/<datasetname>/<category>/<testid>.txt

        Only reads the first 2 lines of each file (metadata), NOT the item data.
        The C++ algorithms read the full file themselves.

        Args:
            dataset_name: Name of the dataset directory under data/ (e.g., 'knapsack_easy')
            categories: Optional list of category names to include. If None, includes all.

        Returns:
            Tuple of (DataFrame with test cases, total count)
        """
        dataset_dir = self.data_path / dataset_name
        if not dataset_dir.exists():
            logger.error(f"Dataset directory not found: {dataset_dir}")
            return None, 0

        data = []

        # Find all category subdirectories
        all_category_dirs = sorted([d for d in dataset_dir.iterdir() if d.is_dir()])

        if not all_category_dirs:
            logger.warning(f"No category directories found in: {dataset_dir}")
            return pd.DataFrame(), 0

        # Filter to requested categories if specified
        if categories is not None:
            category_dirs = [d for d in all_category_dirs if d.name in categories]
            if not category_dirs:
                logger.warning(
                    f"No matching categories found. Requested: {categories}, "
                    f"Available: {[d.name for d in all_category_dirs]}"
                )
                return pd.DataFrame(), 0
            logger.info(f"Filtering to categories: {[d.name for d in category_dirs]}")
        else:
            category_dirs = all_category_dirs

        for category_dir in category_dirs:
            category = category_dir.name

            # Find all .txt files in this category
            test_files = sorted(
                [f for f in category_dir.glob("*.txt") if f.name != "metadata.txt"],
                key=lambda x: int(x.stem),  # Sort by numeric filename
            )

            for test_file in test_files:
                try:
                    # Only read the first 2 lines - NOT the entire file!
                    with open(test_file, "r") as f:
                        line1 = f.readline()
                        line2 = f.readline()

                    # Parse first line: n capacity max_weight min_weight [optimum_value]
                    first_line = line1.strip().split()
                    n = int(first_line[0])
                    capacity = int(first_line[1])
                    max_weight = int(first_line[2]) if len(first_line) > 2 else 0
                    min_weight = int(first_line[3]) if len(first_line) > 3 else 0
                    best_price = int(first_line[4]) if len(first_line) > 4 else None

                    # Parse second line: optimal picks (may be empty)
                    best_picks = []
                    if line2.strip():
                        best_picks = list(map(int, line2.strip().split()))

                    data.append(
                        {
                            "test_id": f"{category}_{test_file.stem}",
                            "category": category,
                            "filepath": str(test_file),
                            "n": n,
                            "capacity": capacity,
                            "max_weight": max_weight,
                            "min_weight": min_weight,
                            "best_picks": best_picks if best_picks else None,
                            "best_price": best_price,
                        }
                    )
                except Exception as e:
                    logger.error(f"Failed to parse {test_file}: {e}")
                    continue

        if not data:
            logger.warning(f"No valid data loaded from: {dataset_dir}")
            return pd.DataFrame(), 0

        df = pd.DataFrame(data)
        df.set_index("test_id", inplace=True)

        return df, len(data)

    def run_algorithm(
        self,
        algo_name,
        filepath,
        n,
        capacity,
        custom_timeout=None,
        memory_divisor=1,
    ):
        """Run a specific algorithm with given inputs via filepath"""
        if algo_name not in self.algorithms:
            raise ValueError(f"Algorithm {algo_name} not found")

        # Get executable and build command with filepath argument
        executable = self._get_executable_path(algo_name)
        cmd = self._build_command(executable)
        cmd.append(filepath)

        # Calculate timeout
        timeout_seconds = self._calculate_timeout(n, custom_timeout)

        # Get resource limiter if applicable
        preexec_fn = self._get_resource_limiter(memory_divisor)

        try:
            # Run and measure real elapsed time
            start = time.perf_counter()
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout_seconds,
                preexec_fn=preexec_fn,
            )
            end = time.perf_counter()
            elapsed_us = int((end - start) * 1_000_000)

            if result.returncode != 0:
                self._log_multiline_error(f"Error running {algo_name}:", result.stderr)
                return None

            # Parse and return output
            return self._parse_algorithm_output(result.stdout, algo_name, elapsed_us)

        except subprocess.TimeoutExpired:
            logger.warning(f"Algorithm {algo_name} timed out (>{timeout_seconds:.1f}s)")
            return None
        except Exception as e:
            self._log_multiline_error(f"Error running {algo_name}:", str(e))
            return None

    async def run_algorithm_async(
        self,
        algo_name,
        filepath,
        n,
        capacity,
        custom_timeout=None,
        memory_divisor=1,
    ):
        """Run a specific algorithm asynchronously with given inputs via filepath"""
        if algo_name not in self.algorithms:
            raise ValueError(f"Algorithm {algo_name} not found")

        # Get executable and build command with filepath argument
        executable = self._get_executable_path(algo_name)
        cmd = self._build_command(executable)
        cmd.append(filepath)

        # Calculate timeout
        timeout_seconds = self._calculate_timeout(n, custom_timeout)

        # Get resource limiter if applicable
        preexec_fn = self._get_resource_limiter(memory_divisor)

        proc = None
        try:
            start = time.perf_counter()
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                preexec_fn=preexec_fn,
            )

            try:
                stdout, stderr = await asyncio.wait_for(
                    proc.communicate(), timeout=timeout_seconds
                )
                end = time.perf_counter()
                elapsed_us = int((end - start) * 1_000_000)

                if proc.returncode != 0:
                    self._log_multiline_error(
                        f"Error running {algo_name}:", stderr.decode()
                    )
                    return None

                # Parse and return output
                return self._parse_algorithm_output(
                    stdout.decode(), algo_name, elapsed_us
                )

            except asyncio.TimeoutError:
                await self._cleanup_process(proc)
                logger.warning(
                    f"Algorithm {algo_name} timed out (>{timeout_seconds:.1f}s)"
                )
                return None
        except Exception as e:
            await self._cleanup_process(proc)
            self._log_multiline_error(f"Error running {algo_name}:", str(e))
            return None
        finally:
            # Ensure process is properly cleaned up
            await self._cleanup_process(proc)

    async def _cleanup_process(self, proc):
        """Clean up an async subprocess"""
        if proc and proc.returncode is None:
            try:
                proc.terminate()
                await asyncio.wait_for(proc.wait(), timeout=0.5)
            except (ProcessLookupError, asyncio.TimeoutError):
                try:
                    proc.kill()
                    await proc.wait()
                except ProcessLookupError:
                    pass

    def _initialize_results_dataframe(self, df):
        """Initialize results dataframe with all necessary columns"""
        results_df = df.copy()

        # Batch initialize numeric columns with numpy for better performance
        numeric_columns = []
        for algo_name in self.algorithms:
            numeric_columns.extend(
                [
                    f"{algo_name}_value",
                    f"{algo_name}_time",
                    f"{algo_name}_memory",
                    f"{algo_name}_accuracy",
                ]
            )

        # Initialize all numeric columns at once
        results_df[numeric_columns] = np.nan

        # Initialize object and boolean columns
        for algo_name in self.algorithms:
            results_df[f"{algo_name}_items"] = pd.Series(
                [None] * len(results_df), index=results_df.index, dtype=object
            )
            results_df[f"{algo_name}_optimal"] = False
            results_df[f"{algo_name}_exceeds_optimum"] = False

        # Add computed_optimum column to track the reference optimum
        results_df["computed_optimum"] = np.nan

        return results_df

    def _cleanup_event_loop(self, loop):
        """Properly cleanup an event loop"""
        try:
            # Cancel all pending tasks
            pending = asyncio.all_tasks(loop)
            for task in pending:
                task.cancel()
            # Wait for all tasks to complete with a timeout
            if pending:
                loop.run_until_complete(
                    asyncio.gather(*pending, return_exceptions=True)
                )
        except Exception:
            pass
        finally:
            loop.close()

    def simulate_all(
        self,
        dataset_name,
        max_parallel,
        categories=None,
        custom_timeout=None,
        memory_limit_gb=None,
    ):
        """Run all algorithms on the dataset

        Args:
            dataset_name: Name of the dataset directory under data/
            max_parallel: Maximum number of parallel tasks
            categories: Optional list of category names to include. If None, includes all.
            custom_timeout: Optional timeout in seconds (overrides adaptive timeout)
            memory_limit_gb: Optional memory limit in GB
        """
        # Set memory limit for this run
        self.memory_limit_gb = memory_limit_gb

        logger.info(f"Loading dataset from {dataset_name}...")
        df, _ = self.load_dataset(dataset_name, categories=categories)
        if df is None or df.empty:
            logger.error(f"No valid data for dataset {dataset_name}")
            return None

        logger.info(f"Found {len(df)} test cases.")

        # Set parallel tasks for this run
        self.max_parallel_tasks = max_parallel

        # Initialize results dataframe with all necessary columns
        results_df = self._initialize_results_dataframe(df)

        # Pre-calculate the run order for each algorithm
        run_orders = self._prepare_run_order(df)

        # Calculate memory divisor for parallel execution
        memory_divisor = max(1, max_parallel)  # Ensure at least 1
        if self.memory_limit_gb:
            logger.info(
                f"Parallel execution: {max_parallel} tasks, "
                f"memory limit per task: {self.memory_limit_gb / memory_divisor:.2f}GB"
            )

        # Run each algorithm independently
        for algo_name, sorted_indices in run_orders.items():
            algo_display_name = self.algorithms[algo_name]["name"]
            logger.info("=" * 60)
            logger.info(f"Running Algorithm: {algo_display_name}")
            logger.info("=" * 60)

            # Run with parallel execution
            try:
                # Create a new event loop for each algorithm run to avoid cleanup issues
                loop = asyncio.new_event_loop()
                asyncio.set_event_loop(loop)
                try:
                    loop.run_until_complete(
                        self._run_algorithm_parallel(
                            algo_name,
                            sorted_indices,
                            df,
                            results_df,
                            custom_timeout,
                            memory_divisor,
                            max_parallel,
                        )
                    )
                finally:
                    self._cleanup_event_loop(loop)
            except Exception as e:
                logger.error(f"Failed to run {algo_display_name}: {e}")
                continue

        # Compute optimum and calculate accuracy/optimality for all test cases
        logger.info("=" * 60)
        logger.info("Computing accuracy and optimality metrics...")
        logger.info("=" * 60)
        self._compute_accuracy_and_optimality(results_df)

        # Save results
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        results_file = self.results_path / f"results_{dataset_name}_{timestamp}.csv"
        results_df.to_csv(results_file, index=False)
        logger.info(f"Results for {dataset_name} saved to {results_file}")

        return results_df

    def create_visualisations(self, results_df, category="Tiny"):
        """Create comprehensive visualisations"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        viz_dir = self.visualisation_path / f"{category}_{timestamp}"
        viz_dir.mkdir(exist_ok=True)

        algo_list = list(self.algorithms.keys())

        # 1. Execution Time vs Problem Size
        self._plot_time_vs_size(results_df, algo_list, viz_dir)
        # 1b. Execution Time vs Knapsack Capacity
        self._plot_time_vs_capacity(results_df, algo_list, viz_dir)

        # 2. Solution Quality (Accuracy)
        self._plot_accuracy(results_df, algo_list, viz_dir)

        # 3. Memory Usage
        self._plot_memory(results_df, algo_list, viz_dir)

        # 4. Quality vs Time (Pareto Plot)
        self._plot_quality_vs_time(results_df, algo_list, viz_dir)

        # 5. Summary Statistics Table
        self._create_summary_table(results_df, algo_list, viz_dir)

        # 6. Optimality Rate
        self._plot_optimality_rate(results_df, algo_list, viz_dir)

        logger.info(f"Visualisations saved to {viz_dir}")

    def _plot_time_vs_size(self, df, algorithms, viz_dir):
        """Plot execution time vs problem size"""
        plt.figure(figsize=(12, 8))

        for algo in algorithms:
            time_col = f"{algo}_time"
            exceeds_col = f"{algo}_exceeds_optimum"
            if time_col not in df.columns:
                continue

            # Use vectorized operations for filtering
            time_valid = df[time_col].notna()
            exceeds_exists = exceeds_col in df.columns
            exceeds_mask = (
                df[exceeds_col] if exceeds_exists else pd.Series(False, index=df.index)
            )

            df_normal = df[time_valid & ~exceeds_mask]
            df_exceeds = (
                df[time_valid & exceeds_mask] if exceeds_exists else pd.DataFrame()
            )

            # Plot normal cases with circles
            if not df_normal.empty:
                plt.scatter(
                    df_normal["n"],
                    df_normal[time_col] / 1000.0,
                    label=self.algorithms[algo]["name"],
                    s=60,
                    alpha=0.7,
                    marker="o",
                )

            # Plot exceeds-optimum cases with red X markers
            if not df_exceeds.empty:
                plt.scatter(
                    df_exceeds["n"],
                    df_exceeds[time_col] / 1000.0,
                    s=100,
                    alpha=0.9,
                    marker="x",
                    color="red",
                    linewidths=3,
                    label=f"{self.algorithms[algo]['name']} (exceeds optimum!)",
                )

        plt.xlabel("Problem Size (N)", fontsize=12, fontweight="bold")
        plt.ylabel("Execution Time (ms)", fontsize=12, fontweight="bold")
        plt.title("Execution Time vs Problem Size", fontsize=14, fontweight="bold")
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.xscale("log")
        plt.yscale("log")
        plt.tight_layout()
        plt.savefig(viz_dir / "time_vs_size.png", dpi=300, bbox_inches="tight")
        plt.close()

    def _plot_time_vs_capacity(self, df, algorithms, viz_dir):
        """Plot execution time vs knapsack capacity"""
        plt.figure(figsize=(12, 8))

        for algo in algorithms:
            time_col = f"{algo}_time"
            cap_col = "capacity"
            exceeds_col = f"{algo}_exceeds_optimum"
            if time_col not in df.columns or cap_col not in df.columns:
                continue

            # Use vectorized operations for filtering
            valid_mask = df[time_col].notna() & df[cap_col].notna()
            exceeds_exists = exceeds_col in df.columns
            exceeds_mask = (
                df[exceeds_col] if exceeds_exists else pd.Series(False, index=df.index)
            )

            df_normal = df[valid_mask & ~exceeds_mask]
            df_exceeds = (
                df[valid_mask & exceeds_mask] if exceeds_exists else pd.DataFrame()
            )

            # Plot normal cases
            if not df_normal.empty:
                plt.scatter(
                    df_normal[cap_col],
                    df_normal[time_col] / 1000.0,
                    label=self.algorithms[algo]["name"],
                    s=60,
                    alpha=0.7,
                    marker="o",
                )

            # Plot exceeds-optimum cases with red X markers
            if not df_exceeds.empty:
                plt.scatter(
                    df_exceeds[cap_col],
                    df_exceeds[time_col] / 1000.0,
                    s=100,
                    alpha=0.9,
                    marker="x",
                    color="red",
                    linewidths=3,
                    label=f"{self.algorithms[algo]['name']} (exceeds optimum!)",
                )

        plt.xlabel("Knapsack Capacity", fontsize=12, fontweight="bold")
        plt.ylabel("Execution Time (ms)", fontsize=12, fontweight="bold")
        plt.title("Execution Time vs Knapsack Capacity", fontsize=14, fontweight="bold")
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.xscale("log")
        plt.yscale("log")
        plt.tight_layout()
        plt.savefig(viz_dir / "time_vs_capacity.png", dpi=300, bbox_inches="tight")
        plt.close()

    def _plot_accuracy(self, df, algorithms, viz_dir):
        """Plot solution accuracy"""
        plt.figure(figsize=(12, 8))

        accuracies = []
        algo_names = []

        for algo in algorithms:
            acc_col = f"{algo}_accuracy"
            if acc_col in df.columns:
                acc_values = df[acc_col].dropna()
                if len(acc_values) > 0:
                    accuracies.append(acc_values.tolist())
                    algo_names.append(self.algorithms[algo]["name"])

        if accuracies:
            # Calculate dynamic y-axis limits
            all_values = [val for acc_list in accuracies for val in acc_list]
            if all_values:
                min_acc = min(all_values)
                max_acc = max(all_values)
                # Add 5% padding on each side
                range_acc = max_acc - min_acc
                if range_acc < 1:  # If all values are very close
                    y_min = max(0, min_acc - 5)
                    y_max = min(105, max_acc + 5)
                else:
                    padding = range_acc * 0.05
                    y_min = max(0, min_acc - padding)
                    y_max = min(105, max_acc + padding)
            else:
                y_min, y_max = 95, 105

            # Create boxplot with custom styling to show 100% better
            bp = plt.boxplot(
                accuracies,
                labels=algo_names,  # pyright: ignore[reportCallIssue]
                patch_artist=True,
                boxprops=dict(facecolor="lightblue", alpha=0.7),
                medianprops=dict(color="red", linewidth=2),
                whiskerprops=dict(linewidth=1.5),
                capprops=dict(linewidth=1.5),
            )

            # Highlight algorithms at 100% with green boxes
            for i, acc_list in enumerate(accuracies):
                if all(abs(val - 100) < 0.01 for val in acc_list):
                    bp["boxes"][i].set_facecolor("lightgreen")
                    bp["boxes"][i].set_alpha(0.9)

            plt.ylabel("Accuracy (%)", fontsize=12, fontweight="bold")
            plt.title("Solution Quality Distribution", fontsize=14, fontweight="bold")
            plt.grid(True, alpha=0.3, axis="y")
            plt.ylim([y_min, y_max])
            plt.axhline(
                y=100,
                color="darkgreen",
                linestyle="--",
                label="Optimal (100%)",
                linewidth=2,
            )

            # Rotate x-axis labels to prevent overlap
            plt.xticks(rotation=30, ha="right")

            plt.legend()
            plt.tight_layout()
            plt.savefig(
                viz_dir / "accuracy_distribution.png", dpi=300, bbox_inches="tight"
            )
        plt.close()

    def _plot_memory(self, df, algorithms, viz_dir):
        """Plot memory usage with dynamic unit selection"""
        plt.figure(figsize=(12, 8))

        # Collect all memory values to determine appropriate unit - vectorized
        mem_cols = [
            f"{algo}_memory" for algo in algorithms if f"{algo}_memory" in df.columns
        ]
        if mem_cols:
            all_memory_bytes = df[mem_cols].values.flatten()
            all_memory_bytes = all_memory_bytes[~np.isnan(all_memory_bytes)]
        else:
            all_memory_bytes = np.array([])

        # Determine appropriate unit based on max value
        if len(all_memory_bytes) > 0:
            max_memory = all_memory_bytes.max()
            if max_memory < 1024:  # Less than 1 KB
                unit = "Bytes"
                divisor = 1
            elif max_memory < 1024 * 1024:  # Less than 1 MB
                unit = "KB"
                divisor = 1024
            elif max_memory < 1024 * 1024 * 1024:  # Less than 1 GB
                unit = "MB"
                divisor = 1024 * 1024
            else:
                unit = "GB"
                divisor = 1024 * 1024 * 1024
        else:
            unit = "KB"
            divisor = 1024

        for algo in algorithms:
            mem_col = f"{algo}_memory"
            exceeds_col = f"{algo}_exceeds_optimum"
            if mem_col not in df.columns:
                continue

            # Use vectorized operations for filtering
            mem_valid = df[mem_col].notna()
            exceeds_exists = exceeds_col in df.columns
            exceeds_mask = (
                df[exceeds_col] if exceeds_exists else pd.Series(False, index=df.index)
            )

            df_normal = df[mem_valid & ~exceeds_mask]
            df_exceeds = (
                df[mem_valid & exceeds_mask] if exceeds_exists else pd.DataFrame()
            )

            # Plot normal cases
            if not df_normal.empty:
                plt.scatter(
                    df_normal["n"],
                    df_normal[mem_col] / divisor,
                    label=self.algorithms[algo]["name"],
                    s=50,
                    marker="s",
                    alpha=0.7,
                )

            # Plot exceeds-optimum cases with red diamonds
            if not df_exceeds.empty:
                plt.scatter(
                    df_exceeds["n"],
                    df_exceeds[mem_col] / divisor,
                    s=100,
                    marker="D",
                    alpha=0.9,
                    color="red",
                    edgecolors="darkred",
                    linewidths=2,
                    label=f"{self.algorithms[algo]['name']} (exceeds optimum!)",
                )

        plt.xlabel("Problem Size (N)", fontsize=12, fontweight="bold")
        plt.ylabel(f"Memory Usage ({unit})", fontsize=12, fontweight="bold")
        plt.title("Memory Usage vs Problem Size", fontsize=14, fontweight="bold")
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(viz_dir / "memory_usage.png", dpi=300, bbox_inches="tight")
        plt.close()

    def _plot_quality_vs_time(self, df, algorithms, viz_dir):
        """Plot quality vs time (Pareto plot)"""
        plt.figure(figsize=(12, 8))

        for algo in algorithms:
            time_col = f"{algo}_time"
            acc_col = f"{algo}_accuracy"
            exceeds_col = f"{algo}_exceeds_optimum"

            if time_col not in df.columns or acc_col not in df.columns:
                continue

            # Use vectorized operations for filtering
            valid_mask = df[time_col].notna() & df[acc_col].notna()
            exceeds_exists = exceeds_col in df.columns
            exceeds_mask = (
                df[exceeds_col] if exceeds_exists else pd.Series(False, index=df.index)
            )

            df_normal = df[valid_mask & ~exceeds_mask]
            df_exceeds = (
                df[valid_mask & exceeds_mask] if exceeds_exists else pd.DataFrame()
            )

            # Plot normal cases
            if not df_normal.empty:
                plt.scatter(
                    df_normal[time_col] / 1000.0,
                    df_normal[acc_col],
                    label=self.algorithms[algo]["name"],
                    s=100,
                    alpha=0.6,
                    edgecolors="black",
                    linewidth=1.5,
                    marker="o",
                )

            # Plot exceeds-optimum cases with red squares
            if not df_exceeds.empty:
                plt.scatter(
                    df_exceeds[time_col] / 1000.0,
                    df_exceeds[acc_col],
                    s=150,
                    alpha=0.9,
                    marker="s",
                    color="red",
                    edgecolors="darkred",
                    linewidth=2,
                    label=f"{self.algorithms[algo]['name']} (exceeds optimum!)",
                )

        plt.xlabel("Execution Time (ms)", fontsize=12, fontweight="bold")
        plt.ylabel("Solution Quality (%)", fontsize=12, fontweight="bold")
        plt.title(
            "Quality vs Time Trade-off (Pareto Plot)", fontsize=14, fontweight="bold"
        )
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.xscale("log")
        plt.tight_layout()
        plt.savefig(viz_dir / "quality_vs_time.png", dpi=300, bbox_inches="tight")
        plt.close()

    def _plot_optimality_rate(self, df, algorithms, viz_dir):
        """Plot optimality rate for each algorithm"""
        plt.figure(figsize=(12, 6))

        algo_names = []
        optimality_rates = []

        for algo in algorithms:
            opt_col = f"{algo}_optimal"
            if opt_col in df.columns:
                # Ensure there are non-NA values to avoid errors on empty dataframes
                valid_entries = df[opt_col].dropna()
                if not valid_entries.empty:
                    rate = valid_entries.sum() / len(valid_entries) * 100
                else:
                    rate = 0.0
                algo_names.append(self.algorithms[algo]["name"])
                optimality_rates.append(rate)

        if algo_names:
            bars = plt.bar(
                algo_names,
                optimality_rates,
                color="skyblue",
                edgecolor="black",
                linewidth=1.5,
            )
            plt.ylabel("Optimality Rate (%)", fontsize=12, fontweight="bold")
            plt.title(
                "Percentage of Optimal Solutions Found", fontsize=14, fontweight="bold"
            )
            plt.ylim([0, 105])
            plt.grid(True, alpha=0.3, axis="y")

            # Add value labels on bars
            for bar, rate in zip(bars, optimality_rates):
                height = bar.get_height()
                plt.text(
                    bar.get_x() + bar.get_width() / 2.0,
                    height,
                    f"{rate:.1f}%",
                    ha="center",
                    va="bottom",
                    fontweight="bold",
                )

            # Rotate x-axis labels to prevent overlap
            plt.xticks(rotation=30, ha="right")

            plt.tight_layout()
            plt.savefig(viz_dir / "optimality_rate.png", dpi=300, bbox_inches="tight")
        plt.close()

    def _format_time(self, time_us):
        """Format time with appropriate unit"""
        if time_us < 1000:  # Less than 1 ms
            return f"{time_us:.1f} us"  # μs"
        elif time_us < 1_000_000:  # Less than 1 s
            return f"{time_us / 1000:.3f} ms"
        else:
            return f"{time_us / 1_000_000:.3f} s"

    def _format_memory(self, memory_bytes):
        """Format memory with appropriate unit"""
        if memory_bytes < 1024:  # Less than 1 KB
            return f"{memory_bytes:.1f} B"
        elif memory_bytes < 1024 * 1024:  # Less than 1 MB
            return f"{memory_bytes / 1024:.2f} KB"
        elif memory_bytes < 1024 * 1024 * 1024:  # Less than 1 GB
            return f"{memory_bytes / (1024 * 1024):.2f} MB"
        else:
            return f"{memory_bytes / (1024 * 1024 * 1024):.2f} GB"

    def _create_summary_table(self, df, algorithms, viz_dir):
        """Create summary statistics table with dynamic units"""
        summary_data = []

        for algo in algorithms:
            time_col = f"{algo}_time"
            acc_col = f"{algo}_accuracy"
            mem_col = f"{algo}_memory"
            opt_col = f"{algo}_optimal"

            if time_col in df.columns:
                algo_name = self.algorithms[algo]["name"]
                # Use dropna to avoid None/NaN interfering with stats
                times = pd.to_numeric(df[time_col], errors="coerce").dropna()
                avg_time_us = times.mean() if not times.empty else 0
                max_time_us = times.max() if not times.empty else 0
                min_time_us = times.min() if not times.empty else 0
                avg_accuracy = (
                    pd.to_numeric(df[acc_col], errors="coerce").dropna().mean()
                    if acc_col in df.columns
                    else 0
                )
                mems = (
                    pd.to_numeric(df[mem_col], errors="coerce").dropna()
                    if mem_col in df.columns
                    else pd.Series(dtype=float)
                )
                avg_memory_bytes = mems.mean() if not mems.empty else 0
                optimality_rate = (
                    (
                        pd.to_numeric(df[opt_col], errors="coerce").fillna(0).sum()
                        / len(df)
                        * 100
                    )
                    if opt_col in df.columns
                    else 0
                )

                summary_data.append(
                    {
                        "Algorithm": algo_name,
                        "Avg Time": self._format_time(avg_time_us),
                        "Min Time": self._format_time(min_time_us),
                        "Max Time": self._format_time(max_time_us),
                        "Avg Accuracy (%)": f"{avg_accuracy:.2f}",
                        "Avg Memory": self._format_memory(avg_memory_bytes),
                        "Optimality Rate (%)": f"{optimality_rate:.1f}",
                    }
                )

        summary_df = pd.DataFrame(summary_data)

        # Save as CSV
        summary_df.to_csv(viz_dir / "summary_statistics.csv", index=False)

        # Create visual table
        fig, ax = plt.subplots(figsize=(12, len(summary_data) * 0.5 + 1.5))
        ax.axis("tight")
        ax.axis("off")

        table = ax.table(
            cellText=summary_df.values.tolist(),
            colLabels=summary_df.columns.tolist(),
            cellLoc="center",
            loc="center",
            colWidths=[0.2, 0.12, 0.12, 0.12, 0.16, 0.12, 0.16],
        )

        table.auto_set_font_size(False)
        table.set_fontsize(10)
        table.scale(1, 1.5)

        # Style header
        for i in range(len(summary_df.columns)):
            table[(0, i)].set_facecolor("#40466e")
            table[(0, i)].set_text_props(weight="bold", color="white")

        # Alternate row colours
        for i in range(1, len(summary_data) + 1):
            for j in range(len(summary_df.columns)):
                if i % 2 == 0:
                    table[(i, j)].set_facecolor("#f0f0f0")
                else:
                    table[(i, j)].set_facecolor("white")

        plt.title("Summary Statistics", fontsize=14, fontweight="bold", pad=20)
        plt.tight_layout()
        plt.savefig(viz_dir / "summary_table.png", dpi=300, bbox_inches="tight")
        plt.close()

        logger.info("Summary Statistics:")
        for line in summary_df.to_string(index=False).split("\n"):
            logger.info(line)

    def _compute_accuracy_and_optimality(self, results_df):
        """Compute accuracy and optimality after all algorithms have run"""
        over_optimum_cases = []  # Track cases where algo exceeds optimum

        # Pre-compile list of algorithm value columns that exist
        algo_value_cols = {
            algo_name: f"{algo_name}_value"
            for algo_name in self.algorithms
            if f"{algo_name}_value" in results_df.columns
        }

        for idx in results_df.index:
            row = results_df.loc[idx]

            # Determine the reference optimum
            if pd.notna(row.get("best_price")):
                # Use known optimum if available
                reference_optimum = row["best_price"]
            else:
                # Use maximum value returned by any algorithm - vectorized
                algo_values = [
                    row[col] for col in algo_value_cols.values() if pd.notna(row[col])
                ]

                if algo_values:
                    reference_optimum = max(algo_values)
                else:
                    # No algorithms produced results for this test case
                    continue

            # Store the computed optimum
            results_df.at[idx, "computed_optimum"] = reference_optimum

            # Calculate accuracy and optimality for each algorithm
            for algo_name, value_col in algo_value_cols.items():
                if not pd.notna(row[value_col]):
                    continue

                algo_value = row[value_col]

                # Check if algorithm exceeded the optimum
                exceeds_optimum = algo_value > reference_optimum + 1e-9
                if exceeds_optimum:
                    over_optimum_cases.append(
                        {
                            "test_id": idx,
                            "algorithm": self.algorithms[algo_name]["name"],
                            "algo_value": algo_value,
                            "optimum": reference_optimum,
                            "excess": algo_value - reference_optimum,
                            "n": row["n"],
                            "capacity": row["capacity"],
                        }
                    )
                    logger.warning(
                        f"⚠️  Algorithm {self.algorithms[algo_name]['name']} EXCEEDED optimum "
                        f"for test_id={idx}: got {algo_value}, optimum={reference_optimum} "
                        f"(excess: {algo_value - reference_optimum})"
                    )

                # Calculate accuracy
                if reference_optimum == 0:
                    accuracy = 100.0 if algo_value == 0 else 0.0
                else:
                    # Cap accuracy at 100% for display, but preserve the over-optimum flag
                    accuracy = min(100.0, (algo_value / reference_optimum * 100.0))

                # Determine optimality
                is_optimal = abs(algo_value - reference_optimum) < 1e-9

                # Store results
                results_df.at[idx, f"{algo_name}_accuracy"] = accuracy
                results_df.at[idx, f"{algo_name}_optimal"] = is_optimal
                results_df.at[idx, f"{algo_name}_exceeds_optimum"] = exceeds_optimum

        logger.info("Accuracy and optimality metrics computed for all test cases.")

        if over_optimum_cases:
            logger.error("=" * 60)
            logger.error(
                f"⚠️  FOUND {len(over_optimum_cases)} CASES WHERE ALGORITHMS EXCEEDED OPTIMUM"
            )
            logger.error("=" * 60)
            for case in over_optimum_cases:
                logger.error(
                    f"  Test {case['test_id']}: {case['algorithm']} "
                    f"got {case['algo_value']} vs optimum {case['optimum']} "
                    f"(+{case['excess']:.2f}, n={case['n']}, capacity={case['capacity']})"
                )
            logger.error("=" * 60)

    def _prepare_run_order(self, df):
        """Pre-calculates the run order for all algorithms."""
        run_orders = {}
        # Work on a copy to avoid modifying original
        df_temp = df.copy()

        for algo_name, config in self.algorithms.items():
            complexity_col = f"{algo_name}_complexity"

            # Vectorised operation for better performance
            df_temp[complexity_col] = df_temp.apply(
                lambda row: config["sort_key"](row["n"], row["capacity"]), axis=1
            )

            # Store sorted indices
            run_orders[algo_name] = df_temp.sort_values(
                by=complexity_col, ascending=True
            ).index.tolist()

        return run_orders

    async def _run_algorithm_parallel(
        self,
        algo_name,
        sorted_indices,
        df,
        results_df,
        custom_timeout,
        memory_divisor,
        max_parallel,
    ):
        """Run algorithm tests in parallel with batching and failure tracking"""
        algo_display_name = self.algorithms[algo_name]["name"]
        is_millionscale = self.algorithms[algo_name]["millionscale"]
        semaphore = asyncio.Semaphore(max_parallel)

        total_tests = len(sorted_indices)
        failed_test_numbers = set()  # Track which test numbers have failed
        failures_lock = asyncio.Lock()
        test_count = 0
        should_stop = False

        def check_consecutive_failures(test_num):
            """Check if there are 3 consecutive test numbers that failed"""
            # Loop over all 3 windows
            for i in [-1, 0, 1]:
                # Generate the set of three consecutive test numbers and check if subset
                if {test_num + j + i for j in [-1, 0, 1]} <= failed_test_numbers:
                    # Return True if found
                    return True
            # No such window found
            return False

        async def run_single_test(idx, test_num):
            nonlocal should_stop, test_count

            # Check if we should stop before even starting
            if should_stop:
                return None

            async with semaphore:
                # Double-check after acquiring semaphore
                if should_stop:
                    return None

                row = df.loc[idx]

                # Skip non-millionscale algorithms for n > 1e6
                if row["n"] > 1e6 and not is_millionscale:
                    logger.info(
                        f"{test_num}/{total_tests}: SKIPPING test_id={idx}, n={row['n']} "
                        f"(n > 1e6, algorithm is not millionscale)"
                    )
                    return None

                logger.info(
                    f"{test_num}/{total_tests}: test_id={idx}, n={row['n']}, capacity={row['capacity']}"
                )

                result = await self.run_algorithm_async(
                    algo_name,
                    row["filepath"],
                    row["n"],
                    row["capacity"],
                    custom_timeout,
                    memory_divisor,
                )

                async with failures_lock:
                    if result:
                        # Store the raw results without calculating accuracy/optimality yet
                        results_df.at[idx, f"{algo_name}_value"] = result["max_value"]
                        results_df.at[idx, f"{algo_name}_time"] = result[
                            "execution_time"
                        ]
                        results_df.at[idx, f"{algo_name}_memory"] = result[
                            "memory_used"
                        ]
                        results_df.at[idx, f"{algo_name}_items"] = result[
                            "selected_items"
                        ]

                        logger.info(
                            f"  -> {test_num}/{total_tests} Value: {result['max_value']}, "
                            f"Time: {result['execution_time']}us"  # μs"
                        )
                        return True
                    else:
                        logger.warning(
                            f"  -> {test_num}/{total_tests} FAILED (test_id={idx})"
                        )
                        failed_test_numbers.add(test_num)

                        # Check for 3 consecutive failures
                        if check_consecutive_failures(test_num):
                            should_stop = True
                            logger.critical(
                                f"*** Algorithm {algo_display_name} hit 3 consecutive test failures "
                                f"(failed tests: {sorted(failed_test_numbers)}) - stopping ***"
                            )
                        return False

        # Launch all tasks at once - semaphore controls concurrency
        tasks = []
        for i, idx in enumerate(sorted_indices):
            if should_stop:
                break
            task = asyncio.create_task(run_single_test(idx, i + 1))
            tasks.append(task)

        # Process results as they complete
        try:
            for completed_task in asyncio.as_completed(tasks):
                try:
                    _ = await completed_task
                    test_count += 1

                    # If we should stop, cancel all remaining tasks
                    if should_stop:
                        cancelled_count = 0
                        for task in tasks:
                            if not task.done():
                                task.cancel()
                                cancelled_count += 1

                        if cancelled_count > 0:
                            logger.info(f"Cancelled {cancelled_count} pending tasks")

                        # Wait for cancellations to complete with timeout
                        try:
                            await asyncio.wait_for(
                                asyncio.gather(*tasks, return_exceptions=True),
                                timeout=5.0,
                            )
                        except asyncio.TimeoutError:
                            logger.warning("Some tasks did not cancel cleanly")

                        logger.info(
                            f"Skipping remaining {total_tests - test_count} test cases."
                        )
                        break

                except asyncio.CancelledError:
                    continue
                except Exception as e:
                    logger.error(f"Exception in test: {e}")
                    # Exception counts as a failure - we don't know the test_num here
                    # but we can still set should_stop if needed
                    async with failures_lock:
                        # Since we don't have the test_num in this exception handler,
                        # we'll just log the error but not track it for consecutive failures
                        pass
        finally:
            # Ensure all tasks are complete
            await asyncio.gather(*tasks, return_exceptions=True)
            # Give a small delay to allow subprocess cleanup
            await asyncio.sleep(0.1)


def main():
    # Get the base path (parent of simulation folder)
    base_path = Path(__file__).parent.parent

    # Setup logging
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = base_path / "simulation" / "logs"
    log_dir.mkdir(exist_ok=True)  # Ensure log directory exists
    log_file = log_dir / f"simulation_{timestamp}.log"

    # Configure root logger
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)-8s] %(message)s",
        handlers=[logging.FileHandler(log_file), logging.StreamHandler()],
    )

    logger.info("=" * 60)
    logger.info("Knapsack Algorithm Simulation")
    logger.info("=" * 60)

    # --- DEFINE: memory limit (in GB, set to None to disable) ---
    memory_limit_gb = 48.0

    # --- DEFINE: simulation runs ---
    # Format: [dataset_name, max_parallel_tasks, [categories], timeout_seconds]
    # - dataset_name: directory under data/ (required)
    # - max_parallel_tasks: number of parallel tasks (required)
    # - categories: list of category names to include, or None/omit for all (optional)
    # - timeout_seconds: custom timeout, or None/omit for adaptive (optional)
    # Note: memory limit will be split among parallel tasks
    simulation_runs = [
        ["knapsack_hard2", 8, ["H2pisingerlarge"], 4],
        ["knapsack_hard2", 8, ["H2pisingerlowdim"], 4],
    ]

    # Create simulator
    simulator = KnapsackSimulator(base_path)

    for run_config in simulation_runs:
        # Unpack config with defaults
        dataset_name = run_config[0]
        max_parallel = run_config[1]
        categories = run_config[2] if len(run_config) > 2 else None
        timeout = run_config[3] if len(run_config) > 3 else None

        logger.info(f"--- Running simulation for dataset: {dataset_name} ---")
        logger.info(f"Parallel tasks: {max_parallel}")
        if categories:
            logger.info(f"Categories: {categories}")
        if timeout:
            logger.info(f"Using timeout: {timeout}s")

        results_df = simulator.simulate_all(
            dataset_name=dataset_name,
            max_parallel=max_parallel,
            categories=categories,
            custom_timeout=timeout,
            memory_limit_gb=memory_limit_gb,
        )

        # Create visualisations only if we have results
        if results_df is not None and not results_df.empty:
            logger.info("Generating visualisations...")
            simulator.create_visualisations(results_df, category=dataset_name)
        else:
            logger.warning(
                "No results to visualise for dataset %s (check dataset and runs).",
                dataset_name,
            )

    logger.info("=" * 60)
    logger.info("All simulations completed successfully!")
    logger.info("=" * 60)


if __name__ == "__main__":
    main()
