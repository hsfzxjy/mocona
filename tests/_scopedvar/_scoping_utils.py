import time
import asyncio
import threading
import contextlib
import inspect

from _scopedvar import S, V


class _Executable:

    arg_num = 0

    def __init__(self, *args):
        self.args = args[: self.arg_num]
        self.exes = args[self.arg_num :]
        self._init()

    def _init(self):
        self.async_ = False

    def __call__(self):
        for exe in self.exes:
            if exe.async_:
                asyncio.run(exe.__acall__())
            else:
                obj = exe()
        for exe in self.exes:
            exe.finalize()

    async def __acall__(self):
        for exe in self.exes:
            await exe.__acall__()
        for exe in self.exes:
            exe.finalize()

    def finalize(self):
        ...


class Thread(_Executable):
    def _target(self):
        super().__call__()

    def __call__(self):
        self._thread = threading.Thread(target=self._target)
        self._thread.start()

    async def __acall__(self):
        raise NotImplementedError

    def finalize(self):
        self._thread.join()


class Sleep(_Executable):
    arg_num = 1

    def __call__(self):
        time.sleep(self.args[0])

    async def __acall__(self):
        await asyncio.sleep(self.args[0])


class Coroutine(_Executable):
    def _init(self):
        self.async_ = True

    def __call__(self):
        raise NotImplementedError


class Gather(_Executable):
    def __call__(self):
        raise NotImplementedError

    async def __acall__(self):
        await asyncio.gather(*(exe.__acall__() for exe in self.exes))


class ShouldBe(_Executable):
    arg_num = 2

    def __call__(self):
        assert self.args[0] == self.args[1], f"{self.args[0]} != {self.args[1]}"

    async def __acall__(self):
        self()


class ShouldEmpty(_Executable):
    arg_num = 1

    def __call__(self):
        return S.isempty(self.args[0])

    async def __acall__(self):
        self()


class Assign(_Executable):
    arg_num = 2

    def __call__(self):
        S.assign(self.args[0], self.args[1])

    async def __acall__(self):
        self()


class Var:
    def __init__(self, name):
        self.var = S._varfor(getattr(V, name)["w"])

    def should_be(self, value):
        return ShouldBe(self.var, value)

    def should_empty(self):
        return ShouldEmpty(self.var)

    def assign(self, value):
        return Assign(self.var, value)


class _ScopeBase(_Executable):
    arg_num = 1
    method = contextlib.nullcontext

    def _get_context(self):
        arg = self.args[0]
        method = self.method
        if isinstance(arg, tuple):
            return method(*(v.var for v in arg))
        elif isinstance(arg, dict):
            return method({k.var: v for k, v in arg.items()})
        elif isinstance(arg, Var):
            return method(arg.var)
        else:
            self.exes = (arg,) + self.exes
            return method()

    def __call__(self):
        with self._get_context():
            super().__call__()

    async def __acall__(self):
        with self._get_context():
            await super().__acall__()


class Patch(_ScopeBase):
    method = S.patch


class PatchLocal(_ScopeBase):
    method = S.patch_local


class Shield(_ScopeBase):
    method = S.shield


class ShieldLocal(_ScopeBase):
    method = S.shield_local


class Isolate(_ScopeBase):
    method = S.isolate


class IsolateLocal(_ScopeBase):
    method = S.isolate_local


def _map_join(mapf, it, joiner=", "):
    return joiner.join(map(mapf, it))


def _get_name(obj, depth=3):
    if isinstance(obj, list):
        if len(obj) == 1:
            return _get_name(obj[0], depth)
        return "[{}]".format(_map_join(lambda x: _get_name(x, depth - 1), obj))
    elif isinstance(obj, _Executable):
        name = obj.__class__.__name__
        if depth > 1:
            items = filter(
                lambda x: not S.isvar(x) and isinstance(x, _Executable),
                obj.args + obj.exes,
            )
        else:
            items = []
        items = _map_join(lambda x: _get_name(x, depth - 1), items)
        return "{}({})".format(name, items)


def mark_async(obj):
    for child in obj.args + obj.exes:
        if S.isvar(child) or not isinstance(child, _Executable):
            continue
        if obj.async_:
            child.async_ = True
        mark_async(child)


def make(*exes):
    glb = inspect.currentframe().f_back.f_globals
    exes = list(exes)
    for obj in exes:
        mark_async(obj)

    name = _get_name(exes)
    cnt = 0
    new_name = name
    while new_name in glb:
        cnt += 1
        new_name = name + "_" + str(cnt)
    name = new_name

    funcname = f"test_{name}"

    def test_func():
        nonlocal exes
        Isolate(*exes)()

    test_func.__qualname__ = funcname
    glb[funcname] = test_func
