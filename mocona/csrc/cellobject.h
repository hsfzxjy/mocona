#ifndef _SCOPEDVAR_CELLOBJECT_H
#define _SCOPEDVAR_CELLOBJECT_H

#include "Python.h"
#include "common.h"

PROTECTED PyTypeObject CellObject_Type;

typedef struct _CellObject {
    PyObject_HEAD;
    PyObject *          wrapped;
    PyObject *          dict;
    struct _CellObject *prev;
} CellObject;

PROTECTED int Cell_Assign(CellObject *self, PyObject *wrapped);
PROTECTED CellObject *Cell_New();

#include "_trace_refcnt.h"

DEFINE_REFCNT_TRACE_FOR(CELL)

#endif