# ArrayDeque

ArrayDeque is a fast, array-backed deque implementation for Python written in
C. It aims to provide high-performance double-ended queue operations similar to
Python’s built-in collections.deque with a straightforward, efficient design.


## Features

- Fast appends and pops at both ends.
- Efficient random access and in-place item assignment.
- Full support for iteration, slicing (via __getitem__ and __setitem__),
  and common deque methods.
- A comprehensive CPython C-extension for optimal performance.
- Includes a detailed benchmark comparing performance with collections.deque.


## Installation

There are two ways to install ArrayDeque.


### Installation via pip

There are pre-built wheels available on PyPI, install it directly:

  pip install arraydeque


### Building from source

Clone the repository and run:

  git clone https://github.com/yourusername/arraydeque.git
  cd arraydeque
  pip install -e .

This will compile the C-extension and install the module into your Python
environment.


## Usage

After installation, you can use ArrayDeque just like a regular Python deque:

  from arraydeque import ArrayDeque
  
  # Create an ArrayDeque instance
  dq = ArrayDeque()
  
  # Append items on the right
  dq.append(10)
  dq.append(20)
  
  # Append items on the left
  dq.appendleft(5)
  
  # Access by index
  print(dq[0])  # -> 5
  
  # Pop elements
  print(dq.pop())     # -> 20
  print(dq.popleft()) # -> 5

ArrayDeque supports the standard deque API including extend, extendleft (which
reverses the order), clear, and iteration.


## Benchmarking

A benchmark script ([benchmark.py](benchmark.py)) is provided to compare the
performance of ArrayDeque against Python's built-in collections.deque. The
benchmark runs tests for operations such as append, appendleft, pop, popleft,
random access, and a mixed workload. Each operation is run several times and
the median is reported to give an accurate performance comparison.

After running the benchmark (see instructions below), a plot (saved as
`plot.png`) is generated that visually compares the two implementations using a
fivethirtyeight style bar chart.

To run the benchmark:

  python benchmark.py

Inspect `plot.png` for a detailed performance comparison.


## Testing

Tests are implemented using the standard `unittest` framework. The test suite
verifies all core functionalities of the ArrayDeque module and ensures that
edge cases such as underflow are handled properly.

To run the tests:

  python test_arraydeque.py

Alternatively, if you’re using [tox](https://tox.readthedocs.io/), just run:

  tox

The project’s CI workflows (GitHub Actions) automatically build and test
ArrayDeque on Ubuntu, macOS, and Windows.


## Continuous Integration

This project uses GitHub Actions to automate testing, linting, and release management.

- [Release Workflow](.github/workflows/release.yml): Builds wheels for Ubuntu, macOS, and Windows, then publishes to PyPI.
- [Test Workflow](.github/workflows/test.yml): Runs tests on multiple Python versions.
- [tox.ini](tox.ini): Configures testing environments and lint/format tasks using [ruff](https://beta.ruff.rs/).


## Development

To set up your development environment:

1. Clone the repository.
2. Create a virtual environment:

     python -m venv env
     source env/bin/activate   # On Unix/macOS
     env\Scripts\activate      # On Windows

3. Install the development dependencies:

     pip install tox

4. To format and lint the code, run:

     tox -e format
     tox -e lint


## License

This project is distributed under the Apache2 License.
