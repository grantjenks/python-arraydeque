#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <stddef.h>  /* for offsetof */

#ifndef ARRAYDEQUE_VERSION
#define ARRAYDEQUE_VERSION "1.2.2"
#endif

/* The ArrayDeque object structure.
   A new field weakreflist is added to support weak references */
typedef struct {
    PyObject_HEAD
    PyObject *weakreflist;   /* For weak reference support */
    PyObject **array;        /* Pointer to array of PyObject* */
    Py_ssize_t capacity;     /* Allocated length of array */
    Py_ssize_t size;         /* Number of elements stored */
    Py_ssize_t head;         /* Index of first element */
    Py_ssize_t tail;         /* Index one past the last element */
    Py_ssize_t maxlen;       /* Maximum allowed size (if < 0 then unbounded) */
} ArrayDequeObject;

/* Forward declaration of type for iterator */
typedef struct {
    PyObject_HEAD
    ArrayDequeObject *deque; /* Reference to the deque */
    Py_ssize_t index;        /* Current index into the deque (0 .. size) */
} ArrayDequeIter;

/* Resize the backing array to new_capacity and recenter the data.
   Returns 0 on success and -1 on failure. */
static int
arraydeque_resize(ArrayDequeObject *self, Py_ssize_t new_capacity)
{
    PyObject **new_array;
    Py_ssize_t new_head;
    Py_ssize_t i;

    new_array = PyMem_New(PyObject *, new_capacity);
    if (new_array == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    /* Calculate new head so that the existing items are centered */
    new_head = (new_capacity - self->size) / 2;
    for (i = 0; i < self->size; i++) {
        new_array[new_head + i] = self->array[self->head + i];
    }
    PyMem_Free(self->array);
    self->array = new_array;
    self->capacity = new_capacity;
    self->head = new_head;
    self->tail = new_head + self->size;
    return 0;
}

/* Method: append(x)
   Append an item to the right end.
   If a maxlen is set and the deque is full, the leftmost item is discarded.
   If maxlen==0, the operation is a no-op. */
static PyObject *
ArrayDeque_append(ArrayDequeObject *self, PyObject *arg)
{
    if (self->maxlen == 0) {
        Py_RETURN_NONE;
    }
    if (self->maxlen >= 0 && self->size == self->maxlen) {
        PyObject *old = self->array[self->head];
        Py_DECREF(old);
        self->array[self->head] = NULL;
        self->head++;
        self->size--;
    }
    if (self->tail >= self->capacity) {
        if (arraydeque_resize(self, self->capacity * 2) < 0)
            return NULL;
    }
    Py_INCREF(arg);
    self->array[self->tail] = arg;
    self->tail++;
    self->size++;
    Py_RETURN_NONE;
}

/* Method: appendleft(x)
   Append an item to the left end.
   If a maxlen is set and the deque is full, the rightmost element is discarded.
   If maxlen==0, the operation is a no-op. */
static PyObject *
ArrayDeque_appendleft(ArrayDequeObject *self, PyObject *arg)
{
    if (self->maxlen == 0) {
        Py_RETURN_NONE;
    }
    if (self->maxlen >= 0 && self->size == self->maxlen) {
        self->tail--;
        Py_DECREF(self->array[self->tail]);
        self->array[self->tail] = NULL;
        self->size--;
    }
    if (self->head <= 0) {
        if (arraydeque_resize(self, self->capacity * 2) < 0)
            return NULL;
    }
    self->head--;
    Py_INCREF(arg);
    self->array[self->head] = arg;
    self->size++;
    Py_RETURN_NONE;
}

/* Method: pop()
   Remove and return an item from the right end. */
static PyObject *
ArrayDeque_pop(ArrayDequeObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->size == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
        return NULL;
    }
    self->tail--;
    self->size--;
    PyObject *item = self->array[self->tail];
    self->array[self->tail] = NULL;
    return item;
}

