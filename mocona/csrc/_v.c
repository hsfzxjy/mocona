#include "_scopedvar_module.h"

static void _V_dealloc(_V *self) { Py_TYPE(self)->tp_free(self); }

static PyObject *_V_getattro(_V *self, PyObject *name) {
    PyObject *decl = (PyObject *)NEW_OBJECT(DeclObject);
    if (!decl)
        return NULL;
    decl = PyObject_GetAttr(decl, name);
    Py_XDECREF(decl);
    return decl;
}

static PyMethodDef methods[] = {
    {NULL},
};

PyTypeObject _V_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "_V",
    .tp_doc = "_V",
    .tp_basicsize = sizeof(_V),
    .tp_dealloc = (destructor)_V_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,

    .tp_getattro = (getattrofunc)_V_getattro,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
    .tp_methods = methods,
};

_V *V_instance = NULL;
int _V_Init() {
    if (PyType_Ready(&_V_Type) < 0)
        return -1;
    if (!(V_instance = NEW_OBJECT(_V)))
        return -1;
    return 0;
}