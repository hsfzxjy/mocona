#ifndef _SCOPEDVAR_CTXMGR_H
#define _SCOPEDVAR_CTXMGR_H

#include "common.h"

#include "scopestackobject.h"

PROTECTED PyTypeObject _ctxmgr_Type;

typedef struct {
    PyObject_HEAD2;

    scopeinitspec spec;
} _ctxmgr;

PROTECTED PyObject *_ctxmgr_New(scopeinitspec *spec);

#endif