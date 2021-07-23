#include "_scopedvar_module.h"

static PyObject *
ScopeStack_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    ScopeStackObject *self;

    self = (ScopeStackObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    scopeinitspec spec = {
        .is_global = 1,
        .is_bottom = 1,
        .is_transparent = 0,
    };
    self->top_global_scope = Scope_New(&spec, NULL);
    self->ctxvar_scope = PyContextVar_New("Scope", NULL);
    self->stack_ver = 1;

    return (PyObject *)self;
}

static int
ScopeStack_traverse(ScopeStackObject *self, visitproc visit, void *arg) {
    ScopeObject *scope = self->top_global_scope;
    while (scope) {
        Py_VISIT(scope);
        scope = scope->f_prev;
    }
    Py_VISIT(self->ctxvar_scope);

    return 0;
}

static int ScopeStack_clear(ScopeStackObject *self) {
    ScopeObject *scope = self->top_global_scope;
    while (scope) {
        ScopeObject *prev = scope->f_prev;
        Py_CLEAR(scope);
        scope = prev;
    }
    Py_CLEAR(self->ctxvar_scope);

    return 0;
}

static void ScopeStack_dealloc(ScopeStackObject *self) {
    PyObject_GC_UnTrack(self);
    ScopeStack_clear(self);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *ScopeStack_getvar(ScopeStackObject *self, PyObject *decl) {
    return ScopeStack_GetVar(self, (DeclObject *)decl);
}

static PyMethodDef ScopeStack_methods[] = {
    {"getvar", (PyCFunction)ScopeStack_getvar, METH_O, 0},
    {NULL},
};

PyTypeObject ScopeStackObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "_scopedvar.ScopeStack",
    .tp_doc = "ScopeStack",
    .tp_basicsize = sizeof(ScopeStackObject),
    .tp_dealloc = (destructor)ScopeStack_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,

    .tp_traverse = (traverseproc)ScopeStack_traverse,
    .tp_clear = (inquiry)ScopeStack_clear,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = ScopeStack_new,
    .tp_free = PyObject_GC_Del,

    .tp_methods = ScopeStack_methods,
};

static inline int _is_main_thread() {
    static unsigned long main_thread_ident;
    static int           initialized = 0, result;

    PyObject *threading_module = NULL, *main_thread = NULL, *main_ident = NULL;

    if (!initialized) {
        threading_module = PyImport_ImportModule("threading");
        if (!threading_module)
            goto except;
        main_thread = PyObject_GetAttrString(threading_module, "_main_thread");
        if (!main_thread)
            goto except;
        main_ident = PyObject_GetAttrString(main_thread, "ident");
        if (!main_ident)
            goto except;
        main_thread_ident = PyLong_AsUnsignedLong(main_ident);
        initialized = 1;
    }
    result = PyThread_get_thread_ident() == main_thread_ident;
    goto done;
except:
    result = -1;
done:
    Py_XDECREF(threading_module);
    Py_XDECREF(main_thread);
    Py_XDECREF(main_ident);
    return result;
}

#define ENSURE_MAIN_THREAD(ERR_TEXT)                                           \
    do {                                                                       \
        int result = _is_main_thread();                                        \
        switch (result) {                                                      \
        case 0:                                                                \
            PyErr_SetString(PyExc_RuntimeError, (ERR_TEXT));                   \
        case -1:                                                               \
            goto except;                                                       \
        }                                                                      \
    } while (0);

static inline int
ScopeStack_EnterGlobalScope(ScopeStackObject *self, scopeinitspec *spec) {

    ScopeObject *new_scope = NULL;

    ENSURE_MAIN_THREAD("cannot enter a global scope outside the main thread")

    if (self->top_global_scope && SCOPE_HAS_NEXT(self->top_global_scope)) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot enter a global scope with more than one local scope alive");
        goto except;
    }
    new_scope = Scope_New(spec, self->top_global_scope);
    if (!new_scope)
        goto except;

    if (Scope_AddCells(new_scope, spec) < 0)
        goto except;

    Py_XSETREF(self->top_global_scope, new_scope);
    self->stack_ver++;
    return 0;
except:
    Py_XDECREF(new_scope);
    return -1;
}

