#ifndef Py_INTERNAL_HAMT_H
#define Py_INTERNAL_HAMT_H

#include "port.h"
#undef _PyObject_GC_TRACK
#define _PyObject_GC_TRACK(op) PyObject_GC_Track(op)

#define _Py_HAMT_MAX_TREE_DEPTH 7


#define PyHamt_Check(o) (Py_TYPE(o) == &_PyHamt_Type)


/* Abstract tree node. */
typedef struct {
    PyObject_HEAD
} PyHamtNode;


/* An HAMT immutable mapping collection. */
typedef struct {
    PyObject_HEAD
    PyHamtNode *h_root;
    PyObject *h_weakreflist;
    Py_ssize_t h_count;
} PyHamtObject;


/* A struct to hold the state of depth-first traverse of the tree.

   HAMT is an immutable collection.  Iterators will hold a strong reference
   to it, and every node in the HAMT has strong references to its children.

   So for iterators, we can implement zero allocations and zero reference
   inc/dec depth-first iteration.

   - i_nodes: an array of seven pointers to tree nodes
   - i_level: the current node in i_nodes
   - i_pos: an array of positions within nodes in i_nodes.
*/
typedef struct {
    PyHamtNode *i_nodes[_Py_HAMT_MAX_TREE_DEPTH];
    Py_ssize_t i_pos[_Py_HAMT_MAX_TREE_DEPTH];
    int8_t i_level;
} PyHamtIteratorState;


/* Base iterator object.

   Contains the iteration state, a pointer to the HAMT tree,
   and a pointer to the 'yield function'.  The latter is a simple
   function that returns a key/value tuple for the 'Items' iterator,
   just a key for the 'Keys' iterator, and a value for the 'Values'
   iterator.
*/
typedef struct {
    PyObject_HEAD
    PyHamtObject *hi_obj;
    PyHamtIteratorState hi_iter;
    binaryfunc hi_yield;
} PyHamtIterator;


PROTECTED PyTypeObject _PyHamt_Type;
PROTECTED PyTypeObject _PyHamt_ArrayNode_Type;
PROTECTED PyTypeObject _PyHamt_BitmapNode_Type;
PROTECTED PyTypeObject _PyHamt_CollisionNode_Type;
PROTECTED PyTypeObject _PyHamtKeys_Type;
PROTECTED PyTypeObject _PyHamtValues_Type;
PROTECTED PyTypeObject _PyHamtItems_Type;


/* Create a new HAMT immutable mapping. */
PROTECTED PyHamtObject * _PyHamt_New(void);

/* Return a new collection based on "o", but with an additional
   key/val pair. */
PROTECTED PyHamtObject * _PyHamt_Assoc(PyHamtObject *o, PyObject *key, PyObject *val);

/* Return a new collection based on "o", but without "key". */
PROTECTED PyHamtObject * _PyHamt_Without(PyHamtObject *o, PyObject *key);

/* Find "key" in the "o" collection.

   Return:
   - -1: An error occurred.
   - 0: "key" wasn't found in "o".
   - 1: "key" is in "o"; "*val" is set to its value (a borrowed ref).
*/
PROTECTED int _PyHamt_Find(PyHamtObject *o, PyObject *key, PyObject **val);

/* Check if "v" is equal to "w".

   Return:
   - 0: v != w
   - 1: v == w
   - -1: An error occurred.
*/
PROTECTED int _PyHamt_Eq(PyHamtObject *v, PyHamtObject *w);

/* Return the size of "o"; equivalent of "len(o)". */
PROTECTED Py_ssize_t _PyHamt_Len(PyHamtObject *o);

/* Return a Keys iterator over "o". */
PROTECTED PyObject * _PyHamt_NewIterKeys(PyHamtObject *o);

/* Return a Values iterator over "o". */
PROTECTED PyObject * _PyHamt_NewIterValues(PyHamtObject *o);

/* Return a Items iterator over "o". */
PROTECTED PyObject * _PyHamt_NewIterItems(PyHamtObject *o);

PROTECTED int _PyHamt_Init(void);
PROTECTED void _PyHamt_Fini(void);

#endif /* !Py_INTERNAL_HAMT_H */
