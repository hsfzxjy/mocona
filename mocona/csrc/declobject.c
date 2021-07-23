#include "_scopedvar_module.h"

TRACE_REFCNT_INIT(DECL)

static PyObject *Decl_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    DeclObject *self;
    self = (DeclObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->_writer = PyMem_Malloc(sizeof(_PyUnicodeWriter));
    if (!self->_writer)
        return NULL;
    _PyUnicodeWriter_Init(self->_writer);
    self->_writer->overallocate = 1;
    self->_writer->min_length = 16;
    self->data = NULL;
    self->flags = DECL_FLAGS_MUTABLE;

    DECL_TRACE_INC(TR_ALLOCATED);
    return (PyObject *)self;
}

static int Decl_traverse(DeclObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->data);

    return 0;
}

static int Decl_clear(DeclObject *self) {
    Py_CLEAR(self->data);

    return 0;
}

static inline int _Decl_writer_dealloc(DeclObject *self) {
    if (self->_writer) {
        _PyUnicodeWriter_Dealloc(self->_writer);
        PyMem_Free(self->_writer);
    }
    return 0;
}

static inline int _Decl_writer_finish(DeclObject *self) {
    if (self->data)
        return 0;

    PyObject *str = _PyUnicodeWriter_Finish(self->_writer);
    if (!str)
        return -1;

    self->data = str;
    return 0;
}

static void Decl_dealloc(DeclObject *self) {
    _Decl_writer_dealloc(self);

    Decl_clear(self);

    Py_TYPE(self)->tp_free(self);
    DECL_TRACE_INC(TR_FREED);
}

