import random
import operator
import contextlib
from itertools import product

import pytest
from mocona.scopedvar import S, V

from _dummy import _Dummy


@contextlib.contextmanager
def _preinit(enable, *vars):
    if enable:
        values = {var: random.random() for var in vars}
        ctxmgr = S.isolate(values)
    else:
        ctxmgr = contextlib.nullcontext()

    with ctxmgr:
        yield

        if enable:
            for var, value in values.items():
                assert var == value


def _params(ops):
    return list(product(ops, [False, True]))


tenary_ops = [
    pow,
    operator.setitem,
]


@pytest.mark.parametrize("op, enable_preinit", _params(tenary_ops))
def test_tenary_ops(op, enable_preinit):
    a = _Dummy(1)
    b = _Dummy(2)
    c = _Dummy(3)
    vara = S._varfor(V.a)
    varb = S._varfor(V.b)
    varc = S._varfor(V.c)
    with _preinit(enable_preinit, vara, varb, varc):
        with S.isolate():
            S.assign(vara, a)
            S.assign(varb, b)
            S.assign(varc, c)
            assert op(a, b, c) == op(vara, varb, varc)
            assert op(a, b, c) == op(vara, varb, c)
            assert op(a, b, c) == op(vara, b, varc)
            assert op(a, b, c) == op(vara, b, c)


binary_ops = [
    operator.getitem,
    operator.delitem,
    operator.add,
    operator.sub,
    operator.mul,
    operator.mod,
    operator.pow,
    operator.lshift,
    operator.rshift,
    operator.and_,
    operator.xor,
    operator.or_,
    operator.floordiv,
    operator.truediv,
    divmod,
    operator.matmul,
    operator.ne,
    operator.eq,
    operator.le,
    operator.lt,
    operator.ge,
    operator.gt,
]


@pytest.mark.parametrize("op, enable_preinit", _params(binary_ops))
def test_binary_ops(op, enable_preinit):
    a = _Dummy(1)
    b = _Dummy(2)
    vara = S._varfor(V.a)
    varb = S._varfor(V.b)
    with _preinit(enable_preinit, vara, varb):
        with S.isolate():
            S.assign(vara, a)
            S.assign(varb, b)
            assert op(a, b) == op(vara, varb)
            assert op(a, b) == op(a, varb)
            assert op(a, b) == op(vara, b)


unary_ops = [
    operator.neg,
    operator.pos,
    operator.abs,
    operator.inv,
    operator.index,
    reversed,
    round,
    (bool, False),
    (int, 42),
    (float, 42.1),
    (complex, 1 + 42j),
    (bytes, b"test"),
    (str, "test"),
]


@pytest.mark.parametrize("op, enable_preinit", _params(unary_ops))
def test_unary_ops(op, enable_preinit):
    if isinstance(op, tuple):
        op, default = op
    else:
        default = 1
    a = _Dummy(default)
    vara = S._varfor(V.a)

    with _preinit(enable_preinit, vara):
        with S.isolate():
            S.assign(vara, a)
            assert op(a) == op(vara)


inplace_binary_ops = [
    operator.iadd,
    operator.isub,
    operator.imul,
    operator.imod,
    operator.ifloordiv,
    operator.itruediv,
    operator.ixor,
    operator.ior,
    operator.iand,
    operator.ipow,
    operator.ilshift,
    operator.irshift,
    operator.imatmul,
]


@pytest.mark.parametrize("op, enable_preinit", _params(inplace_binary_ops))
def test_inplace_binary_ops_immutable(op, enable_preinit):
    a = _Dummy(1)
    b = _Dummy(2)
    a.immutable = True
    vara = S._varfor(V.a)
    varb = S._varfor(V.b)
    with _preinit(enable_preinit, vara, varb):
        with S.isolate():
            S.assign(vara, _Dummy(1))
            S.assign(varb, _Dummy(2))
            assert op(a, b) == op(vara, varb) is vara
            assert op(a, b) == op(a, varb)
            assert op(a, b) == vara
    assert a == _Dummy(1)
    assert b == _Dummy(2)


@pytest.mark.parametrize("op, enable_preinit", _params(inplace_binary_ops))
def test_inplace_binary_ops_mutable(op, enable_preinit):
    a = _Dummy(1)
    b = _Dummy(2)
    a.immutable = False
    vara = S._varfor(V.a)
    varb = S._varfor(V.b)
    with _preinit(enable_preinit, vara, varb):
        with S.isolate():
            S.assign(vara, _Dummy(1))
            vara.immutable = False
            S.assign(varb, _Dummy(2))
            assert op(a, b) == op(vara, varb) is vara
            assert vara != _Dummy(1)
            assert varb == _Dummy(2)

    assert a != _Dummy(1)
    assert b == _Dummy(2)


def test_docstring():
    class _SubDummy(_Dummy):
        "sub dummy"

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _SubDummy)
        assert _SubDummy.__doc__ == var.__doc__
        var.__doc__ = new_doc = "sub dummy 2"
        assert _SubDummy.__doc__ == new_doc


def test_qualname():
    class _SubDummy(_Dummy):
        ...

    target_name = "test_qualname.<locals>._SubDummy"

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _SubDummy)
        assert var.__qualname__ == target_name
        var.__qualname__ = new_name = "new_name"
        assert _SubDummy.__qualname__ == new_name


def test_name():
    import decimal

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, decimal)
        assert var.__name__ == "decimal"
        var.__name__ = new_name = "decimal2"
        assert decimal.__name__ == new_name


def test_module():
    import sys

    class _SubDummy(_Dummy):
        ...

    target_module = _SubDummy.__module__

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _SubDummy)
        assert var.__module__ is target_module
        var.__module__ = sys
        assert _SubDummy.__module__ is sys


def test_class():
    class _SubDummy(_Dummy):
        ...

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _SubDummy)
        assert var.__class__ is type


def test_annotations():
    def _f(a: int) -> float:
        ...

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _f)
        assert var.__annotations__ is _f.__annotations__
        var.__annotations__ = {"a": str}
        assert _f.__annotations__ == {"a": str}


def test_contextmanager():
    lst = []
    obj = _Dummy(lst)

    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, obj)
        with var:
            assert lst == ["enter"]
        assert lst == ["enter", None, None, None]
        del lst[:]
        with var:
            raise RuntimeError
        assert lst[1] is RuntimeError


def test_mro_entries():
    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _Dummy(0))

        class _SubDummy(var):
            ...

        assert _SubDummy.__bases__ == (int, _Dummy)


def test_call():
    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, _Dummy)
        assert var(0) == _Dummy(0)


def test_iter():
    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, [0])
        assert next(iter(var)) == 0

        S.assign(var, 0)
        with pytest.raises(TypeError):
            iter(var)


def test_iternext():
    var = S._varfor(V.a)
    with S.isolate():
        S.assign(var, iter([0]))
        assert next(var) == 0

        S.assign(var, 0)
        with pytest.raises(TypeError):
            iter(var)
