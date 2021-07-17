from _scoping_utils import *

A = Var("A")

make(
    A.assign(1),
    A.should_be(1),
)

make(
    A.should_empty(),
)

for method in (Patch, PatchLocal):
    make(
        method(
            A.assign(1),
            A.should_be(1),
        ),
        A.should_be(1),
    )

    make(
        method(
            A,
            A.assign(1),
            A.should_be(1),
        ),
        A.should_empty(),
    )

    make(
        method(
            {A: 1},
            A.should_be(1),
        ),
        A.should_empty(),
    )

for method in (Shield, ShieldLocal):
    make(
        A.assign(1),
        method(
            A.should_be(1),
            A.assign(2),
            A.should_be(2),
        ),
        A.should_be(1),
    )

    make(
        A.assign(1),
        method(
            A,
            A.should_empty(),
            A.assign(2),
            A.should_be(2),
        ),
        A.should_be(1),
    )

    make(
        A.assign(1),
        method(
            {A: 2},
            A.assign(2),
            A.should_be(2),
        ),
        A.should_be(1),
    )

for method in (Isolate, IsolateLocal):
    make(
        A.assign(1),
        method(
            A.should_empty(),
            A.assign(2),
            A.should_be(2),
        ),
        A.should_be(1),
    )

    make(
        A.assign(1),
        method(
            A,
            A.should_empty(),
            A.assign(2),
            A.should_be(2),
        ),
        A.should_be(1),
    )

    make(
        A.assign(1),
        method(
            {A: 2},
            A.assign(2),
            A.should_be(2),
        ),
        A.should_be(1),
    )
