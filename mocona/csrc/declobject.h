#ifndef _SCOPEDVAR_DECLOBJECT_H
#define _SCOPEDVAR_DECLOBJECT_H

#include "common.h"

PROTECTED PyTypeObject DeclObject_Type;

typedef struct {
    PyObject_HEAD;

    unsigned long     flags;
    PyObject *        dotted_name;
    _PyUnicodeWriter *_writer;
} DeclObject;

#define DECL_FLAGS_MUTABLE (1UL << 1)
#define DECL_FLAGS_SYNCHRONIZED (1UL << 2)

#endif