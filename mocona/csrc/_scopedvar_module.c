#include "_scopedvar_module.h"

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_scopedvar",
    .m_doc = "",
    .m_size = -1,
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
        PyType_Ready(&_ctxmgr_Type) < 0 || _S_Init() < 0 || _V_Init() < 0)
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
