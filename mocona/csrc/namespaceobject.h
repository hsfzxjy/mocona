#ifndef _SCOPEDVAR_NAMESPACE_H
#define _SCOPEDVAR_NAMESPACE_H

#include "common.h"

#include "scopestackobject.h"

PROTECTED PyTypeObject NamespaceObject_Type;

typedef struct {
    PyObject_HEAD;

    PyObject *data;
} NamespaceObject;

PROTECTED int NamespaceObject_Init();
PROTECTED     PyObject *
              NamespaceObject_GetAttr(NamespaceObject *self, PyObject *name);
PROTECTED NamespaceObject *V_instance;

#include "_trace_refcnt.h"

DEFINE_REFCNT_TRACE_FOR(NAMESPACE)

#endif