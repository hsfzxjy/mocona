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
#if (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 8)
    if (!_PyArg_CheckPositional("assign", nargs, 2, 2)) {
        return NULL;
    }
#else
    if (nargs != 2) {
        PyErr_SetString(PyExc_TypeError, "assign() takes exactly 2 arguments");
        return NULL;
    }
#endif
    if (Py_TYPE(args[0]) != &VarObject_Type) {
        PyErr_SetString(PyExc_TypeError, "expect a Var");
        return NULL;
    }
    VarObject *var = (VarObject *)args[0];
    PyObject * rhs = args[1];
    if (Var_Assign(var, rhs) < 0)
        return NULL;
    Py_INCREF(var);
    return (PyObject *)var;
}

static PyObject *
_S_BuildVar(PyObject *self, PyObject *decl_or_ns, PyObject *varname);

static PyObject *_S__varfor(PyObject *self, PyObject *decl) {
    if (Py_TYPE(decl) != &DeclObject_Type) {
        PyErr_Format(
            PyExc_TypeError,
            "expect a declaration, got '%s'",
            decl->ob_type->tp_name);
        return NULL;
    }
    return _S_BuildVar(self, decl, NULL);
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

static PyObject *_S_unwrap(PyObject *self, PyObject *arg) {
    if (Py_TYPE(arg) != &VarObject_Type) {
        PyErr_SetString(PyExc_RuntimeError, "expect a Var");
        return NULL;
    }
    return Var_Unwrap(Var_CAST(arg));
}

static PyObject *_S_inject(PyObject *self, PyObject *arg) {
#define BUILD_VAR_AND_CHECK(OBJ, VARNAME)                                      \
    if (Py_TYPE((OBJ)) != &DeclObject_Type &&                                  \
        Py_TYPE((OBJ)) != &NamespaceObject_Type)                               \
        continue;                                                              \
    PyObject *var = _S_BuildVar(self, (OBJ), (VARNAME));                       \
    if (!var)                                                                  \
        goto except;

    PyFunctionObject *func = (PyFunctionObject *)arg;

    PyObject *defaults = func->func_defaults;
    if (defaults) {
#define CUR_ARGNAME() PyTuple_GET_ITEM(code->co_varnames, defaults_start_at + i)

        PyCodeObject *code = (PyCodeObject *)func->func_code;

        int defaults_count = PyTuple_GET_SIZE(defaults);
        int defaults_start_at = code->co_argcount - defaults_count;
        int i;

        for (i = 0; i < defaults_count; ++i) {
            PyObject *item = PyTuple_GET_ITEM(defaults, i);
            BUILD_VAR_AND_CHECK(item, CUR_ARGNAME());
            if (PyTuple_SetItem(defaults, i, var) < 0) {
                Py_DECREF(var);
                goto except;
            }
        }
#undef CUR_ARGNAME
    }
    defaults = func->func_kwdefaults;
    if (defaults) {
        Py_ssize_t pos = 0;
        PyObject * key, *value;
        while (PyDict_Next(defaults, &pos, &key, &value)) {
            BUILD_VAR_AND_CHECK(value, key);
            if (PyDict_SetItem(defaults, key, var) < 0) {
                Py_DECREF(var);
                goto except;
            }
            Py_DECREF(var);
        }
    }
    goto done;
except:
    return NULL;
done:
    Py_INCREF(func);
    return (PyObject *)func;
#undef BUILD_VAR_AND_CHECK
}

#define DECLARE_SCOPE_FUNCTION_ENTRY(NAME)                                     \
    { #NAME, (PyCFunction)_S_##NAME, METH_VARARGS, 0 }

static PyMethodDef methods[] = {
    {"assign", (PyCFunction)_S_assign, METH_FASTCALL, 0},
    {"isvar", (PyCFunction)_S_isvar, METH_O, 0},
    {"isempty", (PyCFunction)_S_isempty, METH_O, 0},
    {"unwrap", (PyCFunction)_S_unwrap, METH_O, 0},
    {"inject", (PyCFunction)_S_inject, METH_O, 0},
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

static PyObject *
_S_BuildVar(PyObject *self, PyObject *decl_or_ns, PyObject *varname) {
    DeclObject *decl = NULL;
    PyObject *  decl_name = NULL;
    PyObject *  result = NULL;

    int free_decl = 0;

    if (Py_TYPE(decl_or_ns) == &NamespaceObject_Type) {
        decl = (DeclObject *)NamespaceObject_GetAttr(
            (NamespaceObject *)decl_or_ns, varname);
        if (!decl)
            goto except;
        free_decl = 1;

    } else if (Py_TYPE(decl_or_ns) == &DeclObject_Type) {
        decl = (DeclObject *)decl_or_ns;

        if (!varname || decl->flags & DECL_FLAGS_NONSTRICT)
            goto getvar;

        decl_name = Decl_GetString(decl);
        if (!decl_name)
            goto except;

        if (!PyUnicode_Tailmatch(decl_name, varname, 0, PY_SSIZE_T_MAX, 1))
            goto name_mismatch;

        Py_ssize_t name_at =
            PyUnicode_GET_LENGTH(decl_name) - PyUnicode_GET_LENGTH(varname);
        if (name_at > 0 && PyUnicode_READ_CHAR(decl_name, name_at - 1) != '/')
            goto name_mismatch;

        goto getvar;

    } else {
        goto except;
    }

getvar:
    result = ScopeStack_GetVar(_scopedvar_current_stack, (DeclObject *)decl);
    goto done;

name_mismatch:
    PyErr_Format(
        PyExc_NameError,
        "argument name %R mismatches strict declaration '/%S'. "
        "Perhaps you want to\n"
        " 1) suffix `[...]` to make it a namespace, or\n"
        " 2) suffix `- 0` to make it non-strict.",
        varname,
        decl_name);
    goto except;

done:
except:
    if (free_decl)
        Py_XDECREF(decl);
    return result;
}
