#include "_scopedvar_module.h"

PyTypeObject VarObject_Type;

static inline CellObject *_Var_GetTrueCell(VarObject *self);
static inline int         _Var_SetCell(VarObject *self, CellObject *cell);

#define DECLARE_CELL(SELF, NAME)                                               \
    CellObject *NAME = _Var_GetTrueCell(Var_CAST(SELF));

#define ENSURE_VAR_CACHE_UPDATED(SELF, ERRVAL)                                 \
    do {                                                                       \
        CellObject *      cell = NULL;                                         \
        PyThreadState *   ts = NULL;                                           \
        ScopeStackObject *stack = Var_CAST(SELF)->stack;                       \
        if (Var_CAST(SELF)->cached_stack_ver == stack->stack_ver) {            \
            ts = PyThreadState_Get();                                          \
            if (Var_CAST(SELF)->cached_tsid == ts->id &&                       \
                Var_CAST(SELF)->cached_tsver == ts->context_ver)               \
                break;                                                         \
        }                                                                      \
        cell = ScopeStack_FindCell(stack, (PyObject *)Var_CAST(SELF)->decl);   \
        if (!cell) {                                                           \
            return (ERRVAL);                                                   \
        }                                                                      \
        _Var_SetCell(Var_CAST(SELF), cell);                                    \
        Var_CAST(SELF)->cached_stack_ver = stack->stack_ver;                   \
        if (ts) {                                                              \
            Var_CAST(SELF)->cached_tsid = ts->id;                              \
            Var_CAST(SELF)->cached_tsver = ts->context_ver;                    \
        }                                                                      \
    } while (0);

#define ENSURE_MUTABLE(SELF, ERRVAL)                                           \
    do {                                                                       \
        if (!((VarObject *)(SELF))->mutable) {                                 \
            PyErr_SetString(PyExc_RuntimeError, "var is not mutable");         \
            return (ERRVAL);                                                   \
        }                                                                      \
    } while (0);

#define _ENSURE_NOT_EMPTY(SELF, ERRVAL, CELL)                                  \
    do {                                                                       \
        if (!CELL) {                                                           \
            PyErr_SetString(PyExc_ValueError, "no cell attached");             \
            return (ERRVAL);                                                   \
        }                                                                      \
        if (!CELL->wrapped) {                                                  \
            PyErr_Format(                                                      \
                PyExc_ValueError, "attached cell [%lu] is empty", CELL);       \
            return (ERRVAL);                                                   \
        }                                                                      \
    } while (0);

#define ENSURE_NOT_EMPTY(SELF, ERRVAL) _ENSURE_NOT_EMPTY(SELF, ERRVAL, cell)

#define BINARY_FUNC_BEGIN                                                      \
    if (PyObject_IsInstance(o1, (PyObject *)&VarObject_Type)) {                \
        ENSURE_VAR_CACHE_UPDATED(o1, NULL)                                     \
        DECLARE_CELL(o1, cell_o1)                                              \
        _ENSURE_NOT_EMPTY(o1, NULL, cell_o1)                                   \
        o1 = cell_o1->wrapped;                                                 \
    }                                                                          \
                                                                               \
    if (PyObject_IsInstance(o2, (PyObject *)&VarObject_Type)) {                \
        ENSURE_VAR_CACHE_UPDATED(o2, NULL)                                     \
        DECLARE_CELL(o2, cell_o2)                                              \
        _ENSURE_NOT_EMPTY(o2, NULL, cell_o2)                                   \
        o2 = cell_o2->wrapped;                                                 \
    }

#define INPLACE_BINARY_FUNC_BEGIN                                              \
    ENSURE_MUTABLE(self, NULL)                                                 \
                                                                               \
    PyObject *object = NULL;                                                   \
                                                                               \
    ENSURE_VAR_CACHE_UPDATED(self, NULL)                                       \
    DECLARE_CELL(self, cell)                                                   \
    ENSURE_NOT_EMPTY(self, NULL)                                               \
                                                                               \
    if (PyObject_IsInstance(other, (PyObject *)&VarObject_Type)) {             \
        ENSURE_VAR_CACHE_UPDATED(other, NULL)                                  \
        DECLARE_CELL(other, cell_other)                                        \
        _ENSURE_NOT_EMPTY(other, NULL, cell_other)                             \
        other = cell->wrapped;                                                 \
    }

