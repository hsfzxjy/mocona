from mocona.scopedvar import Template, S, V
from test_refcnt import isolate_testcase


class Dummy(Template, ns=V.dummy[...]):
    foo: int
    bar: str


@isolate_testcase
def test_get_from_template():
    S.assign(S._varfor(V.dummy.foo), 1)
    assert Dummy.foo == 1


@isolate_testcase
@S.inject
def test_set_via_template(dummy_foo=V.dummy.foo - 0):
    Dummy.foo = 12
    assert dummy_foo == 12
