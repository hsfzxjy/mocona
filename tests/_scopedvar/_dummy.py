import operator


class _Dummy:
    def __init__(self, value):
        self.value = value if isinstance(value, tuple) else (value,)
        self.immutable = True

    def _build_value(self, *args):
        lst = list(self.value)
        for arg in args:
            if isinstance(arg, _Dummy):
                lst.extend(arg.value)
            else:
                lst.append(arg)

        return tuple(lst)

    def _make_func(name):
        def _func(self, *args):
            return _Dummy(self._build_value(name, *args))

        return _func

    for funcname in (
        "__add__",
        "__sub__",
        "__mul__",
        "__mod__",
        "__divmod__",
        "__pow__",
        "__neg__",
        "__pos__",
        "__abs__",
        "__invert__",
        "__lshift__",
        "__rshift__",
        "__and__",
        "__xor__",
        "__or__",
        "__floordiv__",
        "__truediv__",
        "__matmul__",
        "__contains__",
        "__getitem__",
        "__setitem__",
        "__delitem__",
        "__reversed__",
        "__round__",
    ):
        locals()[funcname] = _make_func(funcname)

    def _make_func(name):
        def _func(self, *args):
            value = self._build_value(name, *args)
            if self.immutable:
                self = _Dummy(value)
                self.immutable = True
            else:
                self.value = value
            return self

        return _func

    for funcname in (
        "__iadd__",
        "__isub__",
        "__imul__",
        "__imod__",
        "__ipow__",
        "__ilshift__",
        "__irshift__",
        "__iand__",
        "__ixor__",
        "__ior__",
        "__ifloordiv__",
        "__itruediv__",
        "__imatmul__",
    ):
        locals()[funcname] = _make_func(funcname)

    def _make_func(name):
        def _func(self, other):
            if not isinstance(other, _Dummy):
                return False
            return getattr(operator, name)(self.value, other.value)

        return _func

    for opname in ("ne", "eq", "le", "lt", "ge", "gt"):
        locals()[f"__{opname}__"] = _make_func(opname)

    del _make_func

    def __int__(self):
        return self.value[0]

    __float__ = __bool__ = __bytes__ = __index__ = __str__ = __complex__ = __int__

    def __len__(self):
        return len(self.value)

    def __enter__(self):
        self.value[0].append("enter")
        return "enter"

    def __exit__(self, *args):
        self.value[0].extend(args)
        return True

    def __mro_entries__(self, bases):
        return (int, _Dummy)
