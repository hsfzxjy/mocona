import typing as T
import sys
import inspect
from weakref import ref, WeakMethod, WeakSet
from collections import defaultdict


EMPTY = object()
join = ".".join


class Listener:

    __slots__ = "f", "weak", "once"

    def __init__(self, f, /, weak: bool, once: bool):
        self.weak = weak
        if weak:
            if inspect.ismethod(f):
                f = WeakMethod(f)
            else:
                f = ref(f)
        self.f = f
        self.once = once

    def unwrap(self):
        if self.weak:
            return self.f()
        else:
            return self.f


def _make_callable_key(f, /):
    if inspect.ismethod(f):
        return (id(f.__self__), id(f.__func__))
    return id(f)


F = T.TypeVar("F")


class _Getter(T.Generic[F]):
    __slots__ = "__emitter", "__names", "__func"

    def __init__(self, emitter: "EventEmitter", func: F):
        self.__func = ref(func)
        self.__emitter = ref(emitter)
        self.__names = []

    def __call__(self, *args, **kwargs):
        if not self.__names:
            return self.__func()(self.__emitter(), *args, **kwargs)
        else:
            return self.__func()(self.__emitter(), join(self.__names), *args, **kwargs)

    def __getattr__(self, name) -> T.Union["_Getter[F]", F]:
        self.__names.append(name)
        return self


class _DottableMethod(T.Generic[F]):

    __slots__ = ("__func",)

    def __init__(self, func: F):
        self.__func = func

    def __get__(self, instance, cls) -> T.Union[_Getter[F], F]:
        return _Getter(instance, self.__func)


class EventEmitter:
    __slots__ = (
        "__listeners",
        "__call_listeners",
        "__reversed_mapping",
        "__name",
        "__weakref__",
    )

    def __init__(self, name: str = ""):
        self.__name = name
        self.__listeners = defaultdict(set)
        self.__call_listeners = defaultdict(type(None))
        self.__reversed_mapping = dict()

    def __add_to_reversed_mapping(self, f, listener, /):
        key = _make_callable_key(f)
        if key not in self.__reversed_mapping:
            self.__reversed_mapping[key] = set()
        self.__reversed_mapping[key].add(listener)

    @_DottableMethod
    def on(self, name: str, f=EMPTY, /, weak=False):
        def _register(f):
            listener = Listener(f, weak=weak, once=False)
            self.__listeners[name].add(listener)
            self.__add_to_reversed_mapping(f, listener)
            return f

        if f is EMPTY:
            return _register

        return _register(f)

    @_DottableMethod
    def once(self, name: str, f=EMPTY, /):
        def _register(f):
            listener = Listener(f, weak=False, once=True)
            self.__listeners[name].add(listener)
            self.__add_to_reversed_mapping(f, listener)
            return f

        if f is EMPTY:
            return _register

        return _register(f)

    @_DottableMethod
    def oncall(self, name: str, f=EMPTY, /, weak=True):
        def _register(f):
            assert self.__call_listeners[name] is None
            self.__call_listeners[name] = Listener(f, weak=weak, once=False)
            return f

        if f is EMPTY:
            return _register

        return _register(f)

    @_DottableMethod
    def emit(self, name, *args, **kwargs):
        to_remove = set()
        for listener in self.__listeners[name]:
            f = listener.unwrap()
            if f is None or listener.once:
                to_remove.add(listener)
            if f is not None:
                f(*args, **kwargs)
        self.__listeners[name] -= to_remove

    @_DottableMethod
    def call(self, name, *args, **kwargs):
        listener = self.__call_listeners[name]
        assert listener is not None
        f = listener.unwrap()
        assert f is not None
        return f(*args, **kwargs)

    @_DottableMethod
    def off(self, name, f, /):
        key = _make_callable_key(f)
        assert key in self.__reversed_mapping
        reversed_set = self.__reversed_mapping[key]
        listeners_set = self.__listeners[name]
        listeners_to_remove = reversed_set & listeners_set
        listeners_set.difference_update(listeners_to_remove)
        reversed_set.difference_update(listeners_to_remove)
        if not reversed_set:
            del self.__reversed_mapping[key]

    @_DottableMethod
    def offcall(self, name, /):
        self.__call_listeners[name] = None
