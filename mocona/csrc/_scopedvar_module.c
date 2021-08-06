#include "_scopedvar_module.h"

#ifdef TRACE_REFCNT
#define _RETURN_LONG(x) return PyLong_FromUnsignedLongLong(x);
#define TR_MATCH_UNICODE_RETURN_LONG(NAME) TR_MATCH_UNICODE(NAME, _RETURN_LONG)
static PyObject *_scopedvar_get_refcnt_stats(PyObject *mod, PyObject *str) {
    TR_MATCH_UNICODE_RETURN_LONG(CELL)
    TR_MATCH_UNICODE_RETURN_LONG(DECL)
    TR_MATCH_UNICODE_RETURN_LONG(SCOPE)
    TR_MATCH_UNICODE_RETURN_LONG(NAMESPACE)
    return NULL;
}

static PyObject *_scopedvar_reset_refcnt_stats(PyObject *mod) {
    CELL_TRACE_RESET();
    DECL_TRACE_RESET();
    SCOPE_TRACE_RESET();
    NAMESPACE_TRACE_RESET();
    Py_RETURN_NONE;
}
#endif

static PyMethodDef methods[] = {
#ifdef TRACE_REFCNT
    {"get_refcnt_stats", (PyCFunction)_scopedvar_get_refcnt_stats, METH_O, 0},
    {"reset_refcnt_stats",
     (PyCFunction)_scopedvar_reset_refcnt_stats,
     METH_NOARGS,
     0},
#endif
    {NULL},
};

static int _scopedvar_clear(PyObject *m) {
    _PyHamt_Fini();
    return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_scopedvar",
    .m_doc = "",
    .m_size = -1,
    .m_methods = methods,
    .m_clear = _scopedvar_clear,
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
        PyType_Ready(&_ctxmgr_Type) < 0 || _S_Init() < 0 ||
        NamespaceObject_Init() < 0 || !_PyHamt_Init())
        goto except;

    _scopedvar_current_stack = NEW_OBJECT(ScopeStackObject);
    if (!_scopedvar_current_stack)
        goto except;

    ADD_OBJECT("V", V_instance);
    ADD_OBJECT("S", S_instance);

    return module;
except:
    Py_XDECREF(module);
    return NULL;
}

PyMODINIT_FUNC PyInit__scopedvar(void) { return moduleinit(); }
