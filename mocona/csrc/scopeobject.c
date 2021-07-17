#include "_scopedvar_module.h"

#define ACQUIRE_LOCK                                                           \
    if (SCOPE_HAS_ATTR(self, IS_GLOBAL) &&                                     \
        !PyThread_acquire_lock(self->lock, NOWAIT_LOCK)) {                     \
        Py_BEGIN_ALLOW_THREADS;                                                \
        PyThread_acquire_lock(self->lock, WAIT_LOCK);                          \
        Py_END_ALLOW_THREADS;                                                  \
    }

#define RELEASE_LOCK                                                           \
    if (SCOPE_HAS_ATTR(self, IS_GLOBAL))                                       \
        PyThread_release_lock(self->lock);

#define SCOPE_SET_ATTR(SELF, NAME)                                             \
    ((ScopeObject *)SELF)->attr |= (SCOPE_ATTR_##NAME)

static PyObject *Scope_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    ScopeObject *self;

    self = (ScopeObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->cells = NULL;
    self->lock = NULL;
    self->attr = 0;
    self->f_refcnt = 0;
    self->f_prev = NULL;

    return (PyObject *)self;
}

static int Scope_traverse(ScopeObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->cells);
    Py_VISIT(self->f_prev);

    return 0;
}

static int Scope_clear(ScopeObject *self) {
    Py_CLEAR(self->cells);
    Py_CLEAR(self->f_prev);

    return 0;
}

static void Scope_dealloc(ScopeObject *self) {
    PyObject_GC_UnTrack(self);

    if (SCOPE_HAS_ATTR(self, IS_GLOBAL)) {
        PyThread_release_lock(self->lock);
        PyThread_free_lock(self->lock);
    }

    Scope_clear(self);

    Py_TYPE(self)->tp_free(self);
}

PyTypeObject ScopeObject_Type = {
    PyVarObject_HEAD_INIT2(NULL, 0),
    .tp_name = "Scope",
    .tp_doc = "Scope",
    .tp_basicsize = sizeof(ScopeObject),
    .tp_dealloc = (destructor)Scope_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,

    .tp_traverse = (traverseproc)Scope_traverse,
    .tp_clear = (inquiry)Scope_clear,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = Scope_new,
    .tp_free = PyObject_GC_Del,
};

ScopeObject *
Scope_New(scopeinitspec *spec, ScopeObject *prev, PyObject *context) {
    ScopeObject *self = NEW_OBJECT(ScopeObject);

    if (!self)
        goto except;

    if (spec->is_global)
        SCOPE_SET_ATTR(self, IS_GLOBAL);

    if (spec->is_bottom)
        SCOPE_SET_ATTR(self, IS_BOTTOM);

    if (spec->is_transparent)
        SCOPE_SET_ATTR(self, IS_TRANSPARENT);

    if (spec->is_global) {
        if (!(self->lock = PyThread_allocate_lock())) {
            PyErr_SetString(PyExc_MemoryError, "cannot allocate a lock");
            goto except;
        }
    } else {
        assert(prev != NULL);
        self->pycontext = context;
    }

    Py_XINCREF(prev);
    self->f_prev = prev;
    SCOPE_XINCREF(prev);

    if (spec->is_bottom || !prev) {
        if (!(self->cells = _PyHamt_New()))
            goto except;
    } else {
        PyHamtObject *cells = prev->cells;
        Py_INCREF(cells);
        self->cells = cells;
    }

    goto done;
except:
    Py_XDECREF(self);
    return NULL;
done:
    return self;
}

static inline int
_Scope_AssocCell(ScopeObject *self, PyObject *decl, PyObject *cell) {

    PyHamtObject *new_cells = _PyHamt_Assoc(self->cells, decl, cell);

    if (!new_cells)
        goto except;
    Py_XSETREF(self->cells, new_cells);
    return 0;
except:
    return -1;
}

static inline CellObject *_Scope_AddCell(ScopeObject *self, PyObject *decl) {
    PyObject *cell = (PyObject *)Cell_New();
    if (!cell)
        goto except;

    if (_Scope_AssocCell(self, decl, cell) < 0)
        goto except;

    Py_DECREF(cell);
    return (CellObject *)cell;
except:
    Py_XDECREF(cell);
    return NULL;
}

