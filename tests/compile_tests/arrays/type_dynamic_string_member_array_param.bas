$CONSOLE:ONLY
$UNSTABLE:TypeFields
OPTION _EXPLICIT

' A _Dynamic AS STRING member array is handed straight to an ordinary
' items() AS STRING parameter, so the descriptor payload must match that of an
' ordinary STRING array.

TYPE Leaf
    labels(1) _DYNAMIC AS STRING
    count AS LONG
END TYPE

DIM work AS Leaf
REDIM work.labels(1 TO 3)
work.labels(1) = "red"
work.labels(2) = "green"
work.labels(3) = "blue"
work.count = 3

AssertString "direct 1", work.labels(1), "red"
AssertString "direct 3", work.labels(3), "blue"

ReadBack work.labels()
WriteThrough work.labels()

AssertString "after write 1", work.labels(1), "RED"
AssertString "after write 3", work.labels(3), "BLUE"
AssertBool "sibling member intact", work.count = 3

SYSTEM

SUB ReadBack (items() AS STRING)
    AssertString "param lbound", items(LBOUND(items)), "red"
    AssertString "param 2", items(2), "green"
    AssertString "param ubound", items(UBOUND(items)), "blue"
END SUB

SUB WriteThrough (items() AS STRING)
    DIM i AS LONG
    FOR i = LBOUND(items) TO UBOUND(items)
        items(i) = UCASE$(items(i))
    NEXT
END SUB


'$include:'../utilities/assert.bm'
