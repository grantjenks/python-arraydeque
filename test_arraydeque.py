import unittest
import arraydeque
from arraydeque import ArrayDeque
from collections import deque

# A helper list of deque types to test.
DEQUE_TYPES = [
    ('collections.deque', deque),
    ('arraydeque.ArrayDeque', ArrayDeque),
]


class TestArrayDeque(unittest.TestCase):
    def setUp(self):
        # For tests that don't concern maxlen, use the ArrayDeque implementation.
        self.deque = ArrayDeque()

    def test_append_and_pop(self):
        # Test appending items and then popping them from the right.
        self.deque.append(1)
        self.deque.append(2)
        self.deque.append(3)
        self.assertEqual(len(self.deque), 3)

        self.assertEqual(self.deque.pop(), 3)
        self.assertEqual(self.deque.pop(), 2)
        self.assertEqual(self.deque.pop(), 1)
        self.assertEqual(len(self.deque), 0)

        # Popping from an empty deque should raise an IndexError.
        with self.assertRaises(IndexError):
            self.deque.pop()

    def test_appendleft_and_popleft(self):
        # Test appending items to the left and then popleft.
        self.deque.appendleft(1)
        self.deque.appendleft(2)
        self.deque.appendleft(3)
        self.assertEqual(len(self.deque), 3)

        self.assertEqual(self.deque.popleft(), 3)
        self.assertEqual(self.deque.popleft(), 2)
        self.assertEqual(self.deque.popleft(), 1)
        self.assertEqual(len(self.deque), 0)

        # popleft on an empty deque should raise an IndexError.
        with self.assertRaises(IndexError):
            self.deque.popleft()

    def test_extend(self):
        # Test extend on the right.
        self.deque.extend([10, 20, 30])
        self.assertEqual(len(self.deque), 3)
        self.assertEqual(list(self.deque), [10, 20, 30])

    def test_extendleft(self):
        # Test extendleft: note that extendleft reverses the order of the input.
        self.deque.extendleft([10, 20, 30])
        self.assertEqual(len(self.deque), 3)
        self.assertEqual(list(self.deque), [30, 20, 10])

    def test_indexing_and_assignment(self):
        # Test __getitem__ and __setitem__.
        self.deque.extend([100, 200, 300, 400])
        self.assertEqual(self.deque[0], 100)
        self.assertEqual(self.deque[1], 200)
        self.assertEqual(self.deque[2], 300)
        self.assertEqual(self.deque[3], 400)

        # Test negative indices.
        self.assertEqual(self.deque[-1], 400)
        self.assertEqual(self.deque[-2], 300)

        # Test __setitem__.
        self.deque[0] = 111
        self.deque[-1] = 444
        self.assertEqual(self.deque[0], 111)
        self.assertEqual(self.deque[3], 444)

        # Test out-of-range indexing and assignment.
        with self.assertRaises(IndexError):
            _ = self.deque[4]
        with self.assertRaises(IndexError):
            self.deque[-5] = 999

    def test_invalid_index_type_get(self):
        # Test __getitem__ with a non-integer index.
        self.deque.extend([1, 2, 3])
        with self.assertRaises(TypeError):
            _ = self.deque['0']

    def test_invalid_index_type_set(self):
        # Test __setitem__ with a non-integer index.
        self.deque.extend([1, 2, 3])
        with self.assertRaises(TypeError):
            self.deque['1'] = 100

    def test_iteration(self):
        # Test that the deque is iterable.
        items = [1, 2, 3, 4, 5]
        self.deque.extend(items)
        iterated = [item for item in self.deque]
        self.assertEqual(iterated, items)

    def test_contains(self):
        # Test the __contains__ behavior via iteration.
        items = [10, 20, 30]
        self.deque.extend(items)
        for item in items:
            self.assertTrue(item in self.deque)
        self.assertFalse(999 in self.deque)

    def test_clear(self):
        # Test the clear() method.
        self.deque.extend([1, 2, 3, 4])
        self.assertEqual(len(self.deque), 4)
        self.deque.clear()
        self.assertEqual(len(self.deque), 0)
        self.assertEqual(list(self.deque), [])
        # Test that the deque can be reused after clearing.
        self.deque.append(99)
        self.assertEqual(list(self.deque), [99])

    def test_mixed_operations(self):
        # A mix of operations to simulate realistic usage.
        self.deque.append(10)  # deque: [10]
        self.deque.appendleft(20)  # deque: [20, 10]
        self.deque.extend([30, 40])  # deque: [20, 10, 30, 40]
        self.deque.extendleft(
            [50, 60]
        )  # extendleft reverses the order: [60, 50, 20, 10, 30, 40]
        self.assertEqual(list(self.deque), [60, 50, 20, 10, 30, 40])

        # Remove from both ends.
        self.assertEqual(self.deque.popleft(), 60)
        self.assertEqual(self.deque.pop(), 40)
        self.assertEqual(list(self.deque), [50, 20, 10, 30])

    def test_overallocation_growth(self):
        # Push enough items on both ends to force the internal array to resize.
        for i in range(50):
            self.deque.append(i)
        for i in range(50, 100):
            self.deque.appendleft(i)
        # The items added with appendleft will appear in reverse order compared to the input.
        left_side = list(range(99, 49, -1))
        right_side = list(range(50))
        expected = left_side + right_side
        self.assertEqual(list(self.deque), expected)
        self.assertEqual(len(self.deque), 100)

    def test_slice_unsupported(self):
        # collections.deque does not support slicing. Our implementation should also raise TypeError.
        self.deque.extend([1, 2, 3, 4, 5])
        with self.assertRaises(TypeError):
            _ = self.deque[1:3]

    def test_repr_and_str(self):
        # While our type doesn't override __repr__ or __str__, it should at least
        # return a string without error.
        self.deque.extend([1, 2, 3])
        rep = repr(self.deque)
        self.assertTrue(isinstance(rep, str))
        st = str(self.deque)
        self.assertTrue(isinstance(st, str))

    def test_comparison_with_list_and_deque(self):
        # Test that the behavior of ArrayDeque (iteration order, len, indexing)
        # is consistent with that of a list and collections.deque.
        items = list(range(20))
        ad = ArrayDeque()
        cd = deque()
        for item in items:
            ad.append(item)
            cd.append(item)
        self.assertEqual(list(ad), list(cd))
        self.assertEqual(len(ad), len(cd))
        for i in range(len(items)):
            self.assertEqual(ad[i], list(cd)[i])

    def test_initializer_with_iterable(self):
        # Test that the initializer accepts an iterable, similar to collections.deque.
        ad = ArrayDeque([1, 2, 3, 4])
        self.assertEqual(list(ad), [1, 2, 3, 4])
        # Test with an empty iterable.
        ad_empty = ArrayDeque([])
        self.assertEqual(list(ad_empty), [])

    def test_version(self):
        # Version should be set to a valid non-zero version.
        ver_tuple = tuple(map(int, arraydeque.__version__.split('.')))
        self.assertGreater(ver_tuple, (0, 0, 0))


