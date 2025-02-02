#!/usr/bin/env python
"""
test_arraydeque.py

This file contains a comprehensive test suite for the ArrayDeque type
implemented in arraydeque.c. Its functionality is intended to be compatible
with CPython's collections.deque. The tests below cover basic operations,
bounded (maxlen) behavior, rotations, indexing, iteration, comparisons,
pickling, copying, weak references, and subclassing.

To run the tests:
    python test_arraydeque.py
"""

import unittest
import pickle
import copy
import weakref
import gc

from arraydeque import ArrayDeque
from collections import deque  # for reference comparisons

# A "big" number used in some lengthy tests.
BIG = 100000


# ---------------------------
# Basic Functionality Testing
# ---------------------------
class TestArrayDequeBasic(unittest.TestCase):
    def setUp(self):
        self.d = ArrayDeque()

    def test_append_and_pop(self):
        # Append right and pop from the right.
        self.d.append(1)
        self.d.append(2)
        self.d.append(3)
        self.assertEqual(len(self.d), 3)
        self.assertEqual(self.d.pop(), 3)
        self.assertEqual(self.d.pop(), 2)
        self.assertEqual(self.d.pop(), 1)
        self.assertEqual(len(self.d), 0)
        with self.assertRaises(IndexError):
            self.d.pop()

    def test_appendleft_and_popleft(self):
        # Append left and pop from the left.
        self.d.appendleft(1)
        self.d.appendleft(2)
        self.d.appendleft(3)
        self.assertEqual(len(self.d), 3)
        self.assertEqual(self.d.popleft(), 3)
        self.assertEqual(self.d.popleft(), 2)
        self.assertEqual(self.d.popleft(), 1)
        with self.assertRaises(IndexError):
            self.d.popleft()

    def test_extend(self):
        # Extend on the right.
        self.d.extend([10, 20, 30])
        self.assertEqual(list(self.d), [10, 20, 30])

    def test_extendleft(self):
        # Extend on the left (order reversed relative to the iterable)
        self.d.extendleft([10, 20, 30])
        self.assertEqual(list(self.d), [30, 20, 10])

    def test_clear(self):
        # Test clear method.
        self.d.extend([1, 2, 3, 4])
        self.assertEqual(len(self.d), 4)
        self.d.clear()
        self.assertEqual(len(self.d), 0)
        self.assertEqual(list(self.d), [])
        # Reuse after clear.
        self.d.append(99)
        self.assertEqual(list(self.d), [99])

    def test_iteration(self):
        # Test that the ArrayDeque is iterable.
        items = [1, 2, 3, 4, 5]
        self.d.extend(items)
        self.assertEqual(list(iter(self.d)), items)

    def test_repr_and_str(self):
        # Test that __repr__ and __str__ return strings.
        self.d.extend([1, 2, 3])
        rep = repr(self.d)
        st = str(self.d)
        self.assertIsInstance(rep, str)
        self.assertIsInstance(st, str)

    def test_indexing_and_assignment(self):
        # Test __getitem__ and __setitem__.
        self.d.extend([100, 200, 300, 400])
        self.assertEqual(self.d[0], 100)
        self.assertEqual(self.d[1], 200)
        self.assertEqual(self.d[2], 300)
        self.assertEqual(self.d[3], 400)
        # Test negative indices.
        self.assertEqual(self.d[-1], 400)
        self.assertEqual(self.d[-2], 300)
        # Assignment tests.
        self.d[0] = 111
        self.d[-1] = 444
        self.assertEqual(self.d[0], 111)
        self.assertEqual(self.d[3], 444)
        # Out-of-range access.
        with self.assertRaises(IndexError):
            _ = self.d[4]
        with self.assertRaises(IndexError):
            self.d[-5] = 999

    def test_invalid_index_type(self):
        # Non-integer indices should raise a TypeError.
        self.d.extend([1, 2, 3])
        with self.assertRaises(TypeError):
            _ = self.d['0']
        with self.assertRaises(TypeError):
            self.d['1'] = 100

    def test_slice_unsuported(self):
        # Slicing should raise a TypeError (in line with CPython's deque).
        self.d.extend([1, 2, 3, 4, 5])
        with self.assertRaises(TypeError):
            _ = self.d[1:3]

    def test_contains(self):
        # Test __contains__ behavior.
        self.d.extend([10, 20, 30])
        for item in [10, 20, 30]:
            self.assertTrue(item in self.d)
        self.assertFalse(999 in self.d)

    def test_initializer_with_iterable(self):
        # Initializer should accept an iterable (and optional maxlen).
        d1 = ArrayDeque([1, 2, 3, 4])
        self.assertEqual(list(d1), [1, 2, 3, 4])
        d2 = ArrayDeque([])
        self.assertEqual(list(d2), [])


# ---------------------------
# Rotation Testing
# ---------------------------
class TestArrayDequeRotation(unittest.TestCase):
    def setUp(self):
        self.d = ArrayDeque('abcde')

    def test_rotate_right(self):
        self.d.rotate(1)
        self.assertEqual(list(self.d), list('eabcd'))

    def test_rotate_left(self):
        self.d.rotate(-1)
        self.assertEqual(list(self.d), list('bcdea'))

    def test_rotate_default(self):
        # Default rotate() rotates by 1.
        self.d.rotate()
        self.assertEqual(list(self.d), list('eabcd'))

    def test_rotate_multiple(self):
        original = list(self.d)
        for i in range(10):
            self.d.rotate(i)
            self.d.rotate(-i)
            self.assertEqual(list(self.d), original)

    def test_rotate_empty_deque(self):
        d_empty = ArrayDeque()
        d_empty.rotate(5)
        self.assertEqual(list(d_empty), [])