static inline int ScopeStack_ExitGlobalScope(ScopeStackObject *self) {
    ENSURE_MAIN_THREAD("cannot exit a global scope outside the main thread")
    if (!self->top_global_scope) {
        PyErr_SetString(PyExc_RuntimeError, "no global scope to exit");
        goto except;
    }
    if (!self->top_global_scope->f_prev) {
        PyErr_SetString(
            PyExc_RuntimeError, "cannot exit the outermost global scope");
        goto except;
    }
    if (SCOPE_HAS_NEXT(self->top_global_scope)) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot exit a global scope with more than one local scope alive");
        goto except;
    }
    ScopeObject *prev = self->top_global_scope->f_prev;
    Py_INCREF(prev);
    Py_DECREF(self->top_global_scope);
    SCOPE_XDECREF(prev);
    self->top_global_scope = prev;

    self->stack_ver++;
    return 0;
except:
    return -1;
}

static inline int
ScopeStack_EnterLocalScope(ScopeStackObject *self, scopeinitspec *spec) {

    PyObject *   new_ctx = NULL;
    ScopeObject *new_scope = NULL, *prev_scope = NULL;

    if (PyContextVar_Get(self->ctxvar_scope, NULL, (PyObject **)&prev_scope) <
        0)
        goto except;

    if (prev_scope == NULL) {
        prev_scope = self->top_global_scope;
    } else {
        Py_DECREF(prev_scope);
    }

    new_scope = Scope_New(spec, prev_scope);
    if (!new_scope)
        goto except;

    if (Scope_AddCells(new_scope, spec) < 0)
        goto except;

    PyObject *tok = PyContextVar_Set(self->ctxvar_scope, (PyObject *)new_scope);
    if (!tok)
        goto except;
    new_scope->token = tok;
    self->stack_ver++;
    return 0;
except:
    Py_XDECREF(new_scope);
    Py_XDECREF(new_ctx);

    return -1;
}

static inline int ScopeStack_ExitLocalScope(ScopeStackObject *self) {
    ScopeObject *cur_scope = NULL;
    if (PyContextVar_Get(self->ctxvar_scope, NULL, (PyObject **)&cur_scope) < 0)
        goto except;
    if (!cur_scope) {
        PyErr_SetString(PyExc_RuntimeError, "no local scope to exit");
        goto except;
    }
    Py_DECREF(cur_scope);
    if (SCOPE_HAS_NEXT(cur_scope)) {
        PyErr_Format(
            PyExc_RuntimeError,
            "cannot exit a local scope with more than one subsequent scope "
            "alive. Number of subsequent scopes: %lu",
            cur_scope->f_refcnt);
        goto except;
    }

    if (cur_scope->token) {
        if (PyContextVar_Reset(self->ctxvar_scope, cur_scope->token) < 0)
            goto except;
        Py_SETREF(cur_scope->token, NULL);
    }
    SCOPE_XDECREF(cur_scope->f_prev);
    Py_DECREF(cur_scope);
    self->stack_ver++;
    return 0;
except:
    return -1;
}

int ScopeStack_EnterScope(ScopeStackObject *self, scopeinitspec *spec) {
    switch (spec->is_global) {
    case 0:
        return ScopeStack_EnterLocalScope(self, spec);
    case 1:
        return ScopeStack_EnterGlobalScope(self, spec);
    default:
        return -1;
    }
}

int ScopeStack_ExitScope(ScopeStackObject *self, scopeinitspec *spec) {
    switch (spec->is_global) {
    case 0:
        return ScopeStack_ExitLocalScope(self);
    case 1:
        return ScopeStack_ExitGlobalScope(self);
    default:
        return -1;
    }
}

CellObject *ScopeStack_FindCell(ScopeStackObject *self, PyObject *decl) {
    ScopeObject *scope = NULL;
    if (PyContextVar_Get(self->ctxvar_scope, NULL, (PyObject **)&scope) < 0)
        goto except;
    if (!scope)
        scope = self->top_global_scope;
    else
        Py_DECREF(scope);

    return Scope_FindCell(scope, decl);
except:
    return NULL;
}

PROTECTED PyObject *
          ScopeStack_GetVar(ScopeStackObject *self, DeclObject *decl) {
    return (PyObject *)Var_New(self, decl);
}
