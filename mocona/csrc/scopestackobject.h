#ifndef _SCOPEDVAR_SCOPESTACKOBJECT_H
#define _SCOPEDVAR_SCOPESTACKOBJECT_H

#include "common.h"
#include "scopeobject.h"

PROTECTED PyTypeObject ScopeStackObject_Type;

typedef struct {
    PyObject_HEAD;

    ScopeObject *top_global_scope;
    PyObject *   ctxvar_scope;
    uint64_t     stack_ver;
} ScopeStackObject;

PROTECTED CellObject *
          ScopeStack_FindCell(ScopeStackObject *self, PyObject *decl);

PROTECTED int ScopeStack_PushScope(ScopeStackObject *self, scopeinitspec *spec);

PROTECTED int ScopeStack_PopScope(ScopeStackObject *self, scopeinitspec *spec);

PROTECTED ScopeStackObject *_scopedvar_current_stack;

#endif