from _scoping_utils import *

A = Var("A")

make(
    Patch(
        {A: 1},
        Coroutine(
            A.should_be(1),
            A.assign(2),
        ),
        A.should_be(2),
    ),
)

make(
    Coroutine(
        Gather(
            Coroutine(
                PatchLocal(
                    A,
                    A.should_empty(),
                    A.assign(1),
                    Sleep(0),
                    A.should_be(1),
                )
            ),
            Coroutine(
                PatchLocal(
                    A,
                    A.should_empty(),
                    A.assign(2),
                    Sleep(0),
                    A.should_be(2),
                )
            ),
        )
    )
)

make(
    Coroutine(
        PatchLocal(
            A,
            A.assign(1),
            A.should_be(1),
            ShieldLocal(
                A.should_be(1),
                A.assign(2),
            ),
            A.should_be(1),
            IsolateLocal(
                A.should_empty(),
                A.assign(3),
            ),
            A.should_be(1),
        )
    ),
)

make(
    A.assign(0),
    Coroutine(
        A.should_be(0),
        PatchLocal(
            A.should_be(0),
            ShieldLocal(
                A.should_be(0),
                A.assign(1),
            ),
            A.should_be(0),
            IsolateLocal(
                A.should_empty(),
                A.assign(3),
            ),
            A.should_be(0),
        ),
    ),
)