/* Method: popleft()
   Remove and return an item from the left end. */
static PyObject *
ArrayDeque_popleft(ArrayDequeObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->size == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
        return NULL;
    }
    PyObject *item = self->array[self->head];
    self->array[self->head] = NULL;
    self->head++;
    self->size--;
    return item;
}

/* Method: clear()
   Remove all items from the deque. */
static PyObject *
ArrayDeque_clear(ArrayDequeObject *self, PyObject *Py_UNUSED(ignored))
{
    for (Py_ssize_t i = self->head; i < self->tail; i++) {
        Py_CLEAR(self->array[i]);
    }
    self->size = 0;
    self->head = self->capacity / 2;
    self->tail = self->head;
    Py_RETURN_NONE;
}

/* Method: extend(iterable)
   Extend the right side with elements from an iterable. */
static PyObject *
ArrayDeque_extend(ArrayDequeObject *self, PyObject *iterable)
{
    PyObject *iterator, *item;
    iterator = PyObject_GetIter(iterable);
    if (iterator == NULL)
        return NULL;
    while ((item = PyIter_Next(iterator)) != NULL) {
        if (ArrayDeque_append(self, item) == NULL) {
            Py_DECREF(item);
            Py_DECREF(iterator);
            return NULL;
        }
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

/* Method: extendleft(iterable)
   Extend the left side with elements from an iterable.
   Note the order is reversed relative to the input. */
static PyObject *
ArrayDeque_extendleft(ArrayDequeObject *self, PyObject *iterable)
{
    PyObject *list;
    Py_ssize_t len, i;

    list = PySequence_List(iterable);
    if (list == NULL)
        return NULL;
    len = PyList_Size(list);
    for (i = 0; i < len; i++) {
        PyObject *item = PyList_GET_ITEM(list, i);
        if (ArrayDeque_appendleft(self, item) == NULL) {
            Py_DECREF(list);
            return NULL;
        }
    }
    Py_DECREF(list);
    Py_RETURN_NONE;
}

/* Method: rotate(n=1)
   Rotate the deque n steps to the right (n negative rotates left).
   Each rotation uses pop/popleft and append/appendleft. */
static PyObject *
ArrayDeque_rotate(ArrayDequeObject *self, PyObject *args)
{
    long n = 1;
    if (!PyArg_ParseTuple(args, "|l:rotate", &n))
        return NULL;
    if (self->size == 0) {
        Py_RETURN_NONE;
    }
    n = n % self->size;
    if (n > 0) {
        for (long i = 0; i < n; i++) {
            PyObject *item = ArrayDeque_pop(self, NULL);
            if (item == NULL)
                return NULL;
            if (ArrayDeque_appendleft(self, item) == NULL) {
                Py_DECREF(item);
                return NULL;
            }
            Py_DECREF(item);
        }
    }
    else if (n < 0) {
        n = -n;
        for (long i = 0; i < n; i++) {
            PyObject *item = ArrayDeque_popleft(self, NULL);
            if (item == NULL)
                return NULL;
            if (ArrayDeque_append(self, item) == NULL) {
                Py_DECREF(item);
                return NULL;
            }
            Py_DECREF(item);
        }
    }
    Py_RETURN_NONE;
}

/* Method: remove(value)
   Remove the first occurrence of value. */
static PyObject *
ArrayDeque_remove(ArrayDequeObject *self, PyObject *value)
{
    Py_ssize_t i;
    for (i = self->head; i < self->tail; i++) {
        int cmp = PyObject_RichCompareBool(self->array[i], value, Py_EQ);
        if (cmp < 0)
            return NULL;
        if (cmp)
            break;
    }
    if (i == self->tail) {
        PyErr_SetString(PyExc_ValueError, "value not found in deque");
        return NULL;
    }
    Py_DECREF(self->array[i]);
    for (Py_ssize_t j = i; j < self->tail - 1; j++) {
        self->array[j] = self->array[j + 1];
    }
    self->array[self->tail - 1] = NULL;
    self->tail--;
    self->size--;
    Py_RETURN_NONE;
}

/* Method: count(value)
   Count the number of occurrences of value. */
static PyObject *
ArrayDeque_count(ArrayDequeObject *self, PyObject *value)
{
    Py_ssize_t count = 0;
    for (Py_ssize_t i = self->head; i < self->tail; i++) {
        int cmp = PyObject_RichCompareBool(self->array[i], value, Py_EQ);
        if (cmp < 0)
            return NULL;
        if (cmp)
            count++;
    }
    return PyLong_FromSsize_t(count);
}

/* Sequence protocol: __len__ support */
static Py_ssize_t
ArrayDeque_length(ArrayDequeObject *self)
{
    return self->size;
}

/* Sequence protocol: __getitem__ (only integer indices) */
static PyObject *
ArrayDeque_seq_getitem(ArrayDequeObject *self, Py_ssize_t index)
{
    if (index < 0)
        index += self->size;
    if (index < 0 || index >= self->size) {
        PyErr_SetString(PyExc_IndexError, "deque index out of range");
        return NULL;
    }
    PyObject *item = self->array[self->head + index];
    Py_INCREF(item);
    return item;
}

/* Mapping protocol: __getitem__ (same as sequence) */
static PyObject *
ArrayDeque_getitem(ArrayDequeObject *self, PyObject *key)
{
    if (!PyLong_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "deque indices must be integers");
        return NULL;
    }
    Py_ssize_t index = PyNumber_AsSsize_t(key, PyExc_IndexError);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    return ArrayDeque_seq_getitem(self, index);
}

/* Sequence protocol: __setitem__ (only integer indices) */
static int
ArrayDeque_seq_setitem(ArrayDequeObject *self, Py_ssize_t index, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "deque deletion not supported");
        return -1;
    }
    if (index < 0)
        index += self->size;
    if (index < 0 || index >= self->size) {
        PyErr_SetString(PyExc_IndexError, "deque assignment index out of range");
        return -1;
    }
    PyObject *old = self->array[self->head + index];
    Py_INCREF(value);
    self->array[self->head + index] = value;
    Py_DECREF(old);
    return 0;
}

