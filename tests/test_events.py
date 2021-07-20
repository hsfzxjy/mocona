import pytest
import sys
from mocona.events import EventEmitter


def test_on_strong_function():
    a = 1

    def f():
        nonlocal a
        a += 1

    E = EventEmitter()
    E.on("my.event", f, weak=False)
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()
    E.on.my.event(f, weak=False)
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.on("my.event", weak=False)
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.on.my.event
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.on.my.event(weak=False)
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2


def test_on_weak_function():
    a = 1

    def f():
        nonlocal a
        a += 1

    E = EventEmitter()
    E.on("my.event", f, weak=True)
    E.emit("my.event")
    assert a == 2
    del f
    E.emit("my.event")
    assert a == 2

    a = 1

    def f():
        nonlocal a
        a += 1

    E = EventEmitter()
    E.on.my.event(f, weak=True)
    E.emit("my.event")
    assert a == 2
    del f
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.on("my.event", weak=True)
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2
    del f
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.on.my.event(weak=True)
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2
    del f
    E.emit("my.event")
    assert a == 2


def test_once_function():
    a = 1

    def f():
        nonlocal a
        a += 1

    E = EventEmitter()
    E.once("my.event", f)
    E.emit("my.event")
    assert a == 2
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()
    E.once.my.event(f)
    E.emit("my.event")
    assert a == 2
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.once("my.event")
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()

    @E.once.my.event
    def f():
        nonlocal a
        a += 1

    E.emit("my.event")
    assert a == 2
    E.emit("my.event")
    assert a == 2

    a = 1
    E = EventEmitter()


def test_emit():
    E = EventEmitter()

    a = 1

    @E.on.my.event
    def f(b):
        nonlocal a
        a += b

    E.emit("my.event", b=1)
    assert a == 2
    a = 1
    E.emit("my.event", 1)
    assert a == 2

    a = 1
    E.emit.my.event(b=1)
    assert a == 2

    a = 1
    E.emit.my.event(1)
    assert a == 2


def test_call():
    E = EventEmitter()

    a = 1

    @E.oncall.my.event
    def f():
        nonlocal a
        a += 1
        return a

    assert E.call("my.event") == 2
    assert E.call.my.event() == 3


def test_off():
    E = EventEmitter()

    a = 1

    @E.on.my.event
    @E.on.my.event2
    def f():
        nonlocal a
        a += 1

    @E.on.my.event
    def f2():
        nonlocal a
        a += 1

    E.off("my.event", f)
    E.emit.my.event()
    assert a == 2
    E.emit.my.event2()
    assert a == 3
    E.off("my.event2", f)
    E.emit.my.event2()
    assert a == 3


def test_offcall():
    E = EventEmitter()

    a = 1

    @E.oncall.my.event
    @E.oncall.my.event2
    def f():
        nonlocal a
        a += 1
        return a

    E.offcall("my.event")
    with pytest.raises(AssertionError):
        E.call.my.event()
    assert E.call.my.event2() == 2
    E.offcall.my.event2()
    with pytest.raises(AssertionError):
        E.call.my.event2()
