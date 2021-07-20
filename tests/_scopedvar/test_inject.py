import gc

from mocona.scopedvar import S, V

from test_refcnt import stats, isolate_testcase


@isolate_testcase
def test_inject_defaults():
    @S.inject
    def f(x, a=V.a):
        assert x == a

    with S.patch_local({V.a: 1}):
        f(1)

    with S.patch_local({V.a: 2}):
        f(2)

    del f
    assert stats.DECL_ALLOCATED == stats.DECL_FREED == 3
    assert stats.CELL_ALLOCATED == stats.CELL_FREED == 2
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 2


@isolate_testcase
def test_inject_kwdefaults():
    @S.inject
    def f(x, *, a=V.a):
        assert x == a

    with S.patch_local({V.a: 1}):
        f(1)

    with S.patch_local({V.a: 2}):
        f(2)

    del f
    assert stats.DECL_ALLOCATED == stats.DECL_FREED == 3
    assert stats.CELL_ALLOCATED == stats.CELL_FREED == 2
    assert stats.SCOPE_ALLOCATED == stats.SCOPE_FREED == 2
