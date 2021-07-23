#ifndef _SCOPEDVAR_DECLOBJECT_H
#define _SCOPEDVAR_DECLOBJECT_H

#include "common.h"

PROTECTED PyTypeObject DeclObject_Type;

typedef struct {
    PyObject_HEAD;

    unsigned long     flags;
    PyObject *        data;
    _PyUnicodeWriter *_writer;
} DeclObject;

#define DECL_FLAGS_MUTABLE (1UL << 1)
#define DECL_FLAGS_SYNCHRONIZED (1UL << 2)
#define DECL_FLAGS_NONSTRICT (1UL << 3)

PROTECTED DeclObject *Decl_New(PyObject *const *, Py_ssize_t);
PROTECTED PyObject *Decl_GetString(DeclObject *);

#include "_trace_refcnt.h"

DEFINE_REFCNT_TRACE_FOR(DECL)

#endif