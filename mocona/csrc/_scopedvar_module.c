#include "_scopedvar_module.h"

static PyObject *
_scopedvar_assign(PyObject *self, PyObject *const *args, Py_ssize_t nargs) {
    if (!_PyArg_CheckPositional("assign", nargs, 2, 2)) {
        return NULL;
    }
    if (Py_TYPE(args[0]) != &VarObject_Type) {
        PyErr_SetString(PyExc_TypeError, "var should be a Var object");
        return NULL;
    }
    VarObject *var = (VarObject *)args[0];
    PyObject * wrapped = args[1];
    if (!var->mutable) {
        PyErr_SetString(PyExc_RuntimeError, "var is not mutable");
        return NULL;
    }
    ENSURE_VAR_CACHE_UPDATED(var, NULL)
    if (Cell_Assign(var->cell, wrapped) < 0)
        return NULL;
    Py_RETURN_NONE;
}

#define DECLARE_SCOPE_FUNCTION(NAME, G, B, T)                                  \
    static PyObject *_scopedvar_##NAME(PyObject *self, PyObject *args) {       \
        scopeinitspec spec = {                                                 \
            .is_global = (G),                                                  \
            .is_bottom = (B),                                                  \
            .is_transparent = (T),                                             \
        };                                                                     \
                                                                               \
        PyObject *arg0;                                                        \
        if (PyTuple_GET_SIZE(args) == 1 &&                                     \
            PyDict_CheckExact(arg0 = PyTuple_GET_ITEM(args, 0))) {             \
            Py_INCREF(arg0);                                                   \
            spec.container = arg0;                                             \
            spec.container_type = ADD_CELLS_CONTAINER_TYPE_DICT;               \
        } else {                                                               \
            Py_INCREF(args);                                                   \
            spec.container = args;                                             \
            spec.container_type = ADD_CELLS_CONTAINER_TYPE_TUPLE;              \
        }                                                                      \
        return _ctxmgr_New(&spec);                                             \
    }

DECLARE_SCOPE_FUNCTION(patch, 1, 0, 0)
DECLARE_SCOPE_FUNCTION(patch_local, 0, 0, 0)
DECLARE_SCOPE_FUNCTION(shield, 1, 1, 1)
DECLARE_SCOPE_FUNCTION(shield_local, 0, 1, 1)
DECLARE_SCOPE_FUNCTION(isolate, 1, 1, 0)
DECLARE_SCOPE_FUNCTION(isolate_local, 0, 1, 0)

#define DECLARE_SCOPE_FUNCTION_ENTRY(NAME)                                     \
    { #NAME, (PyCFunction)_scopedvar_##NAME, METH_VARARGS, 0 }

static PyMethodDef methods[] = {
    {"assign", (PyCFunction)_scopedvar_assign, METH_FASTCALL, 0},
    DECLARE_SCOPE_FUNCTION_ENTRY(patch),
    DECLARE_SCOPE_FUNCTION_ENTRY(patch_local),
    DECLARE_SCOPE_FUNCTION_ENTRY(shield),
    DECLARE_SCOPE_FUNCTION_ENTRY(shield_local),
    DECLARE_SCOPE_FUNCTION_ENTRY(isolate),
    DECLARE_SCOPE_FUNCTION_ENTRY(isolate_local),
    {NULL},
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_scopedvar",
    .m_doc = "",
    .m_size = -1,
    .m_methods = methods,
};

#define ADD_OBJECT(NAME, O)                                                    \
    do {                                                                       \
        Py_INCREF(O);                                                          \
        if (PyModule_AddObject(module, NAME, (PyObject *)(O)) < 0) {           \
            Py_DECREF(O);                                                      \
            Py_DECREF(module);                                                 \
            return NULL;                                                       \
        }                                                                      \
    } while (0);

ScopeStackObject *_scopedvar_current_stack = NULL;

static PyObject *moduleinit(void) {
    PyObject *module;

    module = PyModule_Create(&moduledef);

    if (module == NULL)
        return NULL;

    if (PyType_Ready(&CellObject_Type) < 0 ||
        PyType_Ready(&VarObject_Type) < 0 ||
        PyType_Ready(&DeclObject_Type) < 0 ||
        PyType_Ready(&ScopeObject_Type) < 0 ||
        PyType_Ready(&ScopeStackObject_Type) < 0 ||
        PyType_Ready(&_ctxmgr_Type) < 0)
        return NULL;

    ADD_OBJECT("Cell", &CellObject_Type);
    ADD_OBJECT("Decl", &DeclObject_Type);

    _scopedvar_current_stack = (ScopeStackObject *)PyObject_CallFunction(
        (PyObject *)&ScopeStackObject_Type, NULL);
    if (!_scopedvar_current_stack)
        return NULL;
    ADD_OBJECT("stack", _scopedvar_current_stack);

    return module;
}

PyMODINIT_FUNC PyInit__scopedvar(void) { return moduleinit(); }
