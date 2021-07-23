#include "_scopedvar_module.h"

TRACE_REFCNT_INIT(NAMESPACE)

static PyObject *
NamespaceObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    NamespaceObject *self;
    self = (NamespaceObject *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;

    self->data = NULL;
    NAMESPACE_TRACE_INC(TR_ALLOCATED);
    return (PyObject *)self;
}

static void NamespaceObject_dealloc(NamespaceObject *self) {
    Py_CLEAR(self->data);
    Py_TYPE(self)->tp_free(self);
    NAMESPACE_TRACE_INC(TR_FREED);
}

static PyObject *NamespaceObject_repr(NamespaceObject *self) {
    if (!self->data) {
        return PyUnicode_FromString("<namespace: />");
    } else {
        return PyUnicode_FromFormat("<namespace: /%U/>", self->data);
    }
}

static PyObject *NamespaceObject_str(NamespaceObject *self) {
    if (!self->data) {
        return PyUnicode_FromString("/");
    } else {
        return PyUnicode_FromFormat("/%U/", self->data);
    }
}

static PyObject *
NamespaceObject_getattro(NamespaceObject *self, PyObject *name) {
    return NamespaceObject_GetAttr(self, name);
}

static PyObject *
NamespaceObject_call(NamespaceObject *self, PyObject *args, PyObject *kwds) {
    ARGCHECK_SINGLE_STR
    return NamespaceObject_GetAttr(self, arg);
except:
    return NULL;
}

static PyObject *NamespaceObject_getitem(NamespaceObject *self, PyObject *key) {
    if (key != Py_Ellipsis) {
        PyErr_Format(PyExc_TypeError, "invalid modifier [%R]", key);
        return NULL;
    }
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyMappingMethods NamespaceObject_as_mapping = {
    .mp_subscript = (binaryfunc)NamespaceObject_getitem,
};

static PyMethodDef methods[] = {
    {NULL},
};

PyTypeObject NamespaceObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "_scopedvar.Namespace",
    .tp_doc = "NamespaceObject",
    .tp_basicsize = sizeof(NamespaceObject),
    .tp_dealloc = (destructor)NamespaceObject_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,

    .tp_getattro = (getattrofunc)NamespaceObject_getattro,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = NamespaceObject_new,
    .tp_repr = (unaryfunc)NamespaceObject_repr,
    .tp_str = (unaryfunc)NamespaceObject_str,
    .tp_call = (ternaryfunc)NamespaceObject_call,
    .tp_methods = methods,
    .tp_as_mapping = &NamespaceObject_as_mapping,
};

NamespaceObject *V_instance = NULL;
int              NamespaceObject_Init() {
    if (PyType_Ready(&NamespaceObject_Type) < 0)
        return -1;
    if (!(V_instance = NEW_OBJECT(NamespaceObject)))
        return -1;
    return 0;
}

PyObject *NamespaceObject_GetAttr(NamespaceObject *self, PyObject *name) {
    PyObject * args[2];
    Py_ssize_t nargs = 1;
    if (self->data) {
        args[0] = (PyObject *)self->data;
        nargs++;
    }
    args[nargs - 1] = name;
    return (PyObject *)Decl_New(args, nargs);
}