#define INPLACE_BINARY_FUNC_END                                                \
    do {                                                                       \
        if (!object)                                                           \
            return NULL;                                                       \
                                                                               \
        Py_XSETREF(self->cell->wrapped, object);                               \
        Py_XSETREF(self->cell->prev, NULL);                                    \
                                                                               \
        Py_INCREF(self);                                                       \
        return (PyObject *)self;                                               \
    } while (0);

static PyObject *Var_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VarObject *self;

    self = (VarObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->cell = NULL;
    self->weakreflist = NULL;
    self->mutable = 0;

    self->stack = NULL;
    self->decl = NULL;
    self->cached_stack_ver = 0;
    self->cached_tsid = 0;
    self->cached_tsver = 0;

    return (PyObject *)self;
}

static int Var_traverse(VarObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->cell);

    return 0;
}

static int Var_clear(VarObject *self) {
    Py_CLEAR(self->cell);

    return 0;
}

static void Var_dealloc(VarObject *self) {
    PyObject_GC_UnTrack(self);

    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);

    Var_clear(self);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *Var_repr(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)

    return PyUnicode_FromFormat(
        "<%s at %p for %s at %p>",
        Py_TYPE(self)->tp_name,
        self,
        Py_TYPE(cell->wrapped)->tp_name,
        cell->wrapped);
}

static Py_hash_t Var_hash(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PyObject_Hash(cell->wrapped);
}

static PyObject *Var_str(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_Str(cell->wrapped);
}

static PyObject *Var_add(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Add(o1, o2);
}

static PyObject *Var_subtract(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Subtract(o1, o2);
}

static PyObject *Var_multiply(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Multiply(o1, o2);
}

static PyObject *Var_remainder(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Remainder(o1, o2);
}

static PyObject *Var_divmod(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Divmod(o1, o2);
}

static PyObject *Var_power(PyObject *o1, PyObject *o2, PyObject *modulo) {
    BINARY_FUNC_BEGIN
    return PyNumber_Power(o1, o2, modulo);
}

static PyObject *Var_negative(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Negative(cell->wrapped);
}

static PyObject *Var_positive(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Positive(cell->wrapped);
}

static PyObject *Var_absolute(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Absolute(cell->wrapped);
}

static int Var_bool(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PyObject_IsTrue(cell->wrapped);
}

static PyObject *Var_invert(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Invert(cell->wrapped);
}

static PyObject *Var_lshift(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Lshift(o1, o2);
}

static PyObject *Var_rshift(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Rshift(o1, o2);
}

static PyObject *Var_and(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_And(o1, o2);
}

static PyObject *Var_xor(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Xor(o1, o2);
}

static PyObject *Var_or(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_Or(o1, o2);
}

static PyObject *Var_long(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Long(cell->wrapped);
}

static PyObject *Var_float(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Float(cell->wrapped);
}

