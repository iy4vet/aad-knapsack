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

# Optimality tolerance and timeout settings
OPTIMALITY_TOLERANCE = 1e-9
DEFAULT_TIMEOUT_PER_ITEM = 0.5
MIN_TIMEOUT = 2.0
MAX_TIMEOUT = 120.0

# Unit thresholds for formatting
KB = 1024
MB = KB * 1024
GB = MB * 1024
MS = 1000
SEC = 1_000_000


class KnapsackSimulator:
    # ==== Initialisation and Configuration ====
    def __init__(self, base_path):
        # Set up paths
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
        # Base timeout used as part of adaptive timeout calculation
        self.base_timeout_seconds = 10.0
        # Memory limit for subprocesses in GB
        self.mem_limit = None
        # Algorithm configurations: Add an entry for each executable placed in algorithms/bin
        self.algorithms = {
            "bruteforce": {
                "executable": self.algorithms_path / "bin" / "bruteforce",
                "name": "Brute Force",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": True,
                "millionscale": False,
            },
            "memoization": {
                "executable": self.algorithms_path / "bin" / "memoization",
                "name": "Memoization",
                "sort_key": lambda n, capacity, wmax, wmin: n * capacity,
                "exact": True,
                "millionscale": False,
            },
            "dynamicprogramming": {
                "executable": self.algorithms_path / "bin" / "dynamicprogramming",
                "name": "Dynamic Programming",
                "sort_key": lambda n, capacity, wmax, wmin: n * capacity,
                "exact": True,
                "millionscale": False,
            },
            "branchandbound": {
                "executable": self.algorithms_path / "bin" / "branchandbound",
                "name": "Branch and Bound",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": False,
            },
            "meetinthemiddle": {
                "executable": self.algorithms_path / "bin" / "meetinthemiddle",
                "name": "Meet in the Middle",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": False,
            },
            "greedyheuristic": {
                "executable": self.algorithms_path / "bin" / "greedyheuristic",
                "name": "Greedy Heuristic",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": True,
            },
            "randompermutation": {
                "executable": self.algorithms_path / "bin" / "randompermutation",
                "name": "Random Permutation",
                "sort_key": lambda n, capacity, wmax, wmin: (n**1.5) * wmax,
                "exact": False,
                "millionscale": False,
            },
            "efficientalgo": {
                "executable": self.algorithms_path / "bin" / "efficientalgo",
                "name": "Efficient Algorithm",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": False,
            },
            "billionscale": {
                "executable": self.algorithms_path / "bin" / "billionscale",
                "name": "Billion Scale",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": True,
            },
            "geneticalgorithm": {
                "executable": self.algorithms_path / "bin" / "geneticalgorithm",
                "name": "Genetic Algorithm",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": True,
            },
            "customalgorithm": {
                "executable": self.algorithms_path / "bin" / "customalgorithm",
                "name": "Custom Algorithm",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": True,
            },
            "customtestbed": {
                "executable": self.algorithms_path / "bin" / "customtestbed",
                "name": "Custom Testbed",
                "sort_key": lambda n, capacity, wmax, wmin: n,
                "exact": False,
                "millionscale": True,
            },
        }

    # ==== Helpers for load_dataset() ====

    # ==== load_dataset() ====

    def load_dataset(self, dataset_name, categories=None):
        """Load knapsack dataset from individual text files in a directory."""
        # Build dataset directory path
        dataset_dir = self.data_path / dataset_name
        # Check if dataset directory exists
        if not dataset_dir.exists():
            logger.error(f"Dataset directory not found: {dataset_dir}")
            return None, 0
        # Find all category subdirectories
        all_category_dirs = sorted([d for d in dataset_dir.iterdir() if d.is_dir()])
        if not all_category_dirs:
            logger.warning(f"No category directories found in: {dataset_dir}")
            return pd.DataFrame(), 0
        # Filter to requested categories if specified
        if categories is not None:
            category_dirs = [d for d in all_category_dirs if d.name in categories]
            # If no matching categories found, return empty DataFrame
            if not category_dirs:
                logger.warning(
                    f"No matching categories found. Requested: {categories}, Available: {[d.name for d in all_category_dirs]}"
                )
                return pd.DataFrame(), 0
            logger.info(f"Filtering to categories: {[d.name for d in category_dirs]}")
        else:
            category_dirs = all_category_dirs
        # Initialise data list
        data = []
        # Load each category directory
        for category_dir in category_dirs:
            category = category_dir.name
            # Get all tests in this category (sorted by numeric filename)
            test_files = sorted(
                [f for f in category_dir.glob("*.txt") if f.name != "metadata.txt"],
                key=lambda x: int(x.stem),
            )
            # Load each test in this category
            for test_file in test_files:
                try:
                    # Read metadata lines
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
                    # Append to data list
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
                # Handle error if this test file cannot be parsed
                except Exception as e:
                    logger.error(f"Failed to parse {test_file}: {e}")
                    continue
        # Check if any data was loaded
        if not data:
            logger.warning(f"No valid data loaded from: {dataset_dir}")
            return pd.DataFrame(), 0
        # Create DataFrame
        df = pd.DataFrame(data)
        df.set_index("test_id", inplace=True)
        return df, len(data)

    # ==== Helpers for run_algorithm_async() ====

    def _log_multiline_error(self, message, error_text):
        """Helper to log multi-line error messages"""
        # Log main message
        logger.error(message)
        # Log each line of the error text
        for line in error_text.split("\n"):
            if line.strip():
                logger.error(f"\t{line.strip()}")

    def _calculate_timeout(self, n, custom_timeout=None):
        """Calculate adaptive timeout based on problem size"""
        # Use custom timeout if provided
        if custom_timeout is not None:
            return float(custom_timeout)
        # Adaptive timeout calculation
        else:
            timeout = self.base_timeout_seconds + DEFAULT_TIMEOUT_PER_ITEM * float(n)
            return max(MIN_TIMEOUT, min(MAX_TIMEOUT, timeout))

    def _get_executable_path(self, algo_name):
        """Get the executable path for an algorithm, handling Windows .exe extension"""
        # Get executable path
        executable = self.algorithms[algo_name]["executable"]
        # Handle Windows .exe extension
        if platform.system() == "Windows" and not executable.exists():
            exe_path = executable.with_suffix(".exe")
            if exe_path.exists():
                executable = exe_path
        # Check if executable exists
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
        else:
            return [str(executable)]

    def _get_resource_limiter(self, mem_divisor=1):
        """Get resource limiting function for Unix systems"""
        # No limiting on Windows or if no memory limit set
        if platform.system() == "Windows" or self.mem_limit is None:
            return None
        # Set memory limit using resource module
        try:
            import resource

            # Define the limiting function
            def limit_resources():
                mem_bytes = int((self.mem_limit * 2**30) / mem_divisor)  # pyright: ignore[reportOptionalOperand]
                resource.setrlimit(resource.RLIMIT_AS, (mem_bytes, mem_bytes))

            return limit_resources
        # Fallback if resource module not available
        except ImportError:
            logger.warning("resource module not available, memory limiting disabled")
            return None

    def _parse_algorithm_output(self, output_str, algo_name, elapsed_us):
        """Parse algorithm output into structured result"""
        # Check for empty output
        if not output_str:
            logger.warning(f"No output from {algo_name}")
            return None
        # Parse output lines
        lines = output_str.strip().split("\n")
        try:
            max_value = int(lines[0].strip())
        except (ValueError, IndexError) as e:
            logger.error(f"Unexpected output format from {algo_name}: {e}")
            return None
        # Init default values
        num_selected = 0
        selected_items = []
        execution_time = elapsed_us
        memory_used = 0
        # Parse additional lines if present
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
        # Return structured result
        return {
            "max_value": max_value,
            "selected_items": selected_items,
            "execution_time": execution_time,
            "memory_used": memory_used,
            "success": True,
        }

    async def _cleanup_process(self, proc):
        """Clean up an async subprocess"""
        # Attempt to terminate the process if still running
        if proc and proc.returncode is None:
            try:
                proc.terminate()
                await asyncio.wait_for(proc.wait(), timeout=0.5)
            # If timed out, attempt to kill the process
            except asyncio.TimeoutError:
                try:
                    proc.kill()
                    await proc.wait()
                # Handle if process already exited
                except ProcessLookupError:
                    pass
                except Exception as e:
                    logger.error(f"Error killing process: {e}")
            # Handle if process already exited
            except ProcessLookupError:
                pass
            except Exception as e:
                logger.error(f"Error cleaning up process: {e}")

    # ==== run_algorithm_async() ====

    async def run_algorithm_async(self, name, filepath, n, timeout=None, mem_divisor=1):
        """Run a specific algorithm asynchronously with given inputs via filepath"""
        # Check if algorithm exists
        if name not in self.algorithms:
            raise ValueError(f"Algorithm {name} not found")
        # Get executable and build command with filepath argument
        executable = self._get_executable_path(name)
        cmd = self._build_command(executable)
        cmd.append(filepath)
        # Calculate timeout and get resource limiter
        timeout_seconds = self._calculate_timeout(n, timeout)
        preexec_fn = self._get_resource_limiter(mem_divisor)
        # Run subprocess asynchronously
        proc = None
        try:
            # Run and measure real elapsed time
            start = time.perf_counter()
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                preexec_fn=preexec_fn,
            )
            # Wait for process to complete with timeout
            try:
                stdout, stderr = await asyncio.wait_for(
                    proc.communicate(), timeout=timeout_seconds
                )
                end = time.perf_counter()
                elapsed_us = int((end - start) * 1_000_000)
                # Check return code
                if proc.returncode != 0:
                    self._log_multiline_error(f"Error running {name}:", stderr.decode())
                    return None
                # Parse and return output
                return self._parse_algorithm_output(stdout.decode(), name, elapsed_us)
            # Handle timeout
            except asyncio.TimeoutError:
                await self._cleanup_process(proc)
                logger.warning(f"Algorithm {name} timed out (>{timeout_seconds:.1f}s)")
                return None
        # Handle other exceptions
        except Exception as e:
            await self._cleanup_process(proc)
            self._log_multiline_error(f"Error running {name}:", str(e))
            return None
        # Ensure cleanup in case of unexpected exit
        finally:
            await self._cleanup_process(proc)

    # ==== Helpers for run_algorithm_parallel() ====

    # ==== run_algorithm_parallel() ====

    async def run_algorithm_parallel(
        self,
        algo_name,
        sorted_indices,
        df,
        results_df,
        timeout_lookup,
        mem_divisor,
        max_parallel,
    ):
        """Run algorithm tests in parallel with batching and failure tracking"""
        # Get algorithm details
        algo_display_name = self.algorithms[algo_name]["name"]
        is_millionscale = self.algorithms[algo_name]["millionscale"]
        # Set up semaphore for limiting parallel tasks
        semaphore = asyncio.Semaphore(max_parallel)
        # Initialise tracking variables
        total_tests = len(sorted_indices)
        failed_test_numbers = set()
        failures_lock = asyncio.Lock()
        test_count = 0
        should_stop = False

        # Helper to check for 3 consecutive failures
        def check_consecutive_failures(test_num):
            """Check if there are 3 consecutive test numbers that failed"""
            for i in [-1, 0, 1]:
                if {test_num + j + i for j in [-1, 0, 1]} <= failed_test_numbers:
                    return True
            return False

        # Define async function to run a single test
        async def run_single_test(idx, test_num):
            # Use non-local instance of should_stop and test_count
            nonlocal should_stop, test_count
            # If should_stop is set, skip further tests
            if should_stop:
                return None
            # Acquire semaphore to limit parallelism
            async with semaphore:
                # Check again if should_stop is set
                if should_stop:
                    return None
                # Get test details
                row = df.loc[idx]
                this_timeout = timeout_lookup(row["category"])
                # Skip non-millionscale algorithms for n > 1e6
                if row["n"] > 1e6 and not is_millionscale:
                    logger.info(
                        f"{test_num}/{total_tests}: SKIPPING test_id={idx}, n={row['n']} (n > 1e6, algorithm is not millionscale)"
                    )
                    return None
                # Log test start
                logger.info(
                    f"{test_num}/{total_tests}: test_id={idx}, n={row['n']}, capacity={row['capacity']}"
                )
                # Run the algorithm asynchronously
                result = await self.run_algorithm_async(
                    algo_name,
                    row["filepath"],
                    row["n"],
                    this_timeout,
                    mem_divisor,
                )
                # Update results dataframe within lock
                async with failures_lock:
                    # If result is valid, store results
                    if result:
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
                            f"  -> {test_num}/{total_tests} Value: {result['max_value']}, Time: {result['execution_time']}us"
                        )
                        return True
                    # If result is None, mark as failed
                    else:
                        logger.warning(
                            f"  -> {test_num}/{total_tests} FAILED (test_id={idx})"
                        )
                        failed_test_numbers.add(test_num)
                        if check_consecutive_failures(test_num):
                            should_stop = True
                            logger.critical(
                                f"*** Algorithm {algo_display_name} hit 3 consecutive test failures (failed tests: {sorted(failed_test_numbers)}) - stopping ***"
                            )
                        return False

        # Run through tests in sorted order
        tasks = []
        for i, idx in enumerate(sorted_indices):
            # Check if should_stop is set
            if should_stop:
                break
            # Create and store task
            tasks.append(asyncio.create_task(run_single_test(idx, i + 1)))
        # Process tasks as they complete
        try:
            for completed_task in asyncio.as_completed(tasks):
                # For each completed task, update test count and check for should_stop
                try:
                    _ = await completed_task
                    test_count += 1
                    # Check if we should stop further tests
                    if should_stop:
                        # Cancel remaining tasks
                        cancelled_count = 0
                        for task in tasks:
                            if not task.done():
                                task.cancel()
                                cancelled_count += 1
                        if cancelled_count > 0:
                            logger.info(f"Cancelled {cancelled_count} pending tasks")
                        # Wait briefly for tasks to cancel
                        try:
                            await asyncio.wait_for(
                                asyncio.gather(*tasks, return_exceptions=True),
                                timeout=5.0,
                            )
                        # Warn if some tasks did not cancel cleanly
                        except asyncio.TimeoutError:
                            logger.warning("Some tasks did not cancel cleanly")
                        # Log skipping remaining tests
                        logger.info(
                            f"Skipping remaining {total_tests - test_count} test cases."
                        )
                        # Exit the loop
                        break
                # Handle task exceptions
                except asyncio.CancelledError:
                    continue
                # Handle other exceptions
                except Exception as e:
                    logger.error(f"Exception in test: {e}")
        # Ensure all tasks are cleaned up
        finally:
            await asyncio.gather(*tasks, return_exceptions=True)
            # Give event loop a moment to settle
            await asyncio.sleep(0.1)

    # ==== Helpers for simulate_all() ====

    def _initialise_results_dataframe(self, df):
        """Initialise results dataframe with all necessary columns"""
        results_df = df.copy()
        # Build all columns using dictionary comprehension
        numeric_cols = {
            f"{algo}_{suffix}": np.nan
            for algo in self.algorithms
            for suffix in ("value", "time", "memory", "accuracy")
        }
        bool_cols = {
            f"{algo}_{suffix}": False
            for algo in self.algorithms
            for suffix in ("optimal", "exceeds_optimum")
        }
        object_cols = {f"{algo}_items": None for algo in self.algorithms}
        # Assign all columns at once
        results_df = results_df.assign(
            **numeric_cols, **bool_cols, **object_cols, computed_optimum=np.nan
        )
        # Convert object columns to proper dtype
        for col in object_cols:
            results_df[col] = results_df[col].astype(object)
        return results_df

    def _prepare_run_order(self, df):
        """Pre-calculates the run order for all algorithms."""
        # Prepare run orders based on complexity
        run_orders = {}
        df_temp = df.copy()
        # Calculate complexity for each algorithm and sort
        for algo_name, config in self.algorithms.items():
            complexity_col = f"{algo_name}_complexity"
            # Calculate complexity using the algorithm's sort_key
            df_temp[complexity_col] = df_temp.apply(
                lambda row: config["sort_key"](
                    row["n"], row["capacity"], row["max_weight"], row["min_weight"]
                ),
                axis=1,
            )
            # Determine run order by sorting on complexity
            run_orders[algo_name] = df_temp.sort_values(
                by=complexity_col, ascending=True
            ).index.tolist()
        return run_orders

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
        # Handle exceptions during cleanup
        except Exception:
            pass
        # Close the loop
        finally:
            loop.close()

    def _compute_accuracy_and_optimality(self, results_df):
        """Compute accuracy and optimality metrics using vectorized operations"""
        logger.info("Computing accuracy and optimality metrics...")
        # 1. Determine Computed Optimum
        results_df["computed_optimum"] = results_df["best_price"]
        exact_cols = [
            f"{algo}_value"
            for algo, config in self.algorithms.items()
            if config["exact"] and f"{algo}_value" in results_df.columns
        ]
        if exact_cols:
            results_df["computed_optimum"] = results_df["computed_optimum"].fillna(
                results_df[exact_cols].max(axis=1)
            )
        all_value_cols = [
            f"{algo}_value"
            for algo in self.algorithms
            if f"{algo}_value" in results_df.columns
        ]
        if all_value_cols:
            results_df["computed_optimum"] = results_df["computed_optimum"].fillna(
                results_df[all_value_cols].max(axis=1)
            )
        # 2. Calculate Metrics for each algorithm
        over_optimum_cases = []
        for algo_name in self.algorithms:
            val_col = f"{algo_name}_value"
            if val_col not in results_df.columns:
                continue
            mask = results_df[val_col].notna() & results_df["computed_optimum"].notna()
            if not mask.any():
                continue
            algo_values = results_df.loc[mask, val_col]
            optimums = results_df.loc[mask, "computed_optimum"]
            # Check for exceeds optimum
            exceeds_mask = algo_values > (optimums + OPTIMALITY_TOLERANCE)
            results_df.loc[mask, f"{algo_name}_exceeds_optimum"] = exceeds_mask
            if exceeds_mask.any():
                for idx in exceeds_mask[exceeds_mask].index:
                    val, opt = algo_values[idx], optimums[idx]
                    over_optimum_cases.append(
                        {
                            "test_id": idx,
                            "algorithm": self.algorithms[algo_name]["name"],
                            "algo_value": val,
                            "optimum": opt,
                            "excess": val - opt,
                            "n": results_df.at[idx, "n"],
                            "capacity": results_df.at[idx, "capacity"],
                        }
                    )
            # Calculate Accuracy
            results_df.loc[mask, f"{algo_name}_accuracy"] = 0.0
            pos_opt_mask = mask & (optimums > 0)
            if pos_opt_mask.any():
                results_df.loc[pos_opt_mask, f"{algo_name}_accuracy"] = (
                    (
                        results_df.loc[pos_opt_mask, val_col]
                        / results_df.loc[pos_opt_mask, "computed_optimum"]
                    )
                    * 100.0
                ).clip(upper=100.0)
            zero_opt_mask = mask & (optimums == 0) & (algo_values == 0)
            if zero_opt_mask.any():
                results_df.loc[zero_opt_mask, f"{algo_name}_accuracy"] = 100.0
            # Optimality
            results_df.loc[mask, f"{algo_name}_optimal"] = (
                algo_values - optimums
            ).abs() < OPTIMALITY_TOLERANCE
        logger.info("Accuracy and optimality metrics computed.")
        if over_optimum_cases:
            logger.error("=" * 60)
            logger.error(
                f"⚠️  FOUND {len(over_optimum_cases)} CASES WHERE ALGORITHMS EXCEEDED OPTIMUM"
            )
            logger.error("=" * 60)
            for case in over_optimum_cases:
                logger.error(
                    f"  Test {case['test_id']}: {case['algorithm']} got {case['algo_value']} vs optimum {case['optimum']} (+{case['excess']:.2f}, n={case['n']}, capacity={case['capacity']})"
                )
            logger.error("=" * 60)

    # ==== simulate_all() ====

    def simulate_all(
        self,
        dataset_name,
        max_parallel,
        categories=None,
        custom_timeout=None,
        mem_limit=None,
    ):
        """Run all algorithms on the dataset"""
        # Set memory limit
        self.mem_limit = mem_limit
        # Validate and prepare timeout lookup
        timeout_lookup = None
        # If custom_timeout is a list, it must match categories
        if isinstance(custom_timeout, list):
            if categories is None or not isinstance(categories, list):
                raise ValueError(
                    "If custom_timeout is a list, categories must be specified as a list."
                )
            if len(custom_timeout) != len(categories):
                raise ValueError(
                    f"Length of custom_timeout list ({len(custom_timeout)}) must match length of categories list ({len(categories)})."
                )
            # Create mapping from category to timeout
            timeout_map = dict(zip(categories, [float(t) for t in custom_timeout]))

            # Define lookup function
            def _lookup_map(cat):
                return timeout_map.get(cat)

            timeout_lookup = _lookup_map
        # If custom_timeout is a single value, use it for all
        elif custom_timeout is not None:

            def _lookup_const(cat):
                return float(custom_timeout)

            timeout_lookup = _lookup_const
        # If no custom_timeout, return None for all
        else:

            def _lookup_none(cat):
                return None

            timeout_lookup = _lookup_none
        # Load dataset
        logger.info(f"Loading dataset from {dataset_name}...")
        df, _ = self.load_dataset(dataset_name, categories=categories)
        # Check if data is available
        if df is None or df.empty:
            logger.error(f"No valid data for dataset {dataset_name}")
            return None
        # Log number of test cases
        logger.info(f"Found {len(df)} test cases.")
        # Set max parallel tasks
        self.max_parallel_tasks = max_parallel
        # Calculate memory divisor for parallel execution
        mem_divisor = max(1, max_parallel)
        if self.mem_limit:
            logger.info(
                f"Parallel execution: {max_parallel} tasks, memory limit per task: {self.mem_limit / mem_divisor:.2f}GB"
            )
        # Initialise results dataframe
        results_df = self._initialise_results_dataframe(df)
        # Prepare run orders based on algorithm complexity
        run_orders = self._prepare_run_order(df)
        # Run each algorithm independently
        for algo_name, sorted_indices in run_orders.items():
            # Display algorithm header
            algo_display_name = self.algorithms[algo_name]["name"]
            logger.info("=" * 60)
            logger.info(f"Running Algorithm: {algo_display_name}")
            logger.info("=" * 60)
            # Attempt to run algorithm
            try:
                # Create a new event loop for each algorithm run to avoid cleanup issues
                loop = asyncio.new_event_loop()
                asyncio.set_event_loop(loop)
                try:
                    loop.run_until_complete(
                        self.run_algorithm_parallel(
                            algo_name,
                            sorted_indices,
                            df,
                            results_df,
                            timeout_lookup,
                            mem_divisor,
                            max_parallel,
                        )
                    )
                # Ensure proper cleanup of the event loop
                finally:
                    self._cleanup_event_loop(loop)
            # Handle exceptions during algorithm run
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
        output_name = (
            f"{dataset_name}_{'_'.join(categories)}" if categories else dataset_name
        )
        results_file = self.results_path / f"results_{output_name}_{timestamp}.csv"
        results_df.to_csv(results_file, index=False)
        logger.info(f"Results for {output_name} saved to {results_file}")
        # Return results dataframe and output name
        return results_df, output_name

    # ==== Helpers for Visualisation ====

    @staticmethod
    def _format_with_unit(value, units):
        """Generic formatter for values with dynamic unit selection."""
        for threshold, divisor, suffix, precision in units:
            if value < threshold:
                return f"{value / divisor:.{precision}f} {suffix}"
        _, divisor, suffix, precision = units[-1]
        return f"{value / divisor:.{precision}f} {suffix}"

    def _format_time(self, time_us):
        """Format time with appropriate unit"""
        units = [(MS, 1, "us", 1), (SEC, MS, "ms", 3), (float("inf"), SEC, "s", 3)]
        return self._format_with_unit(time_us, units)

    def _format_memory(self, memory_bytes):
        """Format memory with appropriate unit"""
        units = [
            (KB, 1, "B", 1),
            (MB, KB, "KB", 2),
            (GB, MB, "MB", 2),
            (float("inf"), GB, "GB", 2),
        ]
        return self._format_with_unit(memory_bytes, units)

    def _determine_memory_unit(self, df, algorithms):
        """Determine appropriate memory unit based on max value in data."""
        mem_cols = [
            f"{algo}_memory" for algo in algorithms if f"{algo}_memory" in df.columns
        ]
        if not mem_cols:
            return "KB", KB
        all_memory_bytes = df[mem_cols].values.flatten()
        all_memory_bytes = all_memory_bytes[~np.isnan(all_memory_bytes)]
        if len(all_memory_bytes) == 0:
            return "KB", KB
        max_memory = all_memory_bytes.max()
        if max_memory < KB:
            return "Bytes", 1
        elif max_memory < MB:
            return "KB", KB
        elif max_memory < GB:
            return "MB", MB
        return "GB", GB

    def _get_filtered_data(self, df, algo, value_col, extra_filter_col=None):
        """Get filtered dataframes for normal and exceeds-optimum cases."""
        exceeds_col = f"{algo}_exceeds_optimum"
        valid_mask = df[value_col].notna()
        if extra_filter_col and extra_filter_col in df.columns:
            valid_mask &= df[extra_filter_col].notna()
        exceeds_mask = (
            df[exceeds_col]
            if exceeds_col in df.columns
            else pd.Series(False, index=df.index)
        )
        df_normal = df[valid_mask & ~exceeds_mask]
        df_exceeds = (
            df[valid_mask & exceeds_mask]
            if exceeds_col in df.columns
            else pd.DataFrame()
        )
        return df_normal, df_exceeds

    @staticmethod
    def _configure_plot(ax, xlabel, ylabel, title, use_log_x=False, use_log_y=False):
        """Apply common plot configuration."""
        ax.set_xlabel(xlabel, fontsize=12, fontweight="bold")
        ax.set_ylabel(ylabel, fontsize=12, fontweight="bold")
        ax.set_title(title, fontsize=14, fontweight="bold")
        ax.legend(fontsize=10)
        ax.grid(True, alpha=0.3)
        if use_log_x:
            ax.set_xscale("log")
        if use_log_y:
            ax.set_yscale("log")

    @staticmethod
    def _save_figure(fig, filepath):
        """Save figure and close it properly."""
        fig.tight_layout()
        fig.savefig(filepath, dpi=300, bbox_inches="tight")
        plt.close(fig)

    def _plot_scatter_with_exceeds(
        self,
        ax,
        df_normal,
        df_exceeds,
        x_col,
        y_col,
        y_divisor,
        algo_name,
        normal_marker="o",
        exceeds_marker="x",
        normal_size=60,
        exceeds_size=100,
    ):
        """Plot scatter data with special markers for exceeds-optimum cases."""
        if not df_normal.empty:
            ax.scatter(
                df_normal[x_col],
                df_normal[y_col] / y_divisor,
                label=algo_name,
                s=normal_size,
                alpha=0.7,
                marker=normal_marker,
            )
        if not df_exceeds.empty:
            ax.scatter(
                df_exceeds[x_col],
                df_exceeds[y_col] / y_divisor,
                s=exceeds_size,
                alpha=0.9,
                marker=exceeds_marker,
                color="red",
                linewidths=3,
                label=f"{algo_name} (exceeds optimum!)",
            )

    # ==== Plotting Functions ====

    def _plot_time_vs_size(self, df, algorithms, viz_dir):
        """Plot execution time vs problem size"""
        fig, ax = plt.subplots(figsize=(12, 8))
        try:
            for algo in algorithms:
                time_col = f"{algo}_time"
                if time_col not in df.columns:
                    continue
                df_normal, df_exceeds = self._get_filtered_data(df, algo, time_col)
                self._plot_scatter_with_exceeds(
                    ax,
                    df_normal,
                    df_exceeds,
                    "n",
                    time_col,
                    1000.0,
                    self.algorithms[algo]["name"],
                )
            self._configure_plot(
                ax,
                "Problem Size (N)",
                "Execution Time (ms)",
                "Execution Time vs Problem Size",
                use_log_x=True,
                use_log_y=True,
            )
            self._save_figure(fig, viz_dir / "time_vs_size.png")
        except Exception:
            plt.close(fig)
            raise

    def _plot_time_vs_capacity(self, df, algorithms, viz_dir):
        """Plot execution time vs knapsack capacity"""
        if "capacity" not in df.columns:
            return
        fig, ax = plt.subplots(figsize=(12, 8))
        try:
            for algo in algorithms:
                time_col = f"{algo}_time"
                if time_col not in df.columns:
                    continue
                df_normal, df_exceeds = self._get_filtered_data(
                    df, algo, time_col, extra_filter_col="capacity"
                )
                self._plot_scatter_with_exceeds(
                    ax,
                    df_normal,
                    df_exceeds,
                    "capacity",
                    time_col,
                    1000.0,
                    self.algorithms[algo]["name"],
                )
            self._configure_plot(
                ax,
                "Knapsack Capacity",
                "Execution Time (ms)",
                "Execution Time vs Knapsack Capacity",
                use_log_x=True,
                use_log_y=True,
            )
            self._save_figure(fig, viz_dir / "time_vs_capacity.png")
        except Exception:
            plt.close(fig)
            raise

    def _plot_accuracy(self, df, algorithms, viz_dir):
        """Plot solution accuracy"""
        fig, ax = plt.subplots(figsize=(12, 8))
        try:
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
                all_values = [val for acc_list in accuracies for val in acc_list]
                if all_values:
                    min_acc, max_acc = min(all_values), max(all_values)
                    range_acc = max_acc - min_acc
                    if range_acc < 1:
                        y_min, y_max = max(0, min_acc - 5), min(105, max_acc + 5)
                    else:
                        padding = range_acc * 0.05
                        y_min, y_max = (
                            max(0, min_acc - padding),
                            min(105, max_acc + padding),
                        )
                else:
                    y_min, y_max = 95, 105
                bp = ax.boxplot(
                    accuracies,
                    labels=algo_names,  # pyright: ignore[reportCallIssue]
                    patch_artist=True,
                    boxprops=dict(facecolor="lightblue", alpha=0.7),
                    medianprops=dict(color="red", linewidth=2),
                    whiskerprops=dict(linewidth=1.5),
                    capprops=dict(linewidth=1.5),
                )
                for i, acc_list in enumerate(accuracies):
                    if all(abs(val - 100) < 0.01 for val in acc_list):
                        bp["boxes"][i].set_facecolor("lightgreen")
                        bp["boxes"][i].set_alpha(0.9)
                ax.set_ylabel("Accuracy (%)", fontsize=12, fontweight="bold")
                ax.set_title(
                    "Solution Quality Distribution", fontsize=14, fontweight="bold"
                )
                ax.grid(True, alpha=0.3, axis="y")
                ax.set_ylim(y_min, y_max)
                ax.axhline(
                    y=100,
                    color="darkgreen",
                    linestyle="--",
                    label="Optimal (100%)",
                    linewidth=2,
                )
                ax.tick_params(axis="x", rotation=30)
                for label in ax.get_xticklabels():
                    label.set_horizontalalignment("right")
                ax.legend()
                self._save_figure(fig, viz_dir / "accuracy_distribution.png")
            else:
                plt.close(fig)
        except Exception:
            plt.close(fig)
            raise

    def _plot_memory(self, df, algorithms, viz_dir):
        """Plot memory usage with dynamic unit selection"""
        fig, ax = plt.subplots(figsize=(12, 8))
        try:
            unit, divisor = self._determine_memory_unit(df, algorithms)
            for algo in algorithms:
                mem_col = f"{algo}_memory"
                if mem_col not in df.columns:
                    continue
                df_normal, df_exceeds = self._get_filtered_data(df, algo, mem_col)
                if not df_normal.empty:
                    ax.scatter(
                        df_normal["n"],
                        df_normal[mem_col] / divisor,
                        label=self.algorithms[algo]["name"],
                        s=50,
                        marker="s",
                        alpha=0.7,
                    )
                if not df_exceeds.empty:
                    ax.scatter(
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
            self._configure_plot(
                ax,
                "Problem Size (N)",
                f"Memory Usage ({unit})",
                "Memory Usage vs Problem Size",
            )
            self._save_figure(fig, viz_dir / "memory_usage.png")
        except Exception:
            plt.close(fig)
            raise

    def _plot_quality_vs_time(self, df, algorithms, viz_dir):
        """Plot quality vs time (Pareto plot)"""
        fig, ax = plt.subplots(figsize=(12, 8))
        try:
            for algo in algorithms:
                time_col = f"{algo}_time"
                acc_col = f"{algo}_accuracy"
                if time_col not in df.columns or acc_col not in df.columns:
                    continue
                df_normal, df_exceeds = self._get_filtered_data(
                    df, algo, time_col, extra_filter_col=acc_col
                )
                if not df_normal.empty:
                    ax.scatter(
                        df_normal[time_col] / 1000.0,
                        df_normal[acc_col],
                        label=self.algorithms[algo]["name"],
                        s=100,
                        alpha=0.6,
                        edgecolors="black",
                        linewidth=1.5,
                        marker="o",
                    )
                if not df_exceeds.empty:
                    ax.scatter(
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
            self._configure_plot(
                ax,
                "Execution Time (ms)",
                "Solution Quality (%)",
                "Quality vs Time Trade-off (Pareto Plot)",
                use_log_x=True,
            )
            self._save_figure(fig, viz_dir / "quality_vs_time.png")
        except Exception:
            plt.close(fig)
            raise

    def _plot_optimality_rate(self, df, algorithms, viz_dir):
        """Plot optimality rate for each algorithm"""
        fig, ax = plt.subplots(figsize=(12, 6))
        try:
            algo_names = []
            optimality_rates = []
            for algo in algorithms:
                opt_col = f"{algo}_optimal"
                if opt_col in df.columns:
                    valid_entries = df[opt_col].dropna()
                    if not valid_entries.empty:
                        rate = valid_entries.sum() / len(valid_entries) * 100
                    else:
                        rate = 0.0
                    algo_names.append(self.algorithms[algo]["name"])
                    optimality_rates.append(rate)
            if algo_names:
                bars = ax.bar(
                    algo_names,
                    optimality_rates,
                    color="skyblue",
                    edgecolor="black",
                    linewidth=1.5,
                )
                ax.set_ylabel("Optimality Rate (%)", fontsize=12, fontweight="bold")
                ax.set_title(
                    "Percentage of Optimal Solutions Found",
                    fontsize=14,
                    fontweight="bold",
                )
                ax.set_ylim(0, 105)
                ax.grid(True, alpha=0.3, axis="y")
                for bar, rate in zip(bars, optimality_rates):
                    height = bar.get_height()
                    ax.text(
                        bar.get_x() + bar.get_width() / 2.0,
                        height,
                        f"{rate:.1f}%",
                        ha="center",
                        va="bottom",
                        fontweight="bold",
                    )
                ax.tick_params(axis="x", rotation=30)
                for label in ax.get_xticklabels():
                    label.set_horizontalalignment("right")
                self._save_figure(fig, viz_dir / "optimality_rate.png")
            else:
                plt.close(fig)
        except Exception:
            plt.close(fig)
            raise

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
        summary_df.to_csv(viz_dir / "summary_statistics.csv", index=False)
        fig, ax = plt.subplots(figsize=(12, len(summary_data) * 0.5 + 1.5))
        try:
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
            for i in range(len(summary_df.columns)):
                table[(0, i)].set_facecolor("#40466e")
                table[(0, i)].set_text_props(weight="bold", color="white")
            for i in range(1, len(summary_data) + 1):
                for j in range(len(summary_df.columns)):
                    if i % 2 == 0:
                        table[(i, j)].set_facecolor("#f0f0f0")
                    else:
                        table[(i, j)].set_facecolor("white")
            ax.set_title("Summary Statistics", fontsize=14, fontweight="bold", pad=20)
            self._save_figure(fig, viz_dir / "summary_table.png")
        except Exception:
            plt.close(fig)
            raise
        logger.info("Summary Statistics:")
        for line in summary_df.to_string(index=False).split("\n"):
            logger.info(line)

    # ==== create_visualisations() ====

    def create_visualisations(self, results_df, output_name="Tiny"):
        """Create comprehensive visualisations"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        viz_dir = self.visualisation_path / f"{output_name}_{timestamp}"
        viz_dir.mkdir(exist_ok=True)
        algo_list = list(self.algorithms.keys())
        # Generate all plots
        self._plot_time_vs_size(results_df, algo_list, viz_dir)
        self._plot_time_vs_capacity(results_df, algo_list, viz_dir)
        self._plot_accuracy(results_df, algo_list, viz_dir)
        self._plot_memory(results_df, algo_list, viz_dir)
        self._plot_quality_vs_time(results_df, algo_list, viz_dir)
        self._create_summary_table(results_df, algo_list, viz_dir)
        self._plot_optimality_rate(results_df, algo_list, viz_dir)
        logger.info(f"Visualisations saved to {viz_dir}")


def main():
    """Main function to run simulations."""
    # -- DEFINE: memory limit in GB. set None to disable --
    mem_limit = 48.0

    # -- DEFINE: simulation runs [dataset_name, max_parallel_tasks, [categories], [timeout_seconds]] --
    # ---- [categories]: List of categories to filter dataset (None = all) ----
    # ---- [timeout_seconds]: Single value or list matching categories (None = default adaptive timeouts) ----
    simulation_runs = []
    # Standard datasets
    simulation_runs.extend(
        [
            ["knapsack_standard1", 12, ["S1Known"], 8],
        ]
    )
    # Hard datasets H01 to H16 (Pisinger Codes)
    simulation_runs.extend(
        [
            [
                "knapsack_hard_l012_100",
                12,
                [f"H{i}Tiny", f"H{i}Small", f"H{i}Medium"],
                [4, 12, 30],
            ]
            for i in [f"{j:02d}" for j in range(1, 17)]
        ]
    )

    # Get the base path (parent of simulation folder)
    base_path = Path(__file__).parent.parent

    # Setup logging
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = base_path / "simulation" / "logs"
    log_dir.mkdir(exist_ok=True)
    log_file = log_dir / f"simulation_{timestamp}.log"
    # Configure logging
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)-8s] %(message)s",
        handlers=[logging.FileHandler(log_file), logging.StreamHandler()],
    )

    # Start message
    logger.info("=" * 60)
    logger.info("Knapsack Algorithm Simulation")
    logger.info("=" * 60)

    # Create simulator
    simulator = KnapsackSimulator(base_path)

    # Run simulations
    for run_config in simulation_runs:
        # Unpack run configuration
        dataset_name = run_config[0]
        max_parallel = run_config[1]
        categories = run_config[2] if len(run_config) > 2 else None
        timeout = run_config[3] if len(run_config) > 3 else None
        # Log run details
        logger.info(f"--- Running simulation for dataset: {dataset_name} ---")
        logger.info(f"Parallel tasks: {max_parallel}")
        if categories:
            logger.info(f"Categories: {categories}")
        if timeout:
            logger.info(f"Using timeout: {timeout}s")
        # Run simulation
        result = simulator.simulate_all(
            dataset_name=dataset_name,
            max_parallel=max_parallel,
            categories=categories,
            custom_timeout=timeout,
            mem_limit=mem_limit,
        )
        # Generate visualisations if results are available
        if result is not None:
            results_df, output_name = result
            if results_df is not None and not results_df.empty:
                logger.info("Generating visualisations...")
                simulator.create_visualisations(results_df, output_name=output_name)
            else:
                logger.warning(
                    "No results to visualise for %s (check dataset and runs).",
                    output_name,
                )
        else:
            logger.warning(
                "No results to visualise for dataset %s (check dataset and runs).",
                dataset_name,
            )

    # Completion message
    logger.info("=" * 60)
    logger.info("All simulations completed successfully!")
    logger.info("=" * 60)


if __name__ == "__main__":
    main()