# ---------------------------
# Maxlen (Bounded) Behavior Testing
# ---------------------------
class TestArrayDequeMaxlen(unittest.TestCase):
    def test_default_maxlen(self):
        d = ArrayDeque()
        # When maxlen is not specified, it behaves as unbounded.
        self.assertEqual(d.maxlen, None)

    def test_set_maxlen(self):
        d = ArrayDeque([1, 2, 3], maxlen=5)
        self.assertEqual(d.maxlen, 5)

    def test_bounded_append(self):
        # Bounded deque: appending to a full deque discards the leftmost element.
        d = ArrayDeque(maxlen=3)
        d.append(1)
        d.append(2)
        d.append(3)
        self.assertEqual(list(d), [1, 2, 3])
        d.append(4)
        self.assertEqual(list(d), [2, 3, 4])

    def test_bounded_appendleft(self):
        # Bounded deque: appending left to a full deque discards the rightmost element.
        d = ArrayDeque(maxlen=3)
        d.appendleft(1)
        d.appendleft(2)
        d.appendleft(3)
        self.assertEqual(list(d), [3, 2, 1])
        d.appendleft(4)
        self.assertEqual(list(d), [4, 3, 2])

    def test_initialization_truncation(self):
        # Initializing with an iterable longer than maxlen should retain only the rightmost items.
        d = ArrayDeque(range(10), maxlen=5)
        self.assertEqual(list(d), list(range(5, 10)))

    def test_maxlen_zero(self):
        # When maxlen==0, the deque should remain empty.
        d = ArrayDeque(maxlen=0)
        d.append(1)
        d.appendleft(2)
        self.assertEqual(list(d), [])
        with self.assertRaises(IndexError):
            d.pop()
        with self.assertRaises(IndexError):
            d.popleft()

    def test_maxlen_readonly(self):
        # The maxlen attribute is read-only.
        d = ArrayDeque('abc', maxlen=3)
        with self.assertRaises(AttributeError):
            d.maxlen = 10


# ---------------------------
# Comparison and Other Methods Testing
# ---------------------------
class TestArrayDequeComparisons(unittest.TestCase):
    def setUp(self):
        self.ad = ArrayDeque('abc')
        self.cd = deque('abc')

    def test_equality(self):
        # ArrayDeque equality compares by iterating, so it should match list(deque) equality.
        self.assertEqual(list(self.ad), list(self.cd))
        d2 = ArrayDeque('abc')
        self.assertEqual(self.ad, d2)
        d3 = ArrayDeque('abcd')
        self.assertNotEqual(self.ad, d3)

    def test_iteration_order(self):
        items = list('abcdef')
        d = ArrayDeque(items)
        self.assertEqual(list(d), items)

    def test_multiplication_unsupported(self):
        # Multiplication is not implemented (unlike list), so it should raise TypeError.
        with self.assertRaises(TypeError):
            _ = self.ad * 2

    def test_delitem_unsupported(self):
        # __delitem__ is not implemented.
        with self.assertRaises(TypeError):
            del self.ad[1]

    def test_insert_unsupported(self):
        # Insert is not implemented by ArrayDeque.
        with self.assertRaises(AttributeError):
            self.ad.insert(1, 'X')

    def test_remove(self):
        # Remove the first appearance of a value.
        d = ArrayDeque('abcbc')
        d.remove('b')
        # "abcbc" becomes ['a','c','b','c'] after removal of the first "b"
        self.assertEqual(list(d), ['a', 'c', 'b', 'c'])
        with self.assertRaises(ValueError):
            d.remove('z')

    def test_count(self):
        d = ArrayDeque('abbccc')
        self.assertEqual(d.count('a'), 1)
        self.assertEqual(d.count('b'), 2)
        self.assertEqual(d.count('c'), 3)
        self.assertEqual(d.count('z'), 0)


# ---------------------------
# Pickling and Copying Testing
# ---------------------------
class TestArrayDequePickleCopy(unittest.TestCase):
    def test_pickle(self):
        d = ArrayDeque(range(10))
        s = pickle.dumps(d, pickle.HIGHEST_PROTOCOL)
        d2 = pickle.loads(s)
        self.assertEqual(list(d), list(d2))
        self.assertEqual(d.maxlen, d2.maxlen)

    def test_deepcopy(self):
        d = ArrayDeque([['a'], ['b']])
        d2 = copy.deepcopy(d)
        self.assertEqual(list(d), list(d2))
        d[0].append('c')
        self.assertNotEqual(d[0], d2[0])

    def test_copy(self):
        d = ArrayDeque(['x', 'y'])
        d2 = copy.copy(d)
        self.assertEqual(list(d), list(d2))
        d.append('z')
        self.assertNotEqual(list(d), list(d2))


# ---------------------------
# Weak Reference Testing
# ---------------------------
class TestArrayDequeWeakref(unittest.TestCase):
    def test_weakref(self):
        d = ArrayDeque('abc')
        ref = weakref.ref(d)
        self.assertIsNotNone(ref())
        del d
        gc.collect()
        self.assertIsNone(ref())


# ---------------------------
# Subclassing Testing
# ---------------------------
class CustomDeque(ArrayDeque):
    pass


class TestArrayDequeSubclass(unittest.TestCase):
    def test_subclass_basic(self):
        d = CustomDeque('abc')
        self.assertEqual(list(d), list('abc'))
        d.append('d')
        self.assertEqual(list(d), list('abcd'))
        # Ensure that conversion back to iterable produces an instance of the subclass.
        d2 = CustomDeque(d)
        self.assertIsInstance(d2, CustomDeque)
        self.assertEqual(list(d2), list(d))


# ---------------------------
# Main: Run all tests
# ---------------------------
if __name__ == '__main__':
    unittest.main()