static PyObject *Var_inplace_add(VarObject *self, PyObject *other) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceAdd(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_subtract(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceSubtract(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_multiply(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceMultiply(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_remainder(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceRemainder(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *
Var_inplace_power(VarObject *self, PyObject *other, PyObject *modulo) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlacePower(cell->wrapped, other, modulo);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_lshift(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceLshift(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_rshift(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceRshift(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_and(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceAnd(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_xor(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceXor(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_or(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceOr(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_floor_divide(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_FloorDivide(o1, o2);
}

static PyObject *Var_true_divide(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_TrueDivide(o1, o2);
}

static PyObject *Var_inplace_floor_divide(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceFloorDivide(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_inplace_true_divide(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceTrueDivide(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_matrix_multiply(PyObject *o1, PyObject *o2) {
    BINARY_FUNC_BEGIN
    return PyNumber_MatrixMultiply(o1, o2);
}

static PyObject *Var_inplace_matrix_multiply(VarObject *self, PyObject *other) {
    INPLACE_BINARY_FUNC_BEGIN
    object = PyNumber_InPlaceMatrixMultiply(cell->wrapped, other);
    INPLACE_BINARY_FUNC_END
}

static PyObject *Var_index(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyNumber_Index(cell->wrapped);
}

static Py_ssize_t Var_length(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PyObject_Length(cell->wrapped);
}

static int Var_contains(VarObject *self, PyObject *value) {
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PySequence_Contains(cell->wrapped, value);
}

static PyObject *Var_getitem(VarObject *self, PyObject *key) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetItem(cell->wrapped, key);
}

static int Var_setitem(VarObject *self, PyObject *key, PyObject *value) {
    ENSURE_MUTABLE(self, -1)
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    if (value == NULL)
        return PyObject_DelItem(cell->wrapped, key);
    else
        return PyObject_SetItem(cell->wrapped, key, value);
}

static PyObject *Var_dir(VarObject *self, PyObject *args) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_Dir(cell->wrapped);
}

static PyObject *Var_enter(VarObject *self, PyObject *args, PyObject *kwds) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    PyObject *method = NULL;
    PyObject *result = NULL;

    method = PyObject_GetAttrString(cell->wrapped, "__enter__");

    if (!method)
        return NULL;

    result = PyObject_Call(method, args, kwds);

    Py_DECREF(method);

    return result;
}

static PyObject *Var_exit(VarObject *self, PyObject *args, PyObject *kwds) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    PyObject *method = NULL;
    PyObject *result = NULL;

    method = PyObject_GetAttrString(cell->wrapped, "__exit__");

    if (!method)
        return NULL;

    result = PyObject_Call(method, args, kwds);

    Py_DECREF(method);

    return result;
}

static PyObject *Var_copy(VarObject *self, PyObject *args, PyObject *kwds) {
    PyErr_SetString(
        PyExc_NotImplementedError, "cell var must define __copy__()");

    return NULL;
}

static PyObject *Var_deepcopy(VarObject *self, PyObject *args, PyObject *kwds) {
    PyErr_SetString(
        PyExc_NotImplementedError, "cell var must define __deepcopy__()");

    return NULL;
}

static PyObject *Var_reduce(VarObject *self, PyObject *args, PyObject *kwds) {
    PyErr_SetString(
        PyExc_NotImplementedError, "cell var must define __reduce_ex__()");

    return NULL;
}

static PyObject *
Var_reduce_ex(VarObject *self, PyObject *args, PyObject *kwds) {
    PyErr_SetString(
        PyExc_NotImplementedError, "cell var must define __reduce_ex__()");

    return NULL;
}

static PyObject *Var_bytes(VarObject *self, PyObject *args) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_Bytes(cell->wrapped);
}

static PyObject *Var_reversed(VarObject *self, PyObject *args) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_CallFunctionObjArgs(
        (PyObject *)&PyReversed_Type, cell->wrapped, NULL);
}

#if PY_MAJOR_VERSION >= 3
static PyObject *Var_round(VarObject *self, PyObject *args) {
    PyObject *module = NULL;
    PyObject *dict = NULL;
    PyObject *round = NULL;

    PyObject *result = NULL;

    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)

    module = PyImport_ImportModule("builtins");

    if (!module)
        return NULL;

    dict = PyModule_GetDict(module);
    round = PyDict_GetItemString(dict, "round");

    if (!round) {
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(round);
    Py_DECREF(module);

    result = PyObject_CallFunctionObjArgs(round, cell->wrapped, NULL);

    Py_DECREF(round);

    return result;
}
#endif

static PyObject *Var_complex(VarObject *self, PyObject *args) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_CallFunctionObjArgs(
        (PyObject *)&PyComplex_Type, cell->wrapped, NULL);
}

static PyObject *
Var_mro_entries(VarObject *self, PyObject *args, PyObject *kwds) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return Py_BuildValue("(O)", cell->wrapped);
}

static PyObject *Var_get_name(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetAttrString(cell->wrapped, "__name__");
}

static int Var_set_name(VarObject *self, PyObject *value) {
    ENSURE_MUTABLE(self, -1)
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PyObject_SetAttrString(cell->wrapped, "__name__", value);
}

static PyObject *Var_get_qualname(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetAttrString(cell->wrapped, "__qualname__");
}

static int Var_set_qualname(VarObject *self, PyObject *value) {
    ENSURE_MUTABLE(self, -1)
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PyObject_SetAttrString(cell->wrapped, "__qualname__", value);
}

static PyObject *Var_get_module(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetAttrString(cell->wrapped, "__module__");
}

static int Var_set_module(VarObject *self, PyObject *value) {
    ENSURE_MUTABLE(self, -1)
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)

    if (PyObject_SetAttrString(cell->wrapped, "__module__", value) == -1)
        return -1;

    return PyDict_SetItemString(cell->dict, "__module__", value);
}

static PyObject *Var_get_doc(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetAttrString(cell->wrapped, "__doc__");
}

static int Var_set_doc(VarObject *self, PyObject *value) {
    ENSURE_MUTABLE(self, -1)
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)

    if (PyObject_SetAttrString(cell->wrapped, "__doc__", value) == -1)
        return -1;

    return PyDict_SetItemString(cell->dict, "__doc__", value);
}

static PyObject *Var_get_class(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetAttrString(cell->wrapped, "__class__");
}

static PyObject *Var_get_annotations(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetAttrString(cell->wrapped, "__annotations__");
}

static int Var_set_annotations(VarObject *self, PyObject *value) {
    ENSURE_MUTABLE(self, -1)
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)
    return PyObject_SetAttrString(cell->wrapped, "__annotations__", value);
}

static PyObject *Var_getattro(VarObject *self, PyObject *name) {
    PyObject *object = NULL;
    PyObject *result = NULL;

    static PyObject *getattr_str = NULL;

    object = PyObject_GenericGetAttr((PyObject *)self, name);

    if (object)
        return object;

    PyErr_Clear();

    if (!getattr_str) {
        getattr_str = PyUnicode_InternFromString("__getattr__");
    }

    object = PyObject_GenericGetAttr((PyObject *)self, getattr_str);

    if (!object)
        return NULL;

    result = PyObject_CallFunctionObjArgs(object, name, NULL);

    Py_DECREF(object);

    return result;
}

static PyObject *Var_getattr(VarObject *self, PyObject *args) {
    PyObject *name = NULL;

    if (!PyArg_ParseTuple(args, "U:__getattr__", &name))
        return NULL;

    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)

    return PyObject_GetAttr(cell->wrapped, name);
}

static int Var_setattro(VarObject *self, PyObject *name, PyObject *value) {
    ENSURE_MUTABLE(self, -1)

    static PyObject *self_str = NULL;
    static PyObject *wrapped_str = NULL;
    static PyObject *startswith_str = NULL;

    PyObject *match = NULL;

    if (!startswith_str) {
        startswith_str = PyUnicode_InternFromString("startswith");
    }

    if (!self_str) {
        self_str = PyUnicode_InternFromString("_self_");
    }

    match = PyObject_CallMethodObjArgs(name, startswith_str, self_str, NULL);

    if (match == Py_True) {
        Py_DECREF(match);

        return PyObject_GenericSetAttr((PyObject *)self, name, value);
    } else if (!match)
        PyErr_Clear();

    Py_XDECREF(match);

    if (!wrapped_str) {
        wrapped_str = PyUnicode_InternFromString("__wrapped__");
    }

    if (PyObject_HasAttr((PyObject *)Py_TYPE(self), name))
        return PyObject_GenericSetAttr((PyObject *)self, name, value);

    ENSURE_VAR_CACHE_UPDATED(self, -1)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, -1)

    return PyObject_SetAttr(cell->wrapped, name, value);
}

static PyObject *Var_richcompare(VarObject *self, PyObject *other, int opcode) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_RichCompare(cell->wrapped, other, opcode);
}

static PyObject *Var_iter(VarObject *self) {
    ENSURE_VAR_CACHE_UPDATED(self, NULL)
    DECLARE_CELL(self, cell)
    ENSURE_NOT_EMPTY(self, NULL)
    return PyObject_GetIter(cell->wrapped);
}

static PyNumberMethods Var_as_number = {
    .nb_add = (binaryfunc)Var_add,
    .nb_subtract = (binaryfunc)Var_subtract,
    .nb_multiply = (binaryfunc)Var_multiply,
    .nb_remainder = (binaryfunc)Var_remainder,
    .nb_divmod = (binaryfunc)Var_divmod,
    .nb_power = (ternaryfunc)Var_power,
    .nb_negative = (unaryfunc)Var_negative,
    .nb_positive = (unaryfunc)Var_positive,
    .nb_absolute = (unaryfunc)Var_absolute,
    .nb_bool = (inquiry)Var_bool,
    .nb_invert = (unaryfunc)Var_invert,
    .nb_lshift = (binaryfunc)Var_lshift,
    .nb_rshift = (binaryfunc)Var_rshift,
    .nb_and = (binaryfunc)Var_and,
    .nb_xor = (binaryfunc)Var_xor,
    .nb_or = (binaryfunc)Var_or,
    .nb_int = (unaryfunc)Var_long,
    .nb_float = (unaryfunc)Var_float,
    .nb_inplace_add = (binaryfunc)Var_inplace_add,
    .nb_inplace_subtract = (binaryfunc)Var_inplace_subtract,
    .nb_inplace_multiply = (binaryfunc)Var_inplace_multiply,
    .nb_inplace_remainder = (binaryfunc)Var_inplace_remainder,
    .nb_inplace_power = (ternaryfunc)Var_inplace_power,
    .nb_inplace_lshift = (binaryfunc)Var_inplace_lshift,
    .nb_inplace_rshift = (binaryfunc)Var_inplace_rshift,
    .nb_inplace_and = (binaryfunc)Var_inplace_and,
    .nb_inplace_xor = (binaryfunc)Var_inplace_xor,
    .nb_inplace_or = (binaryfunc)Var_inplace_or,
    .nb_inplace_floor_divide = (binaryfunc)Var_floor_divide,
    .nb_true_divide = (binaryfunc)Var_true_divide,
    .nb_inplace_floor_divide = (binaryfunc)Var_inplace_floor_divide,
    .nb_inplace_true_divide = (binaryfunc)Var_inplace_true_divide,
    .nb_index = (unaryfunc)Var_index,
    .nb_matrix_multiply = (binaryfunc)Var_matrix_multiply,
    .nb_inplace_matrix_multiply = (binaryfunc)Var_inplace_matrix_multiply,
};

static PySequenceMethods Var_as_sequence = {
    .sq_length = (lenfunc)Var_length,
    .sq_contains = (objobjproc)Var_contains,
};

static PyMappingMethods Var_as_mapping = {
    .mp_length = (lenfunc)Var_length,
    .mp_subscript = (binaryfunc)Var_getitem,
    .mp_ass_subscript = (objobjargproc)Var_setitem,
};

static PyMethodDef Var_methods[] = {
    {"__dir__", (PyCFunction)Var_dir, METH_NOARGS, 0},
    {"__enter__", (PyCFunction)Var_enter, METH_VARARGS | METH_KEYWORDS, 0},
    {"__exit__", (PyCFunction)Var_exit, METH_VARARGS | METH_KEYWORDS, 0},
    {"__copy__", (PyCFunction)Var_copy, METH_NOARGS, 0},
    {"__deepcopy__",
     (PyCFunction)Var_deepcopy,
     METH_VARARGS | METH_KEYWORDS,
     0},
    {"__reduce__", (PyCFunction)Var_reduce, METH_NOARGS, 0},
    {"__reduce_ex__",
     (PyCFunction)Var_reduce_ex,
     METH_VARARGS | METH_KEYWORDS,
     0},
    {"__getattr__", (PyCFunction)Var_getattr, METH_VARARGS, 0},
    {"__bytes__", (PyCFunction)Var_bytes, METH_NOARGS, 0},
    {"__reversed__", (PyCFunction)Var_reversed, METH_NOARGS, 0},
    {"__round__", (PyCFunction)Var_round, METH_NOARGS, 0},
    {"__complex__", (PyCFunction)Var_complex, METH_NOARGS, 0},
#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7)
    {"__mro_entries__",
     (PyCFunction)Var_mro_entries,
     METH_VARARGS | METH_KEYWORDS,
     0},
#endif
    {NULL, NULL},
};

static PyGetSetDef Var_getset[] = {
    {"__name__", (getter)Var_get_name, (setter)Var_set_name, 0},
    {"__qualname__", (getter)Var_get_qualname, (setter)Var_set_qualname, 0},
    {"__module__", (getter)Var_get_module, (setter)Var_set_module, 0},
    {"__doc__", (getter)Var_get_doc, (setter)Var_set_doc, 0},
    {"__class__", (getter)Var_get_class, NULL, 0},
    {"__annotations__",
     (getter)Var_get_annotations,
     (setter)Var_set_annotations,
     0},
    {NULL},
};

PyTypeObject VarObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "Var",
    .tp_basicsize = sizeof(VarObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)Var_dealloc,
    .tp_repr = (unaryfunc)Var_repr,
    .tp_as_number = &Var_as_number,
    .tp_as_sequence = &Var_as_sequence,
    .tp_as_mapping = &Var_as_mapping,
    .tp_hash = (hashfunc)Var_hash,
    .tp_str = (unaryfunc)Var_str,
    .tp_getattro = (getattrofunc)Var_getattro,
    .tp_setattro = (setattrofunc)Var_setattro,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)Var_traverse,
    .tp_clear = (inquiry)Var_clear,
    .tp_richcompare = (richcmpfunc)Var_richcompare,
    .tp_weaklistoffset = offsetof(VarObject, weakreflist),
    .tp_iter = (getiterfunc)Var_iter,
    .tp_methods = Var_methods,
    .tp_getset = Var_getset,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = Var_new,
    .tp_free = PyObject_GC_Del,
};

static inline CellObject *_Var_GetTrueCell(VarObject *self) {
    CellObject *cell = self->cell;
    while (cell->prev)
        cell = cell->prev;
    return cell;
}

static inline int _Var_SetCell(VarObject *self, CellObject *cell) {
    if (cell == self->cell)
        return 0;
    Py_INCREF(cell);
    Py_XSETREF(self->cell, cell);
    return 0;
}

VarObject *Var_New(ScopeStackObject *stack, DeclObject *decl) {
    VarObject *self = NEW_OBJECT(VarObject);
    if (!self)
        return self;
    Py_INCREF(stack);
    self->stack = stack;
    Py_INCREF(decl);
    self->decl = decl;
    self->mutable = DECL_FLAGS_MUTABLE & decl->flags;
    return self;
}

int Var_Assign(VarObject *self, PyObject *rhs) {
    if (!self->mutable) {
        PyErr_SetString(PyExc_RuntimeError, "var is immutable");
        return -1;
    }
    ENSURE_VAR_CACHE_UPDATED(self, -1)
    if (Py_TYPE(rhs) == &VarObject_Type) {
        ENSURE_VAR_CACHE_UPDATED(rhs, -1)
        DECLARE_CELL(rhs, rhs_cell)
        _ENSURE_NOT_EMPTY(rhs, -1, rhs_cell)
        rhs = rhs_cell->wrapped;
    }
    if (Cell_Assign(self->cell, rhs) < 0)
        return -1;
    return 0;
}