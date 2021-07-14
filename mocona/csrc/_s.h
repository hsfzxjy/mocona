#ifndef _SCOPEDVAR__S_H
#define _SCOPEDVAR__S_H

#include "common.h"

#include "scopestackobject.h"

PROTECTED PyTypeObject _S_Type;

typedef struct {
    PyObject_HEAD;
} _S;

PROTECTED int _S_Init();
PROTECTED _S *S_instance;

#endif