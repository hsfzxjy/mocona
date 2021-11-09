#ifndef _SCOPEDVAR_VAROBJECT_H
#define _SCOPEDVAR_VAROBJECT_H

#include "common.h"

#include "cellobject.h"
#include "declobject.h"
#include "scopestackobject.h"

PROTECTED PyTypeObject VarObject_Type;

#define Var_CAST(OP) ((VarObject *)(OP))

typedef struct {
    PyObject_HEAD2;

    unsigned char mutable;
    CellObject *cell;
    PyObject *  weakreflist;

    DeclObject *      decl;
    ScopeStackObject *stack;
    uint64_t          cached_stack_ver;
    uint64_t          cached_tsid;
    uint64_t          cached_tsver;
} VarObject;

PROTECTED VarObject *Var_New(ScopeStackObject *stack, DeclObject *decl);
PROTECTED int        Var_Assign(VarObject *self, PyObject *rhs);
PROTECTED int        Var_IsEmpty(VarObject *self);
PROTECTED PyObject *Var_Unwrap(VarObject *self);
#endif