static PyObject *Decl_repr(DeclObject *self) {
#define DECL_FLAG_STR(NAME, STR)                                               \
    ((self->flags & DECL_FLAGS_##NAME) ? (STR) : "")

    if (_Decl_writer_finish(self) == -1)
        return NULL;
    PyObject *result = PyUnicode_FromFormat(
        "<decl /%U: r%s%s>",
        self->data,
        DECL_FLAG_STR(MUTABLE, "w"),
        DECL_FLAG_STR(SYNCHRONIZED, "s"));

    return result;

#undef DECL_FLAG_STR
}

static PyObject *Decl_str(DeclObject *self) {
    if (_Decl_writer_finish(self) == -1)
        return NULL;
    return PyUnicode_FromFormat("/%U", self->data);
}

static PyObject *Decl_getattro(DeclObject *self, PyObject *name) {
    PyObject *object = NULL;
    if ((object = PyObject_GenericGetAttr((PyObject *)self, name))) {
        return object;
    }
    if (PyUnicode_CompareWithASCIIString(name, "__name__") == 0) {
        return object;
    }
    PyErr_Clear();

    if (self->ob_base.ob_refcnt != 1 || self->data) {
        if (_Decl_writer_finish(self) == -1)
            return NULL;
        PyErr_Format(
            PyExc_RuntimeError,
            "cannot continue a settled declaration '/%S' with .%S",
            self->data,
            name);
        return NULL;
    }

    if (self->_writer->size &&
        _PyUnicodeWriter_WriteChar(self->_writer, '/') < 0) {
        return NULL;
    }
    if (_PyUnicodeWriter_WriteStr(self->_writer, name) < 0) {
        return NULL;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *Decl_call(DeclObject *self, PyObject *args, PyObject *kwds) {
    ARGCHECK_SINGLE_STR
    return Decl_getattro(self, arg);
except:
    return NULL;
}

static PyObject *Decl_getitem(DeclObject *self, PyObject *key) {
    if (key == Py_Ellipsis) {
        if (_Decl_writer_finish(self) == -1)
            return NULL;
        NamespaceObject *v = NEW_OBJECT(NamespaceObject);
        if (!v)
            return NULL;
        Py_INCREF(self->data);
        Py_XSETREF(v->data, self->data);
        return (PyObject *)v;
    }

    if (self->data) {
        PyErr_Format(
            PyExc_RuntimeError,
            "cannot set mode for a settled declaration '/%S' with [%R]",
            self->data,
            key);
        return NULL;
    }

    if (!PyUnicode_CheckExact(key)) {
        PyErr_Format(
            PyExc_TypeError,
            "expect mode to be str, got '%s'",
            key->ob_type->tp_name);
        return NULL;
    }
    const Py_UCS1 *flag_str = PyUnicode_1BYTE_DATA(key);
    unsigned long  flags = 0;
    int            mutable_set = 0;
    while (*flag_str) {
        switch (*flag_str) {
        case 's':
            if (flags & DECL_FLAGS_SYNCHRONIZED)
                goto error;
            flags |= DECL_FLAGS_SYNCHRONIZED;
            break;
        case 'w':
            if (mutable_set)
                goto error;
            mutable_set = 1;
            flags |= DECL_FLAGS_MUTABLE;
            break;
        case 'r':
            if (mutable_set)
                goto error;
            mutable_set = 1;
            flags &= ~DECL_FLAGS_MUTABLE;
            break;

        default:
            goto error;
        }
        flag_str++;
    }

    self->flags = flags;
    if (_Decl_writer_finish(self) == -1)
        return NULL;

    Py_INCREF(self);
    return (PyObject *)self;
error:
    PyErr_Format(PyExc_ValueError, "invalid mode %R", key);
    return NULL;
}

static Py_hash_t Decl_hash(DeclObject *self) {
    if (_Decl_writer_finish(self) == -1)
        return -1;

    return PyObject_Hash(self->data);
}

static PyObject *Decl_richcompare(DeclObject *self, PyObject *other, int op) {
    if (Py_TYPE(other) != &DeclObject_Type || !(op == Py_EQ || op == Py_NE))
        Py_RETURN_NOTIMPLEMENTED;

    if (_Decl_writer_finish(self) == -1 ||
        _Decl_writer_finish((DeclObject *)other) == -1)
        return NULL;

    return PyObject_RichCompare(self->data, ((DeclObject *)other)->data, op);
}

static PyObject *Decl_subtract(DeclObject *self, PyObject *other) {
    if (_Decl_writer_finish(self) == -1)
        return NULL;

    if (!PyLong_CheckExact(other) || _PyLong_AsInt(other) != 0) {
        PyErr_Format(
            PyExc_RuntimeError,
            "unknown modifier '- %R'. Perhaps you mean '- 0'?",
            other);
        return NULL;
    }

    self->flags |= DECL_FLAGS_NONSTRICT;
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyNumberMethods Decl_as_number = {
    .nb_subtract = (binaryfunc)Decl_subtract,
};

static PyMappingMethods Decl_as_mapping = {
    .mp_subscript = (binaryfunc)Decl_getitem,
};

PyTypeObject DeclObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "_scopedvar.Declaration",
    .tp_doc = "Decl",
    .tp_basicsize = sizeof(DeclObject),
    .tp_as_mapping = &Decl_as_mapping,
    .tp_as_number = &Decl_as_number,
    .tp_hash = (hashfunc)Decl_hash,
    .tp_richcompare = (richcmpfunc)Decl_richcompare,
    .tp_dealloc = (destructor)Decl_dealloc,
    .tp_getattro = (getattrofunc)Decl_getattro,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_traverse = (traverseproc)Decl_traverse,
    .tp_repr = (reprfunc)Decl_repr,
    .tp_call = (ternaryfunc)Decl_call,
    .tp_str = (unaryfunc)Decl_str,
    .tp_clear = (inquiry)Decl_clear,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = Decl_new,
};

DeclObject *Decl_New(PyObject *const *names, Py_ssize_t nargs) {
    DeclObject *self = NEW_OBJECT(DeclObject);
    if (!self)
        return NULL;
    for (Py_ssize_t i = 0; i < nargs; i++) {
        if (_PyUnicodeWriter_WriteStr(self->_writer, names[i]) < 0)
            return NULL;
        if (i != nargs - 1 &&
            _PyUnicodeWriter_WriteChar(self->_writer, '/') < 0)
            return NULL;
    }
    return self;
}

PyObject *Decl_GetString(DeclObject *self) {
    if (_Decl_writer_finish(self) < 0)
        return NULL;
    return self->data;
}