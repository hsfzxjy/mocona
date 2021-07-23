#include "Python.h"
#include "hamt/pycore_hamt.h"
#include "structmember.h"

#define PROTECTED extern __attribute__((visibility("hidden")))

#define DBG(x)                                                                 \
    do {                                                                       \
        printf(x "\n");                                                        \
        fflush(stdout);                                                        \
    } while (0);

#define PyVarObject_HEAD_INIT2(type, size) PyObject_HEAD_INIT(type) size

#define NEW_OBJECT(TYPE)                                                       \
    (TYPE *)PyObject_CallFunction((PyObject *)&TYPE##_Type, NULL)

#define ARGCHECK_SINGLE_STR                                                    \
    PyObject *   arg = NULL;                                                   \
    static char *empty_kwnames[] = {"name", NULL};                             \
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|", empty_kwnames, &arg))   \
        goto except;
