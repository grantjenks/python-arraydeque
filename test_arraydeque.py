import unittest
import arraydeque
from arraydeque import ArrayDeque


class TestArrayDeque(unittest.TestCase):
    def setUp(self):
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

    def test_iteration(self):
        # Test that the deque is iterable.
        items = [1, 2, 3, 4, 5]
        self.deque.extend(items)
        iterated = [item for item in self.deque]
        self.assertEqual(iterated, items)

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
        )  # extendleft pushes items so that the iterable is reversed;
        # equivalent to: appendleft(50), then appendleft(60)
        # resulting deque: [60, 50, 20, 10, 30, 40]
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

    def test_version(self):
        assert tuple(map(int, arraydeque.__version__.split('.'))) > (0, 0, 0)


if __name__ == '__main__':
    unittest.main()