/* Mapping protocol: __setitem__ */
static int
ArrayDeque_setitem(ArrayDequeObject *self, PyObject *key, PyObject *value)
{
    if (!PyLong_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "deque indices must be integers");
        return -1;
    }
    Py_ssize_t index = PyNumber_AsSsize_t(key, PyExc_IndexError);
    if (index == -1 && PyErr_Occurred())
        return -1;
    return ArrayDeque_seq_setitem(self, index, value);
}

/* __contains__ implementation */
static int
ArrayDeque_contains(ArrayDequeObject *self, PyObject *value)
{
    for (Py_ssize_t i = self->head; i < self->tail; i++) {
        int cmp = PyObject_RichCompareBool(self->array[i], value, Py_EQ);
        if (cmp < 0)
            return -1;
        if (cmp)
            return 1;
    }
    return 0;
}

/* Rich comparison support: only equality and inequality */
static PyObject *
ArrayDeque_richcompare(PyObject *self, PyObject *other, int op)
{
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyObject *self_list = PySequence_List(self);
    PyObject *other_list = PySequence_List(other);
    if (!self_list || !other_list) {
        Py_XDECREF(self_list);
        Py_XDECREF(other_list);
        return NULL;
    }
    int equal = PyObject_RichCompareBool(self_list, other_list, Py_EQ);
    Py_DECREF(self_list);
    Py_DECREF(other_list);
    if (equal < 0)
        return NULL;
    if (op == Py_EQ) {
        if (equal) {
            Py_INCREF(Py_True);
            return Py_True;
        } else {
            Py_INCREF(Py_False);
            return Py_False;
        }
    } else {
        if (equal) {
            Py_INCREF(Py_False);
            return Py_False;
        } else {
            Py_INCREF(Py_True);
            return Py_True;
        }
    }
}

