# ArrayDeque

ArrayDeque is a fast, array-backed deque implementation for Python written in C. It provides high-performance double-ended queue operations similar to Python’s built-in `collections.deque`, with a straightforward and efficient design.

## Features

- **Fast Operations:** Quick appends and pops at both ends.
- **Random Access:** Efficient in-place item assignment and index-based access.
- **Full API Support:** Implements iteration, slicing (via `__getitem__` and `__setitem__`), and common deque methods.
- **C Extension:** A complete CPython C-extension for optimal speed.
- **Benchmark Included:** Compare performance with Python’s built-in `collections.deque`.

![alt text](https://github.com/grantjenks/python-arraydeque/blob/main/plot.png?raw=true)

## Installation

There are two ways to install ArrayDeque.

### Via PyPI

Pre-built wheels are available on PyPI. Simply run:

```bash
pip install arraydeque
```

### Building from Source

Clone the repository and install in editable mode to compile the C-extension:

```bash
git clone https://github.com/yourusername/arraydeque.git
cd arraydeque
pip install -e .
```

## Usage

Once installed, use ArrayDeque just like the standard deque:

```python
from arraydeque import ArrayDeque

# Create an ArrayDeque instance
dq = ArrayDeque()

# Append items on the right
dq.append(10)
dq.append(20)

# Append items on the left
dq.appendleft(5)

# Access by index
print(dq[0])  # Output: 5

# Pop items
print(dq.pop())     # Output: 20
print(dq.popleft()) # Output: 5
```

ArrayDeque supports the standard deque API including methods like `extend`, `extendleft` (which reverses the input order), `clear`, and iteration.

## Benchmarking

A benchmark script ([benchmark.py](benchmark.py)) is provided to compare the performance of ArrayDeque with `collections.deque`.

The benchmark tests various operations such as append, appendleft, pop, popleft, random access, and a mixed workload. Each operation is run 5 times, with the median time reported.

After running the benchmark with:

```bash
python benchmark.py
```

a plot (`plot.png`) is generated that visually compares the two implementations using a fivethirtyeight-style bar chart.

## Testing

Tests are implemented using Python’s built-in `unittest` framework. Run the test suite with:

```bash
python test_arraydeque.py
```

Alternatively, if you’re using [tox](https://tox.readthedocs.io/), simply run:

```bash
tox
```

## Continuous Integration

This project uses GitHub Actions for continuous integration. It includes three workflows:

- **Release Workflow (`.github/workflows/release.yml`):** Builds wheels for Ubuntu, macOS, and Windows, then publishes to PyPI.
- **Test Workflow (`.github/workflows/test.yml`):** Runs the test suite across multiple Python versions.
- **tox Configuration (`tox.ini`):** Defines test, lint, and formatting environments (using [ruff](https://beta.ruff.rs/)).

## Development

To set up a development environment:

1. **Clone the repository:**

   ```bash
   git clone https://github.com/yourusername/arraydeque.git
   cd arraydeque
   ```

2. **Create a virtual environment:**

   On Unix/macOS:
   ```bash
   python -m venv env
   source env/bin/activate
   ```
   On Windows:
   ```bash
   python -m venv env
   env\Scripts\activate
   ```

3. **Install development dependencies:**

   ```bash
   pip install tox
   ```

4. **Format and lint the code:**

   ```bash
   tox -e format
   tox -e lint
   ```

## License

This project is distributed under the Apache License 2.0.
