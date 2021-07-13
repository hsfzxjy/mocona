#include "_scopedvar_module.h"

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
    self->dotted_name = NULL;
    self->flags = 0;

    return (PyObject *)self;
}

static int Decl_traverse(DeclObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->dotted_name);

    return 0;
}

static int Decl_clear(DeclObject *self) {
    Py_CLEAR(self->dotted_name);

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
    if (self->dotted_name)
        return 0;

    PyObject *str = _PyUnicodeWriter_Finish(self->_writer);
    if (!str)
        return -1;

    self->dotted_name = str;
    return 0;
}

static void Decl_dealloc(DeclObject *self) {
    PyObject_GC_UnTrack(self);

    _Decl_writer_dealloc(self);

    Decl_clear(self);

    Py_TYPE(self)->tp_free(self);
}

#define DECL_FLAG_STR(NAME, STR)                                               \
    ((self->flags & DECL_FLAGS_##NAME) ? (STR) : "")

static PyObject *Decl_repr(DeclObject *self) {
    if (_Decl_writer_finish(self) == -1)
        return NULL;
    PyObject *result = PyUnicode_FromFormat(
        "<decl %U: r%s%s>",
        self->dotted_name,
        DECL_FLAG_STR(MUTABLE, "w"),
        DECL_FLAG_STR(SYNCHRONIZED, "s"));

    return result;
}

static PyObject *Decl_getattro(DeclObject *self, PyObject *name) {
    PyObject *object = NULL;
    if ((object = PyObject_GenericGetAttr((PyObject *)self, name))) {
        return object;
    }
    PyErr_Clear();

    if (self->ob_base.ob_refcnt != 1) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "'decl' object cannot be stored in temporary variables");
        return NULL;
    }

    if (self->dotted_name) {
        PyErr_SetString(PyExc_RuntimeError, "'decl' object is finalized");
        return NULL;
    }

    if (self->_writer->size &&
        _PyUnicodeWriter_WriteChar(self->_writer, '.') < 0) {
        return NULL;
    }
    if (_PyUnicodeWriter_WriteStr(self->_writer, name) < 0) {
        return NULL;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *Decl_getitem(DeclObject *self, PyObject *key) {
    if (!PyUnicode_CheckExact(key)) {
        PyErr_SetString(PyExc_TypeError, "flag must be a str");
        return NULL;
    }
    const Py_UCS1 *flag_str = PyUnicode_1BYTE_DATA(key);
    unsigned long  flags = 0;
    while (*flag_str) {
        switch (*flag_str) {
        case 's':
            if (flags & DECL_FLAGS_SYNCHRONIZED)
                goto error;
            flags |= DECL_FLAGS_SYNCHRONIZED;
            break;
        case 'w':
            if (flags & DECL_FLAGS_MUTABLE)
                goto error;
            flags |= DECL_FLAGS_MUTABLE;
            break;
        default:
            goto error;
        }
        flag_str++;
    }

    self->flags = flags;
    Py_INCREF(self);
    return (PyObject *)self;
error:
    PyErr_Format(PyExc_ValueError, "invalid flag %R", key);
    return NULL;
}

static Py_hash_t Decl_hash(DeclObject *self) {
    if (_Decl_writer_finish(self) == -1)
        return -1;

    return PyObject_Hash(self->dotted_name);
}

static PyObject *Decl_richcompare(DeclObject *self, PyObject *other, int op) {
    if (Py_TYPE(other) != &DeclObject_Type || !(op == Py_EQ || op == Py_NE))
        Py_RETURN_NOTIMPLEMENTED;

    if (_Decl_writer_finish(self) == -1 ||
        _Decl_writer_finish((DeclObject *)other) == -1)
        return NULL;

    return PyObject_RichCompare(
        self->dotted_name, ((DeclObject *)other)->dotted_name, op);
}

static PyMappingMethods Decl_as_mapping = {
    .mp_subscript = (binaryfunc)Decl_getitem,
};

PyTypeObject DeclObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "Decl",
    .tp_doc = "Decl",
    .tp_basicsize = sizeof(DeclObject),
    .tp_as_mapping = &Decl_as_mapping,
    .tp_hash = (hashfunc)Decl_hash,
    .tp_richcompare = (richcmpfunc)Decl_richcompare,
    .tp_dealloc = (destructor)Decl_dealloc,
    .tp_getattro = (getattrofunc)Decl_getattro,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)Decl_traverse,
    .tp_repr = (reprfunc)Decl_repr,
    .tp_clear = (inquiry)Decl_clear,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = Decl_new,
    .tp_free = PyObject_GC_Del,
};