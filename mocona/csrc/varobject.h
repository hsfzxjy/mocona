#ifndef _SCOPEDVAR_VAROBJECT_H
#define _SCOPEDVAR_VAROBJECT_H

#include "common.h"

#include "cellobject.h"
#include "declobject.h"
#include "scopestackobject.h"

PROTECTED PyTypeObject VarObject_Type;

#define Var_CAST(OP) ((VarObject *)(OP))

#define ENSURE_VAR_CACHE_UPDATED(SELF, ERRVAL)                                 \
    do {                                                                       \
        CellObject *      cell = NULL;                                         \
        PyThreadState *   ts = NULL;                                           \
        ScopeStackObject *stack = Var_CAST(SELF)->stack;                       \
        if (Var_CAST(SELF)->cached_stack_ver == stack->stack_ver) {            \
            ts = PyThreadState_Get();                                          \
            if (Var_CAST(SELF)->cached_tsid == ts->id &&                       \
                Var_CAST(SELF)->cached_tsver == ts->context_ver)               \
                break;                                                         \
        }                                                                      \
        cell = ScopeStack_FindCell(stack, (PyObject *)Var_CAST(SELF)->decl);   \
        if (!cell) {                                                           \
            return (ERRVAL);                                                   \
        }                                                                      \
        Var_SetCell(Var_CAST(SELF), cell);                                     \
        Var_CAST(SELF)->cached_stack_ver = stack->stack_ver;                   \
        if (ts) {                                                              \
            Var_CAST(SELF)->cached_tsid = ts->id;                              \
            Var_CAST(SELF)->cached_tsver = ts->context_ver;                    \
        }                                                                      \
    } while (0);

#define VAR_TRUE_CELL(SELF, NAME)                                              \
    CellObject *NAME = Var_CAST(SELF)->cell;                                   \
    while (NAME->prev)                                                         \
        NAME = NAME->prev;

typedef struct {
    PyObject_HEAD;

    unsigned char mutable;
    PyObject *  dict;
    CellObject *cell;
    PyObject *  weakreflist;

    DeclObject *      decl;
    ScopeStackObject *stack;
    uint64_t          cached_stack_ver;
    uint64_t          cached_tsid;
    uint64_t          cached_tsver;
} VarObject;

inline CellObject *Var_GetTrueCell(VarObject *self) {
    CellObject *cell = self->cell;
    while (cell->prev)
        cell = cell->prev;
    return cell;
}

PROTECTED int *Var_SetCell(VarObject *self, CellObject *cell);

PROTECTED VarObject *Var_New(ScopeStackObject *stack, DeclObject *decl);

#endif