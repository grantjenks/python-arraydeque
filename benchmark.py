#!/usr/bin/env python
"""
benchmark.py

Compare the performance of collections.deque and arraydeque.ArrayDeque
on various operations:
    - append (push right)
    - appendleft (push left)
    - pop (pop right)
    - popleft (pop left)
    - random access (read by index)
    - mixed workload (a random mix of operations)

Each benchmark is run 5 times and the median is taken.
The results are then plotted on a bar chart using matplotlib with the fivethirtyeight style.
The figure size has been set to 12 (width) and saved as "plot.png".
"""

import timeit
import random
import statistics
import matplotlib.pyplot as plt

from collections import deque
from arraydeque import ArrayDeque

# Map names to the constructors of the two data structures.
DATA_STRUCTS = {
    "collections.deque": deque,
    "arraydeque.ArrayDeque": ArrayDeque,
}

# Benchmark parameters:
# We choose counts such that each test takes roughly between 100ms and 1s.
APPEND_COUNT = 1_000_000         # number of appends for push operations
POP_COUNT = 1_000_000            # number of pops (after pre-filling)
RANDOM_ACCESS_COUNT = 100_000    # number of random accesses
RANDOM_ACCESS_SIZE = 100_000     # size of container for random access
MIXED_COUNT = 100_000            # iterations for mixed workload

def bench_append_right(struct, count=APPEND_COUNT):
    def test():
        d = struct()
        for i in range(count):
            d.append(i)
    return test

def bench_append_left(struct, count=APPEND_COUNT):
    def test():
        d = struct()
        for i in range(count):
            d.appendleft(i)
    return test

def bench_pop_right(struct, count=POP_COUNT):
    def test():
        d = struct()
        for i in range(count):
            d.append(i)
        for i in range(count):
            d.pop()
    return test

def bench_pop_left(struct, count=POP_COUNT):
    def test():
        d = struct()
        for i in range(count):
            d.append(i)
        for i in range(count):
            d.popleft()
    return test

def bench_random_access(struct, access_count=RANDOM_ACCESS_COUNT, size=RANDOM_ACCESS_SIZE):
    def test():
        d = struct()
        for i in range(size):
            d.append(i)
        for _ in range(access_count):
            idx = random.randint(0, size - 1)
            _ = d[idx]
    return test

def bench_mixed_workload(struct, count=MIXED_COUNT):
    ops = ("append", "appendleft", "pop", "popleft", "access")
    def test():
        d = struct()
        for i in range(count):
            op = random.choice(ops)
            if op == "append":
                d.append(i)
            elif op == "appendleft":
                d.appendleft(i)
            elif op == "pop":
                if len(d) > 0:
                    d.pop()
            elif op == "popleft":
                if len(d) > 0:
                    d.popleft()
            elif op == "access":
                if len(d) > 0:
                    idx = random.randint(0, len(d) - 1)
                    _ = d[idx]
    return test

def run_benchmark(test_func, repeat=5):
    """
    Run the test function 'repeat' times with timeit.repeat (number=1)
    and return the median time (in seconds).
    """
    times = timeit.repeat(test_func, number=1, repeat=repeat)
    return statistics.median(times)

def main():
    # For reproducibility.
    random.seed(42)

    # Define the list of benchmarks with a name and a function generator.
    benchmarks = [
        ("append_right", bench_append_right),
        ("append_left", bench_append_left),
        ("pop_right", bench_pop_right),
        ("pop_left", bench_pop_left),
        ("random_access", bench_random_access),
        ("mixed_workload", bench_mixed_workload),
    ]

    # Store the median times in a dictionary:
    # results[benchmark_name][data_struct_name] = time in seconds
    results = {name: {} for name, _ in benchmarks}

    print("Running benchmarks (each test is repeated 5 times)...")
    for bench_name, bench_func in benchmarks:
        for ds_name, constructor in DATA_STRUCTS.items():
            # Create the test function using the appropriate constructor.
            # Some benchmarks have extra parameters; they use default parameters here.
            test_func = bench_func(constructor)
            # Run the benchmark and store the median time.
            med_time = run_benchmark(test_func, repeat=5)
            results[bench_name][ds_name] = med_time
            print(f"{bench_name} - {ds_name}: {med_time:.4f} sec")

    # Prepare to plot: one group per benchmark with two bars each.
    benchmarks_names = [name for name, _ in benchmarks]
    ds_names = list(DATA_STRUCTS.keys())

    # Build the data table for plotting.
    # Each row: benchmark name; each column: median time in seconds.
    data = {ds: [] for ds in ds_names}
    for bench in benchmarks_names:
        for ds in ds_names:
            data[ds].append(results[bench][ds])

    # Set the 538 style.
    plt.style.use("fivethirtyeight")

    # Plotting parameters:
    import numpy as np
    x = np.arange(len(benchmarks_names))
    width = 0.35

    # Changed figsize width to 12.
    fig, ax = plt.subplots(figsize=(12, 6))

    rects1 = ax.bar(x - width/2, data[ds_names[0]], width, label=ds_names[0])
    rects2 = ax.bar(x + width/2, data[ds_names[1]], width, label=ds_names[1])

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel("Time (sec)")
    ax.set_title("Benchmark Comparison: collections.deque vs arraydeque.ArrayDeque")
    ax.set_xticks(x)
    ax.set_xticklabels(benchmarks_names, rotation=45)
    ax.legend()

    # Attach a text label above each bar, displaying its height.
    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            ax.annotate(f"{height:.3f}",
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points",
                        ha="center", va="bottom",
                        fontsize=8)  # Reduced font size for annotations

    autolabel(rects1)
    autolabel(rects2)

    fig.tight_layout()
    # Save the plot as plot.png.
    plt.savefig("plot.png")
    plt.show()

if __name__ == '__main__':
    main()