/* __repr__ implementation */
static PyObject *
ArrayDeque_repr(ArrayDequeObject *self)
{
    PyObject *list = PyList_New(self->size);
    if (!list)
        return NULL;
    for (Py_ssize_t i = 0; i < self->size; i++) {
        PyObject *item = self->array[self->head + i];
        Py_INCREF(item);
        PyList_SET_ITEM(list, i, item);
    }
    PyObject *repr = PyObject_Repr(list);
    Py_DECREF(list);
    if (!repr)
        return NULL;
    PyObject *result = PyUnicode_FromFormat("ArrayDeque(%U)", repr);
    Py_DECREF(repr);
    return result;
}

/* __str__ implementation: same as __repr__ */
static PyObject *
ArrayDeque_str(ArrayDequeObject *self)
{
    return ArrayDeque_repr(self);
}

/* Iterator for ArrayDeque */
static void
ArrayDequeIter_dealloc(ArrayDequeIter *it)
{
    Py_XDECREF(it->deque);
    Py_TYPE(it)->tp_free((PyObject *)it);
}

static PyObject *
ArrayDequeIter_next(ArrayDequeIter *it)
{
    if (it->index < it->deque->size) {
        PyObject *item = it->deque->array[it->deque->head + it->index];
        it->index++;
        Py_INCREF(item);
        return item;
    }
    return NULL;
}

static PyTypeObject ArrayDequeIter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "arraydeque.ArrayDequeIter",
    .tp_basicsize = sizeof(ArrayDequeIter),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)ArrayDequeIter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)ArrayDequeIter_next,
};

/* __iter__ method for ArrayDeque */
static PyObject *
ArrayDeque_iter(ArrayDequeObject *self)
{
    ArrayDequeIter *it;
    it = PyObject_New(ArrayDequeIter, &ArrayDequeIter_Type);
    if (it == NULL)
        return NULL;
    Py_INCREF(self);
    it->deque = self;
    it->index = 0;
    return (PyObject *)it;
}

/* __new__ method: allocate a new ArrayDeque.
   This version uses PyObject_GC_New to ensure that the object
   is not already tracked by the garbage collector (fixing issues on macOS). */
static PyObject *
ArrayDeque_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ArrayDequeObject *self;
    self = PyObject_GC_New(ArrayDequeObject, type);
    if (self == NULL)
        return NULL;
    self->weakreflist = NULL;
    self->capacity = 8;  /* initial capacity */
    self->size = 0;
    self->head = self->capacity / 2;
    self->tail = self->head;
    self->array = PyMem_New(PyObject *, self->capacity);
    if (self->array == NULL) {
        Py_DECREF(self);
        PyErr_NoMemory();
        return NULL;
    }
    for (Py_ssize_t i = 0; i < self->capacity; i++) {
        self->array[i] = NULL;
    }
    /* Default: unbounded deque */
    self->maxlen = -1;
    PyObject_GC_Track((PyObject *)self);
    return (PyObject *)self;
}

/* __init__ method: optionally initialize the deque with an iterable and a maxlen.
   Signature: ArrayDeque([iterable[, maxlen]])
   If maxlen is provided (and not None) it must be a non-negative integer.
   When iterable is longer than maxlen, only the rightmost elements are retained.
*/
static int
ArrayDeque_init(ArrayDequeObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"iterable", "maxlen", NULL};
    PyObject *iterable = NULL;
    PyObject *maxlen_obj = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO:__init__", kwlist,
                                     &iterable, &maxlen_obj))
        return -1;

    if (maxlen_obj == Py_None) {
        self->maxlen = -1;
    } else {
        Py_ssize_t m = PyLong_AsSsize_t(maxlen_obj);
        if (m < 0) {
            PyErr_SetString(PyExc_ValueError, "maxlen must be a non-negative integer");
            return -1;
        }
        self->maxlen = m;
    }

    if (iterable && iterable != Py_None) {
        PyObject *iterator = PyObject_GetIter(iterable);
        if (iterator == NULL)
            return -1;
        PyObject *item;
        while ((item = PyIter_Next(iterator)) != NULL) {
            if (ArrayDeque_append(self, item) == NULL) {
                Py_DECREF(item);
                Py_DECREF(iterator);
                return -1;
            }
            Py_DECREF(item);
        }
        Py_DECREF(iterator);
        if (PyErr_Occurred())
            return -1;
    }
    return 0;
}

