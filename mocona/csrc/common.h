#ifndef _SCOPEDVAR_COMMON_H
#define _SCOPEDVAR_COMMON_H

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#include "pycore/hamt.h"
#include "pycore/port.h"

#define DBG(x)                                                                 \
    do {                                                                       \
        printf(x "\n");                                                        \
        fflush(stdout);                                                        \
    } while (0);

#define PyVarObject_HEAD_INIT2(type, size) PyObject_HEAD_INIT(type) size
#define PyObject_HEAD2 PyObject ob_base

#define NEW_OBJECT(TYPE)                                                       \
    (TYPE *)PyObject_CallFunction((PyObject *)&TYPE##_Type, NULL)

#define ARGCHECK_SINGLE_STR                                                    \
    PyObject *   arg = NULL;                                                   \
    static char *empty_kwnames[] = {"name", NULL};                             \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|", empty_kwnames, &arg))   \
        goto except;

#endif