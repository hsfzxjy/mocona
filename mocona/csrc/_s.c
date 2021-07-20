#include "_scopedvar_module.h"

static void _S_dealloc(_S *self) { Py_TYPE(self)->tp_free(self); }

#define DECLARE_SCOPE_FUNCTION(NAME, G, B, T)                                  \
    static PyObject *_S_##NAME(PyObject *self, PyObject *args) {               \
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

static PyObject *
_S_assign(PyObject *self, PyObject *const *args, Py_ssize_t nargs) {
    if (!_PyArg_CheckPositional("assign", nargs, 2, 2)) {
        return NULL;
    }
    if (Py_TYPE(args[0]) != &VarObject_Type) {
        PyErr_SetString(PyExc_TypeError, "var should be a Var object");
        return NULL;
    }
    VarObject *var = (VarObject *)args[0];
    PyObject * rhs = args[1];
    if (Var_Assign(var, rhs) < 0)
        return NULL;
    Py_INCREF(var);
    return (PyObject *)var;
}

static PyObject *_S__varfor(PyObject *self, PyObject *decl) {
    if (Py_TYPE(decl) != &DeclObject_Type) {
        PyErr_Format(
            PyExc_TypeError,
            "expect a decl object, got %s",
            decl->ob_type->tp_name);
        return NULL;
    }
    return ScopeStack_GetVar(_scopedvar_current_stack, (DeclObject *)decl);
}

static PyObject *_S_isvar(PyObject *self, PyObject *arg) {
    if (Py_TYPE(arg) == &VarObject_Type)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *_S_isempty(PyObject *self, PyObject *arg) {
    if (Py_TYPE(arg) != &VarObject_Type) {
        PyErr_SetString(PyExc_RuntimeError, "expect a Var");
        return NULL;
    }
    switch (Var_IsEmpty(Var_CAST(arg))) {
    case 0:
        Py_RETURN_FALSE;
    case 1:
        Py_RETURN_TRUE;
    case -1:
    default:
        return NULL;
    }
}

#define DECLARE_SCOPE_FUNCTION_ENTRY(NAME)                                     \
    { #NAME, (PyCFunction)_S_##NAME, METH_VARARGS, 0 }

static PyMethodDef methods[] = {
    {"assign", (PyCFunction)_S_assign, METH_FASTCALL, 0},
    {"isvar", (PyCFunction)_S_isvar, METH_O, 0},
    {"isempty", (PyCFunction)_S_isempty, METH_O, 0},
    {"_varfor", (PyCFunction)_S__varfor, METH_O, 0},
    DECLARE_SCOPE_FUNCTION_ENTRY(patch),
    DECLARE_SCOPE_FUNCTION_ENTRY(patch_local),
    DECLARE_SCOPE_FUNCTION_ENTRY(shield),
    DECLARE_SCOPE_FUNCTION_ENTRY(shield_local),
    DECLARE_SCOPE_FUNCTION_ENTRY(isolate),
    DECLARE_SCOPE_FUNCTION_ENTRY(isolate_local),
    {NULL},
};

PyTypeObject _S_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "_S",
    .tp_doc = "_S",
    .tp_basicsize = sizeof(_S),
    .tp_dealloc = (destructor)_S_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,

    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
    .tp_methods = methods,
};

_S *S_instance = NULL;
int _S_Init() {
    if (PyType_Ready(&_S_Type) < 0)
        return -1;
    if (!(S_instance = NEW_OBJECT(_S)))
        return -1;
    return 0;
}