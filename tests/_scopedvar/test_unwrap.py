import gc

from mocona.scopedvar import S, V

from test_refcnt import stats, isolate_testcase

import pickle


@isolate_testcase
def test_unwrap():
    var = S._varfor(V.a)

    S.assign(var, [42])
    unwrapped = S.unwrap(var)
    S.assign(var, None)
    dumped = pickle.dumps(unwrapped)
    loaded = pickle.loads(dumped)

    assert unwrapped == loaded == [42]
