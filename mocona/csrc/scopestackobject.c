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
    self->top_global_scope = Scope_New(&spec, NULL, NULL);
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
        Py_CLEAR(scope);
        scope = scope->f_prev;
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
    return (PyObject *)Var_New(self, (DeclObject *)decl);
}

static PyMethodDef ScopeStack_methods[] = {
    {"getvar", (PyCFunction)ScopeStack_getvar, METH_O, 0},
    {NULL},
};

PyTypeObject ScopeStackObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "ScopeStack",
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
ScopeStack_PushGlobalScope(ScopeStackObject *self, scopeinitspec *spec) {

    ScopeObject *new_scope = NULL;

    ENSURE_MAIN_THREAD("global scope cannot be added in child thread")

    if (self->top_global_scope && SCOPE_HAS_NEXT(self->top_global_scope)) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "global scope cannot be added when any local scope exists");
        goto except;
    }
    new_scope = Scope_New(spec, self->top_global_scope, NULL);
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

static inline int ScopeStack_PopGlobalScope(ScopeStackObject *self) {
    ENSURE_MAIN_THREAD("top global scope cannot be popped in child thread")
    if (!self->top_global_scope) {
        PyErr_SetString(PyExc_RuntimeError, "scope stack empty");
        goto except;
    }
    if (!self->top_global_scope->f_prev) {
        PyErr_SetString(
            PyExc_RuntimeError, "bottom-most scope cannot be popped");
        goto except;
    }
    if (SCOPE_HAS_NEXT(self->top_global_scope)) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "global scope cannot be popped when any local scope exists");
        goto except;
    }
    ScopeObject *prev = self->top_global_scope->f_prev;
    Py_INCREF(prev);
    Py_DECREF(self->top_global_scope);
    self->top_global_scope = prev;

    self->stack_ver++;
    return 0;
except:
    return -1;
}

static inline int
ScopeStack_PushLocalScope(ScopeStackObject *self, scopeinitspec *spec) {

    PyObject *   new_ctx = NULL;
    ScopeObject *new_scope = NULL, *prev_scope = NULL;
    int          entered = 0;

    if (PyContextVar_Get(self->ctxvar_scope, NULL, (PyObject **)&prev_scope) <
        0)
        goto except;

    if (prev_scope == NULL) {
        prev_scope = self->top_global_scope;
    } else {
        Py_DECREF(prev_scope);
    }

    new_ctx = PyContext_CopyCurrent();
    if (!new_ctx)
        goto except;

    new_scope = Scope_New(spec, prev_scope, new_ctx);
    if (!new_scope)
        goto except;

    if (Scope_AddCells(new_scope, spec) < 0)
        goto except;

    if (PyContext_Enter(new_ctx) < 0)
        goto except;
    entered = 1;

    PyObject *tok = PyContextVar_Set(self->ctxvar_scope, (PyObject *)new_scope);
    if (!tok)
        goto except;
    Py_DECREF(tok);

    return 0;
except:
    if (entered) {
        PyContext_Exit(new_ctx);
        // no need to reset
        // Py_XDECREF(PyContextVar_Set(self->ctxvar_scope, prev_scope));
    }
    Py_XDECREF(new_scope);
    Py_XDECREF(new_ctx);

    return -1;
}

static inline int ScopeStack_PopLocalScope(ScopeStackObject *self) {
    ScopeObject *cur_scope = NULL;
    if (PyContextVar_Get(self->ctxvar_scope, NULL, (PyObject **)&cur_scope) < 0)
        goto except;
    if (!cur_scope) {
        PyErr_SetString(PyExc_RuntimeError, "no local scope to pop");
        goto except;
    }
    Py_DECREF(cur_scope);
    if (cur_scope->ob_base.ob_refcnt > 2) {
        PyErr_SetString(
            PyExc_RuntimeError, "current local scope has external references");
        goto except;
    }
    if (PyContext_Exit(cur_scope->pycontext) < 0)
        goto except;

    if (!SCOPE_HAS_ATTR(cur_scope->f_prev, IS_GLOBAL)) {
        PyObject *tok =
            PyContextVar_Set(self->ctxvar_scope, (PyObject *)cur_scope->f_prev);
        if (!tok)
            goto except;
        Py_DECREF(tok);
    }
    Py_DECREF(cur_scope->pycontext);
    Py_DECREF(cur_scope);

    return 0;
except:
    return -1;
}

int ScopeStack_PushScope(ScopeStackObject *self, scopeinitspec *spec) {
    switch (spec->is_global) {
    case 0:
        return ScopeStack_PushLocalScope(self, spec);
    case 1:
        return ScopeStack_PushGlobalScope(self, spec);
    default:
        return -1;
    }
}

int ScopeStack_PopScope(ScopeStackObject *self, scopeinitspec *spec) {
    switch (spec->is_global) {
    case 0:
        return ScopeStack_PopLocalScope(self);
    case 1:
        return ScopeStack_PopGlobalScope(self);
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
