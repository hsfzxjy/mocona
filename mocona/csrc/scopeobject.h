#ifndef _SCOPEDVAR_SCOPEOBJECT_H
#define _SCOPEDVAR_SCOPEOBJECT_H

#include "cellobject.h"
#include "common.h"

typedef struct {
    int       is_global;
    int       is_bottom;
    int       is_transparent;
    PyObject *container;
    int       container_type;
} scopeinitspec;

#define SCOPE_ATTR_IS_GLOBAL ((1UL << 1))
#define SCOPE_ATTR_IS_BOTTOM ((1UL << 2))
#define SCOPE_ATTR_IS_TRANSPARENT ((1UL << 3))
#define SCOPE_HAS_ATTR(SELF, NAME)                                             \
    ((((ScopeObject *)SELF)->attr & SCOPE_ATTR_##NAME) != 0)

// global scope has next iif refcnt >= 2
// local scope has next iif refcnt >= 3
#define SCOPE_HAS_NEXT(SELF)                                                   \
    (((ScopeObject *)(SELF))->ob_base.ob_refcnt >=                             \
     (Py_ssize_t)(3 - SCOPE_HAS_ATTR(SELF, IS_GLOBAL)))

PROTECTED PyTypeObject ScopeObject_Type;

typedef struct _ScopeObject {
    PyObject_HEAD;

    unsigned long attr;

    struct _ScopeObject *f_prev;
    PyHamtObject *       cells;
    union {
        PyThread_type_lock lock;
        PyObject *         pycontext;
    };
} ScopeObject;

PROTECTED ScopeObject *
          Scope_New(scopeinitspec *spec, ScopeObject *prev, PyObject *context);

#define ADD_CELLS_CONTAINER_TYPE_NONE 0
#define ADD_CELLS_CONTAINER_TYPE_TUPLE 1
#define ADD_CELLS_CONTAINER_TYPE_DICT 2
PROTECTED int Scope_AddCells(ScopeObject *self, scopeinitspec *spec);

PROTECTED CellObject *Scope_FindCell(ScopeObject *self, PyObject *decl);

#endif