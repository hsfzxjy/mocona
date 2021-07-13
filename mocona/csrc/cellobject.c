#include "_scopedvar_module.h"

static PyObject *Cell_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    CellObject *self;
    self = (CellObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->dict = PyDict_New();
    self->wrapped = NULL;
    self->prev = NULL;

    return (PyObject *)self;
}

static int Cell_traverse(CellObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->dict);
    Py_VISIT(self->wrapped);
    Py_VISIT(self->prev);

    return 0;
}

static int Cell_clear(CellObject *self) {
    Py_CLEAR(self->dict);
    Py_CLEAR(self->wrapped);
    Py_CLEAR(self->prev);

    return 0;
}

static void Cell_dealloc(CellObject *self) {
    PyObject_GC_UnTrack(self);

    Cell_clear(self);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *Cell_repr(CellObject *self) {
    if (!self->wrapped) {
        PyErr_SetString(PyExc_ValueError, "cell is empty");
        return NULL;
    }

    return PyUnicode_FromFormat(
        "<%s at %p for %s at %p>",
        Py_TYPE(self)->tp_name,
        self,
        Py_TYPE(self->wrapped)->tp_name,
        self->wrapped);
}

PyTypeObject CellObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "Cell",
    .tp_doc = "Cell",
    .tp_basicsize = sizeof(CellObject),
    .tp_dealloc = (destructor)Cell_dealloc,
    .tp_repr = (unaryfunc)Cell_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,

    .tp_traverse = (traverseproc)Cell_traverse,
    .tp_clear = (inquiry)Cell_clear,
    .tp_dictoffset = offsetof(CellObject, dict),
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = Cell_new,
    .tp_free = PyObject_GC_Del,
};

int Cell_Assign(CellObject *self, PyObject *wrapped) {
    static PyObject *module_str = NULL;
    static PyObject *doc_str = NULL;

    PyObject *object = NULL;

    if (!wrapped) {
        PyErr_SetString(PyExc_RuntimeError, "wrapped is null-ptr");
        goto except;
    }

    Py_INCREF(wrapped);
    Py_XSETREF(self->wrapped, wrapped);

    if (!module_str) {
        module_str = PyUnicode_InternFromString("__module__");
    }

    if (!doc_str) {
        doc_str = PyUnicode_InternFromString("__doc__");
    }

    object = PyObject_GetAttr(wrapped, module_str);

    if (object) {
        if (PyDict_SetItem(self->dict, module_str, object) == -1) {
            PyErr_SetString(PyExc_RuntimeError, "error setting __module__");
            goto except;
        }
        Py_DECREF(object);
    } else
        PyErr_Clear();

    object = PyObject_GetAttr(wrapped, doc_str);

    if (object) {
        if (PyDict_SetItem(self->dict, doc_str, object) == -1) {
            PyErr_SetString(PyExc_RuntimeError, "error setting __doc__");
            goto except;
        }
        Py_DECREF(object);
    } else
        PyErr_Clear();

    goto done;
except:
    Py_XDECREF(object);
    return -1;
done:
    Py_XSETREF(self->prev, NULL);
    return 0;
}

PROTECTED CellObject *Cell_New() { return NEW_OBJECT(CellObject); }