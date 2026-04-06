$CONSOLE:ONLY

TYPE LeafType
    Num(1 TO 2) AS LONG
END TYPE

TYPE MidType
    Leaf(1 TO 2) AS LeafType
END TYPE

TYPE TopType
    Node(1 TO 2) AS MidType
END TYPE

ON ERROR GOTO errhandler

DIM G AS TopType
DIM idx AS LONG

idx = UBOUND(G.Node(1).Leaf(1).Num) + 1
G.Node(1).Leaf(1).Num(idx) = 1

PRINT "FAIL: ErrorTest08 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest08 multi-nested member UBOUND overflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest08 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
