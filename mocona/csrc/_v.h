#ifndef _SCOPEDVAR__V_H
#define _SCOPEDVAR__V_H

#include "common.h"

#include "scopestackobject.h"

PROTECTED PyTypeObject _V_Type;

typedef struct {
    PyObject_HEAD;
} _V;

PROTECTED int _V_Init();
PROTECTED _V *V_instance;

#endif