/* __dealloc__ method: free all references and the array */
static void
ArrayDeque_dealloc(ArrayDequeObject *self)
{
    PyObject_GC_UnTrack(self);
    PyObject_ClearWeakRefs((PyObject *)self);
    for (Py_ssize_t i = self->head; i < self->tail; i++) {
        Py_CLEAR(self->array[i]);
    }
    PyMem_Free(self->array);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/* GC traverse function */
static int
ArrayDeque_traverse(ArrayDequeObject *self, visitproc visit, void *arg)
{
    for (Py_ssize_t i = self->head; i < self->tail; i++) {
        Py_VISIT(self->array[i]);
    }
    return 0;
}

/* GC clear function */
static int
ArrayDeque_clear_func(ArrayDequeObject *self)
{
    for (Py_ssize_t i = self->head; i < self->tail; i++) {
        Py_CLEAR(self->array[i]);
    }
    return 0;
}

/* Getter for the maxlen attribute.
   Returns None if unbounded; otherwise a Python integer. */
static PyObject *
ArrayDeque_get_maxlen(ArrayDequeObject *self, void *closure)
{
    if (self->maxlen < 0)
        Py_RETURN_NONE;
    return PyLong_FromSsize_t(self->maxlen);
}

/* __reduce__ for pickling */
static PyObject *
ArrayDeque_reduce(ArrayDequeObject *self)
{
    PyObject *list = PyList_New(self->size);
    if (!list)
        return NULL;
    for (Py_ssize_t i = 0; i < self->size; i++) {
        PyObject *item = self->array[self->head + i];
        Py_INCREF(item);
        PyList_SET_ITEM(list, i, item);
    }
    PyObject *maxlen;
    if (self->maxlen < 0) {
        maxlen = Py_None;
        Py_INCREF(Py_None);
    } else {
        maxlen = PyLong_FromSsize_t(self->maxlen);
        if (maxlen == NULL) {
            Py_DECREF(list);
            return NULL;
        }
    }
    PyObject *args = Py_BuildValue("(OO)", list, maxlen);
    Py_DECREF(list);
    Py_DECREF(maxlen);
    Py_INCREF(Py_TYPE(self));
    return Py_BuildValue("OO", Py_TYPE(self), args);
}

/* Get/Set definitions */
static PyGetSetDef ArrayDeque_getsetters[] = {
    {"maxlen", (getter)ArrayDeque_get_maxlen, NULL,
     "maximum length (read-only); None if unbounded", NULL},
    {NULL}  /* Sentinel */
};

/* Methods table */
static PyMethodDef ArrayDeque_methods[] = {
    {"append",      (PyCFunction)ArrayDeque_append,      METH_O,
     "Append an element to the right end"},
    {"appendleft",  (PyCFunction)ArrayDeque_appendleft,  METH_O,
     "Append an element to the left end"},
    {"pop",         (PyCFunction)ArrayDeque_pop,         METH_NOARGS,
     "Remove and return an element from the right end"},
    {"popleft",     (PyCFunction)ArrayDeque_popleft,     METH_NOARGS,
     "Remove and return an element from the left end"},
    {"clear",       (PyCFunction)ArrayDeque_clear,       METH_NOARGS,
     "Remove all elements"},
    {"extend",      (PyCFunction)ArrayDeque_extend,      METH_O,
     "Extend the right side with elements from an iterable"},
    {"extendleft",  (PyCFunction)ArrayDeque_extendleft,  METH_O,
     "Extend the left side with elements from an iterable"},
    {"rotate",      (PyCFunction)ArrayDeque_rotate,      METH_VARARGS,
     "Rotate the deque n steps to the right (default 1). If n is negative, rotate left."},
    {"remove",      (PyCFunction)ArrayDeque_remove,      METH_O,
     "Remove the first occurrence of value"},
    {"count",       (PyCFunction)ArrayDeque_count,       METH_O,
     "Count the number of occurrences of value"},
    {"__reduce__",  (PyCFunction)ArrayDeque_reduce,      METH_NOARGS,
     "Helper for pickle."},
    {NULL}  /* Sentinel */
};

/* Sequence methods */
static PySequenceMethods ArrayDeque_as_sequence = {
    (lenfunc)ArrayDeque_length,               /* sq_length */
    0,                                        /* sq_concat */
    0,                                        /* sq_repeat */
    (ssizeargfunc)ArrayDeque_seq_getitem,       /* sq_item */
    0,                                        /* sq_slice */
    (ssizeobjargproc)ArrayDeque_seq_setitem,    /* sq_ass_item */
    0,                                        /* sq_ass_slice */
    (objobjproc)ArrayDeque_contains,            /* sq_contains */
    0,                                        /* sq_inplace_concat */
    0                                         /* sq_inplace_repeat */
};

/* Mapping methods */
static PyMappingMethods ArrayDeque_as_mapping = {
    (lenfunc)ArrayDeque_length,       /* mp_length */
    (binaryfunc)ArrayDeque_getitem,   /* mp_subscript */
    (objobjargproc)ArrayDeque_setitem,/* mp_ass_subscript */
};

/* Type definition for ArrayDeque */
static PyTypeObject ArrayDequeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "arraydeque.ArrayDeque",
    .tp_doc = "Array-backed deque with optional bounded length",
    .tp_basicsize = sizeof(ArrayDequeObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)ArrayDeque_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_new = ArrayDeque_new,
    .tp_init = (initproc)ArrayDeque_init,
    .tp_iter = (getiterfunc)ArrayDeque_iter,
    .tp_methods = ArrayDeque_methods,
    .tp_as_sequence = &ArrayDeque_as_sequence,
    .tp_as_mapping = &ArrayDeque_as_mapping,
    .tp_getset = ArrayDeque_getsetters,
    .tp_str = (reprfunc)ArrayDeque_str,
    .tp_repr = (reprfunc)ArrayDeque_repr,
    .tp_richcompare = ArrayDeque_richcompare,
    .tp_weaklistoffset = offsetof(ArrayDequeObject, weakreflist),
    .tp_traverse = (traverseproc)ArrayDeque_traverse,
    .tp_clear = (inquiry)ArrayDeque_clear_func,
};

/* Module definition */
static PyModuleDef arraydequemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "arraydeque",
    .m_doc = "Array-based deque implementation with optional maxlen support",
    .m_size = -1,
};

/* Module initialization function */
PyMODINIT_FUNC
PyInit_arraydeque(void)
{
    PyObject *m;
    if (PyType_Ready(&ArrayDequeType) < 0)
        return NULL;
    if (PyType_Ready(&ArrayDequeIter_Type) < 0)
        return NULL;

    m = PyModule_Create(&arraydequemodule);
    if (m == NULL)
        return NULL;
    Py_INCREF(&ArrayDequeType);
    if (PyModule_AddObject(m, "ArrayDeque", (PyObject *)&ArrayDequeType) < 0) {
        Py_DECREF(&ArrayDequeType);
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddStringConstant(m, "__version__", ARRAYDEQUE_VERSION) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
