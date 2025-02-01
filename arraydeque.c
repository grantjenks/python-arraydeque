#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

/* Define the module version here.
   If ARRAYDEQUE_VERSION is already defined (for example via a compiler flag),
   we won’t override it. */
#ifndef ARRAYDEQUE_VERSION
#define ARRAYDEQUE_VERSION "0.1.0"
#endif

/* The ArrayDeque object structure. We keep an array of PyObject*,
   a capacity, the current number of items (size), and two indices:
   head is the index of the first valid item and tail is the index just
   after the last valid item. We keep the items roughly centered in the array. */
typedef struct {
    PyObject_HEAD
    PyObject **array;       /* pointer to array of PyObject* */
    Py_ssize_t capacity;    /* allocated length of array */
    Py_ssize_t size;        /* number of elements stored */
    Py_ssize_t head;        /* index of first element */
    Py_ssize_t tail;        /* index one past the last element */
} ArrayDequeObject;

/* Forward declaration of type for iterator */
typedef struct {
    PyObject_HEAD
    ArrayDequeObject *deque; /* reference to the deque */
    Py_ssize_t index;        /* current index into the deque (0 .. size) */
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
    /* Free the old array and update the structure */
    PyMem_Free(self->array);
    self->array = new_array;
    self->capacity = new_capacity;
    self->head = new_head;
    self->tail = new_head + self->size;
    return 0;
}

/* Method: append(x)
   Append an item to the right end. */
static PyObject *
ArrayDeque_append(ArrayDequeObject *self, PyObject *arg)
{
    /* If there is no room at the right end, grow the array. */
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
   Append an item to the left end. */
static PyObject *
ArrayDeque_appendleft(ArrayDequeObject *self, PyObject *arg)
{
    /* If there is no room at the left end, grow the array. */
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
    Py_ssize_t i;
    for (i = self->head; i < self->tail; i++) {
        Py_CLEAR(self->array[i]);
    }
    self->size = 0;
    /* Reset head and tail to the center of the current allocation */
    self->head = self->capacity / 2;
    self->tail = self->head;
    Py_RETURN_NONE;
}

/* Method: extend(iterable)
   Extend the right side of the deque by appending elements from the iterable. */
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
   Extend the left side of the deque by appending elements from the iterable.
   Note that the series of left appends results in reversing the order
   of the input. */
static PyObject *
ArrayDeque_extendleft(ArrayDequeObject *self, PyObject *iterable)
{
    PyObject *list;
    Py_ssize_t len, i;

    list = PySequence_List(iterable);
    if (list == NULL)
        return NULL;
    len = PyList_Size(list);
    /* Iterate in forward order: each call to appendleft will put the item
       before the previous ones, thus reversing the order relative to the input. */
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

/* Sequence protocol: __len__ support */
static Py_ssize_t
ArrayDeque_length(ArrayDequeObject *self)
{
    return self->size;
}

/* Sequence protocol: __getitem__ support (only for integer indices) */
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

/* Mapping protocol: __getitem__ support (same as sequence) */
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

/* Sequence protocol: __setitem__ support (only for integer indices) */
static int
ArrayDeque_seq_setitem(ArrayDequeObject *self, Py_ssize_t index, PyObject *value)
{
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

/* Mapping protocol: __setitem__ support */
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

/* Define sequence methods (supporting __len__ and indexing via __getitem__ and __setitem__) */
static PySequenceMethods ArrayDeque_as_sequence = {
    (lenfunc)ArrayDeque_length,           /* sq_length */
    0,                                    /* sq_concat */
    0,                                    /* sq_repeat */
    (ssizeargfunc)ArrayDeque_seq_getitem,  /* sq_item */
    0,                                    /* sq_slice */
    (ssizeobjargproc)ArrayDeque_seq_setitem,/* sq_ass_item */
    0,                                    /* sq_ass_slice */
    0,                                    /* sq_contains */
    0,                                    /* sq_inplace_concat */
    0,                                    /* sq_inplace_repeat */
};

/* Define mapping methods so that deque[index] works as expected. */
static PyMappingMethods ArrayDeque_as_mapping = {
    (lenfunc)ArrayDeque_length,       /* mp_length */
    (binaryfunc)ArrayDeque_getitem,   /* mp_subscript */
    (objobjargproc)ArrayDeque_setitem,/* mp_ass_subscript */
};

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
    /* No more items: signal end of iteration */
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

/* __iter__ method for ArrayDeque: return a new iterator */
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

/* __new__ method: allocate a new ArrayDeque */
static PyObject *
ArrayDeque_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ArrayDequeObject *self;
    self = (ArrayDequeObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;
    self->capacity = 8;  /* initial capacity */
    self->size = 0;
    /* Start in the middle so that both ends have some space */
    self->head = self->capacity / 2;
    self->tail = self->head;
    self->array = PyMem_New(PyObject *, self->capacity);
    if (self->array == NULL) {
        Py_DECREF(self);
        PyErr_NoMemory();
        return NULL;
    }
    /* Initialize all slots to NULL */
    for (Py_ssize_t i = 0; i < self->capacity; i++) {
        self->array[i] = NULL;
    }
    return (PyObject *)self;
}

/* __init__ method: optionally initialize the deque with an iterable.
   This allows calls such as ArrayDeque([1, 2, 3, 4]). */
static int
ArrayDeque_init(ArrayDequeObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *iterable = NULL;
    if (!PyArg_ParseTuple(args, "|O", &iterable))
        return -1;
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
    Py_ssize_t i;
    for (i = self->head; i < self->tail; i++) {
        Py_XDECREF(self->array[i]);
    }
    PyMem_Free(self->array);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

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
    {NULL}  /* Sentinel */
};

/* Type definition for ArrayDeque */
static PyTypeObject ArrayDequeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "arraydeque",
    .tp_doc = "Array-backed deque",
    .tp_basicsize = sizeof(ArrayDequeObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)ArrayDeque_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = ArrayDeque_new,
    .tp_init = (initproc)ArrayDeque_init,
    .tp_iter = (getiterfunc)ArrayDeque_iter,
    .tp_methods = ArrayDeque_methods,
    .tp_as_sequence = &ArrayDeque_as_sequence,
    .tp_as_mapping = &ArrayDeque_as_mapping,
};

/* Module definition */
static PyModuleDef arraydequemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "arraydeque",
    .m_doc = "Array-based deque implementation",
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
    /* Add the type object */
    Py_INCREF(&ArrayDequeType);
    if (PyModule_AddObject(m, "ArrayDeque", (PyObject *)&ArrayDequeType) < 0) {
        Py_DECREF(&ArrayDequeType);
        Py_DECREF(m);
        return NULL;
    }
    /* Add the version in the module so that arraydeque.__version__ is available */
    if (PyModule_AddStringConstant(m, "__version__", ARRAYDEQUE_VERSION) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