class TestMaxlenBehavior(unittest.TestCase):
    def test_default_maxlen(self):
        # When maxlen is not specified, it should be None.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls()
                self.assertIsNone(getattr(d, 'maxlen', None))

    def test_maxlen_attribute_set_on_init(self):
        # Test that when a maxlen is provided at initialization, the attribute is set.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls([1, 2, 3], maxlen=5)
                self.assertEqual(d.maxlen, 5)

    def test_bounded_append(self):
        # When the deque is bounded and full, appending to the right should discard the leftmost element.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls(maxlen=3)
                d.append(1)
                d.append(2)
                d.append(3)
                self.assertEqual(list(d), [1, 2, 3])
                d.append(4)
                # The oldest element (1) should be removed.
                self.assertEqual(list(d), [2, 3, 4])

    def test_bounded_appendleft(self):
        # When the deque is bounded and full, appending to the left should discard the rightmost element.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls(maxlen=3)
                d.appendleft(1)
                d.appendleft(2)
                d.appendleft(3)
                self.assertEqual(list(d), [3, 2, 1])
                d.appendleft(4)
                # The oldest element on the right (1) should be removed.
                self.assertEqual(list(d), [4, 3, 2])

    def test_initialization_with_too_many_items(self):
        # Initializing with an iterable longer than maxlen should retain only the rightmost items.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls(range(10), maxlen=5)
                # For collections.deque, the leftmost items are discarded.
                self.assertEqual(list(d), [5, 6, 7, 8, 9])

    def test_maxlen_immutability(self):
        # The maxlen attribute should be read-only.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls(maxlen=4)
                with self.assertRaises(AttributeError):
                    d.maxlen = 10

    def test_maxlen_behavior_with_mixed_operations(self):
        # Test that bounded deques behave correctly with a mixture of operations.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls(maxlen=4)
                d.append(1)
                d.append(2)
                d.append(3)
                # Now: [1, 2, 3]
                d.appendleft(0)
                # Now full: [0, 1, 2, 3]
                self.assertEqual(list(d), [0, 1, 2, 3])
                # Append to right; should discard leftmost (0)
                d.append(4)
                self.assertEqual(list(d), [1, 2, 3, 4])
                # Appendleft; should discard rightmost (4)
                d.appendleft(-1)
                self.assertEqual(list(d), [-1, 1, 2, 3])
                # Pop should work normally.
                self.assertEqual(d.pop(), 3)
                self.assertEqual(d.popleft(), -1)
                self.assertEqual(list(d), [1, 2])

    def test_maxlen_zero(self):
        # A deque with maxlen=0 should always remain empty.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                d = cls(maxlen=0)
                # Even if we try to add items, they should be ignored.
                d.append(1)
                d.appendleft(2)
                self.assertEqual(list(d), [])
                # Popping should raise an error.
                with self.assertRaises(IndexError):
                    d.pop()
                with self.assertRaises(IndexError):
                    d.popleft()

    def test_negative_maxlen_raises(self):
        # A negative maxlen should raise a ValueError on initialization.
        for name, cls in DEQUE_TYPES:
            with self.subTest(cls=name):
                with self.assertRaises(ValueError):
                    cls(maxlen=-1)


if __name__ == '__main__':
    unittest.main()
