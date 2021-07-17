import gc
import sys
import pytest

import _scopedvar
from _scopedvar import S, V
from _dummy import _Dummy

from functools import wraps


class RefCntStats:
    def __getattr__(self, name):
        return _scopedvar.get_refcnt_stats(name)

    def get_all(self):
        results = {}
        for name in "CELL DECL SCOPE".split():
            for type_ in "ALLOCATED FREED".split():
                key = f"{name}_{type_}"
                results[key] = getattr(self, key)
        return results

    def reset(self):
        _scopedvar.reset_refcnt_stats()


stats = RefCntStats()
A = None


def isolate_testcase(fn):
    @wraps(fn)
    def _inner(*args, **kwargs):
        global A
        A = S._varfor(V.a["w"])
        with S.isolate():
            gc.collect()
            stats.reset()
            fn(*args, **kwargs)
        A = None

    return _inner


@pytest.mark.parametrize("method", [S.patch, S.patch_local])
@isolate_testcase
def test_patch(method):
    global A
    with method():
        S.assign(A, 1)
    A = None

    assert stats.CELL_ALLOCATED == 1
    assert stats.CELL_FREED == 0
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 1


@pytest.mark.parametrize(
    "method",
    [S.shield, S.shield_local, S.isolate, S.isolate_local],
)
@isolate_testcase
def test_shield(method):
    global A
    S.assign(A, 0)
    with method():
        S.assign(A, 1)
    A = None

    assert stats.CELL_ALLOCATED == 2
    assert stats.CELL_FREED == 1
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 1


@isolate_testcase
def test_value_refcnt():
    value = object()
    global A
    with S.isolate():
        S.assign(A, value)
        with S.patch():
            S.assign(A, value)
            with S.shield():
                S.assign(A, value)
                with S.patch_local():
                    S.assign(A, value)
                    with S.shield_local():
                        S.assign(A, value)
                        with S.isolate_local():
                            S.assign(A, value)
    A = None
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 6
    assert stats.CELL_ALLOCATED == 4
    assert stats.CELL_FREED == 4
    assert stats.DECL_FREED == 1

    assert sys.getrefcount(value) == 2


@isolate_testcase
def test_value_refcnt_2():
    value = object()
    global A
    with S.isolate():
        S.assign(A, value)
        with S.patch(A):
            S.assign(A, value)
            with S.shield():
                S.assign(A, value)
                with S.patch_local(A):
                    S.assign(A, value)
                    with S.shield_local():
                        S.assign(A, value)
                        with S.isolate_local():
                            S.assign(A, value)
    A = None
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 6
    assert stats.CELL_ALLOCATED == 6
    assert stats.CELL_FREED == 6
    assert stats.DECL_FREED == 1

    assert sys.getrefcount(value) == 2


@isolate_testcase
def test_value_refcnt_3():
    value = object()
    global A
    with S.isolate():
        S.assign(A, value)
        with S.patch({A: value}):
            S.assign(A, value)
            with S.shield():
                S.assign(A, value)
                with S.patch_local({A: value}):
                    S.assign(A, value)
                    with S.shield_local():
                        S.assign(A, value)
                        with S.isolate_local():
                            S.assign(A, value)
    A = None
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 6
    assert stats.CELL_ALLOCATED == 6
    assert stats.CELL_FREED == 6
    assert stats.DECL_FREED == 1

    assert sys.getrefcount(value) == 2