CellObject *Scope_FindCell(ScopeObject *self, PyObject *decl) {
    CellObject *result = NULL, *prev_cell = NULL;

    ScopeObject *cur_scope = self;
    ACQUIRE_LOCK
    do {
        int found = _PyHamt_Find(cur_scope->cells, decl, (PyObject **)&result);
        switch (found) {
        case -1:
            goto except;
        case 1:
            goto done;
        }
        if (SCOPE_HAS_ATTR(cur_scope, IS_BOTTOM))
            break;
        cur_scope = cur_scope->f_prev;
    } while (cur_scope);

    // not found and bottom
    if (!(result = _Scope_AddCell(cur_scope, decl)))
        goto except;

    if (SCOPE_HAS_ATTR(cur_scope, IS_TRANSPARENT) && cur_scope->f_prev) {
        prev_cell = Scope_FindCell(cur_scope->f_prev, decl);
        if (!prev_cell)
            goto except;
        Py_INCREF(prev_cell);
        Py_XSETREF(result->prev, prev_cell);
    }

    goto done;
except:
    Py_XDECREF(result);
    Py_XDECREF(prev_cell);
    RELEASE_LOCK
    return NULL;
done:
    if (self != cur_scope) {
        if (_Scope_AssocCell(self, decl, (PyObject *)result) < 0)
            goto except;
    }
    RELEASE_LOCK
    return result;
}

typedef int (
    *iteritemsproc)(PyObject *, Py_ssize_t *, PyObject **, PyObject **);

static inline int
_Scope_AddCells(ScopeObject *self, PyObject *container, iteritemsproc getter) {

    ACQUIRE_LOCK
    Py_ssize_t    pos = 0;
    PyObject *    cell = NULL, *decl = NULL, *value = NULL;
    PyHamtObject *cells, *new_cells = NULL;
    cells = self->cells;
    Py_INCREF(cells);

    while (getter(container, &pos, &decl, &value)) {
        if (Py_TYPE(decl) == &VarObject_Type) {
            decl = (PyObject *)Var_CAST(decl)->decl;
        } else if (Py_TYPE(decl) != &DeclObject_Type) {
            PyErr_Format(
                PyExc_TypeError,
                "expect a decl object, got '%s'",
                decl->ob_type->tp_name);
            goto except;
        }
        switch (_PyHamt_Find(cells, decl, &cell)) {
        // case 1:
        //     PyErr_Format(PyExc_KeyError, "key %R exists", decl);
        case -1:
            goto except;
        }

        cell = (PyObject *)Cell_New();
        if (!cell)
            goto except;
        if (value && Cell_Assign((CellObject *)cell, value) < 0)
            goto except;

        new_cells = _PyHamt_Assoc(cells, decl, cell);
        if (!new_cells)
            goto except;

        Py_SETREF(cells, new_cells);
    }
    goto done;
except:
    Py_XDECREF(cell);
    if (cells == self->cells)
        Py_DECREF(self->cells);
    else
        Py_XDECREF(new_cells);
    RELEASE_LOCK
    return -1;
done:
    if (self->cells != cells)
        Py_XSETREF(self->cells, cells);
    RELEASE_LOCK
    return 0;
}

static int _tuple_iteritems(
    PyObject *  tuple,
    Py_ssize_t *pos,
    PyObject ** key,
    PyObject ** value) {
    if (*pos >= PyTuple_GET_SIZE(tuple))
        return 0;
    *key = PyTuple_GET_ITEM(tuple, *pos);
    (*pos)++;
    return 1;
}

static iteritemsproc _dict_iteritems = PyDict_Next;

int Scope_AddCells(ScopeObject *self, scopeinitspec *spec) {
    switch (spec->container_type) {
    case ADD_CELLS_CONTAINER_TYPE_NONE:
        return 0;
    case ADD_CELLS_CONTAINER_TYPE_TUPLE:
        return _Scope_AddCells(self, spec->container, _tuple_iteritems);
    case ADD_CELLS_CONTAINER_TYPE_DICT:
        return _Scope_AddCells(self, spec->container, _dict_iteritems);
    default:
        return -1;
    }
}
