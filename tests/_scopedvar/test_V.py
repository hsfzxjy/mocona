import pytest

from mocona.scopedvar import V


@pytest.mark.parametrize(
    "var, string",
    [
        [V, "/"],
        [V.a, "/a"],
        [V("a"), "/a"],
        [V.a[...], "/a/"],
        [V.a[...].b, "/a/b"],
        [V.a[...].b - 0, "/a/b"],
    ],
)
def test_V_str(var, string):
    assert str(var) == string
