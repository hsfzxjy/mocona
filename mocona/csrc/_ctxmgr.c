#include "_scopedvar_module.h"

static PyObject *
_ctxmgr_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    _ctxmgr *self;
    self = (_ctxmgr *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    return (PyObject *)self;
}

static int _ctxmgr_traverse(_ctxmgr *self, visitproc visit, void *arg) {
    Py_VISIT(self->spec.container);

    return 0;
}

static int _ctxmgr_clear(_ctxmgr *self) {
    Py_CLEAR(self->spec.container);

    return 0;
}

static void _ctxmgr_dealloc(_ctxmgr *self) {
    PyObject_GC_UnTrack(self);

    _ctxmgr_clear(self);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *_ctxmgr_enter(_ctxmgr *self, PyObject *args, PyObject *kwds) {
    if (ScopeStack_EnterScope(_scopedvar_current_stack, &(self->spec)) != 0) {
        if (!PyErr_Occurred())
            PyErr_SetString(
                PyExc_RuntimeError,
                "unknown error occurred when adding a new scope");
        return NULL;
    } else
        Py_RETURN_NONE;
}

static PyObject *_ctxmgr_exit(_ctxmgr *self, PyObject *args, PyObject *kwds) {
    if (ScopeStack_ExitScope(_scopedvar_current_stack, &(self->spec)) != 0) {
        if (!PyErr_Occurred())
            PyErr_SetString(
                PyExc_RuntimeError,
                "unknown error occurred when popped a scope");
        return NULL;
    } else
        Py_RETURN_NONE;
}

static PyMethodDef _ctxmgr_methods[] = {
    {"__enter__", (PyCFunction)_ctxmgr_enter, METH_VARARGS | METH_KEYWORDS, 0},
    {"__exit__", (PyCFunction)_ctxmgr_exit, METH_VARARGS | METH_KEYWORDS, 0},
    {NULL},
};

PyTypeObject _ctxmgr_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "_ctxmgr",
    .tp_doc = "_ctxmgr",
    .tp_basicsize = sizeof(_ctxmgr),
    .tp_dealloc = (destructor)_ctxmgr_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,

    .tp_traverse = (traverseproc)_ctxmgr_traverse,
    .tp_clear = (inquiry)_ctxmgr_clear,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = _ctxmgr_new,
    .tp_free = PyObject_GC_Del,
    .tp_methods = _ctxmgr_methods,
};

PyObject *_ctxmgr_New(scopeinitspec *spec) {
    _ctxmgr *self = NEW_OBJECT(_ctxmgr);
    if (!self)
        return NULL;
    memcpy(&(self->spec), spec, sizeof(scopeinitspec));
    return (PyObject *)self;
}