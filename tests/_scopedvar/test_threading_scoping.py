from _scoping_utils import *

A = Var("A")

make(
    Patch(
        {A: 1},
        Thread(
            A.should_be(1),
            A.assign(2),
        ),
        A.should_be(2),
    ),
)

make(
    PatchLocal(
        {A: 1},
        Thread(
            A.should_empty(),
            A.assign(2),
        ),
        A.should_be(1),
    ),
)

make(
    A.assign(1),
    Shield(
        Thread(
            A.should_be(1),
            A.assign(2),
        ),
        A.should_be(2),
    ),
    A.should_be(1),
)

make(
    A.assign(1),
    ShieldLocal(
        Thread(
            A.should_empty(),
            A.assign(2),
        ),
        A.should_be(2),
        A.assign(3),
    ),
    A.should_be(2),
)

make(
    A.assign(1),
    Isolate(
        Thread(
            A.should_empty(),
            A.assign(2),
        ),
        A.should_be(2),
    ),
    A.should_be(1),
)

make(
    A.assign(1),
    IsolateLocal(
        Thread(
            A.should_be(1),
            A.assign(2),
        ),
        A.should_empty(),
        A.assign(3),
    ),
    A.should_be(2),
)
