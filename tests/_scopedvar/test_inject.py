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
    stats.check(DECL=3, CELL=2, SCOPE=2)


@isolate_testcase
def test_inject_defaults_with_namespace():
    @S.inject
    def f(x, a=V, a2=V.a - 0):
        assert x == a == a2

    with S.patch_local({V.a: 1}):
        f(1)

    with S.patch_local({V.a: 2}):
        f(2)

    del f
    stats.check(DECL=4, CELL=2, SCOPE=2, NAMESPACE=0)


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
    stats.check(DECL=3, CELL=2, SCOPE=2)


@isolate_testcase
def test_inject_kwdefaults_with_namespace():
    @S.inject
    def f(x, a2=V.a - 0, a=V):
        assert x == a == a2

    with S.patch_local({V.a: 1}):
        f(1)

    with S.patch_local({V.a: 2}):
        f(2)

    del f
    stats.check(DECL=4, CELL=2, SCOPE=2, NAMESPACE=0)


@isolate_testcase
def test_inject_mixed_defaults_with_namespace():
    @S.inject
    def f(x, a=V.NS[...], *, a2=V.NS.a - 0):
        assert x == a == a2

    with S.patch_local({V.NS.a: 1}):
        f(1)

    with S.patch_local({V.NS.a: 2}):
        f(2)

    del f
    stats.check(DECL=5, CELL=2, SCOPE=2, NAMESPACE=1)
