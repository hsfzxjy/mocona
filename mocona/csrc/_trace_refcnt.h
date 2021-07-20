#ifndef _SCOPEDVAR_TRACE_REFCNT_H
#define _SCOPEDVAR_TRACE_REFCNT_H

#define TR_FREED 0
#define TR_ALLOCATED 1

#ifdef TRACE_REFCNT
#define _VAR_FOR(T, NAME)                                                      \
    (*((T) == TR_FREED ? &NAME##_FREED : &NAME##_ALLOCATED))

#define DEFINE_REFCNT_TRACE_FOR(NAME)                                          \
    PROTECTED uint64_t NAME##_FREED;                                           \
    PROTECTED uint64_t NAME##_ALLOCATED;                                       \
    static inline void NAME##_TRACE_INC(int type_) {                           \
        _VAR_FOR(type_, NAME)++;                                               \
    }                                                                          \
    static inline uint64_t NAME##_TRACE_GET(int type_) {                       \
        return _VAR_FOR(type_, NAME);                                          \
    }                                                                          \
    static inline void NAME##_TRACE_RESET() {                                  \
        _VAR_FOR(TR_FREED, NAME) = 0;                                          \
        _VAR_FOR(TR_ALLOCATED, NAME) = 0;                                      \
    }

#define TRACE_REFCNT_INIT(NAME)                                                \
    uint64_t NAME##_FREED = 0;                                                 \
    uint64_t NAME##_ALLOCATED = 0;

#define _TR_MATCH_UNICODE(NAME, T, TS, OP)                                     \
    do {                                                                       \
        if (PyUnicode_CompareWithASCIIString(str, #NAME "_" TS) == 0)          \
            OP(_VAR_FOR(T, NAME))                                              \
    } while (0);

#define TR_MATCH_UNICODE(NAME, OP)                                             \
    do {                                                                       \
        _TR_MATCH_UNICODE(NAME, TR_ALLOCATED, "ALLOCATED", OP);                \
        _TR_MATCH_UNICODE(NAME, TR_FREED, "FREED", OP);                        \
    } while (0);
#else
#define DEFINE_REFCNT_TRACE_FOR(NAME)                                          \
    static inline void     NAME##_TRACE_INC(int type_) {}                      \
    static inline uint64_t NAME##_TRACE_GET(int type_) { return 0; }           \
    static inline void     NAME##_TRACE_RESET() {}

#define TRACE_REFCNT_INIT(NAME)
#endif

#endif