$CONSOLE:ONLY

TYPE LeafType
    Num(4 TO 5) AS LONG
END TYPE

TYPE MidType
    Leaf(2 TO 3) AS LeafType
END TYPE

TYPE TopType
    Node(8 TO 9) AS MidType
END TYPE

ON ERROR GOTO errhandler

DIM G AS TopType

IF LBOUND(G.Node) = 8 AND UBOUND(G.Node) = 9 AND LBOUND(G.Node(8).Leaf) = 2 AND UBOUND(G.Node(8).Leaf) = 3 AND LBOUND(G.Node(8).Leaf(2).Num) = 4 AND UBOUND(G.Node(8).Leaf(2).Num) = 5 THEN
    PRINT "PASS: ErrorTest14 multi-nested member bounds are correct."
ELSE
    PRINT "FAIL: ErrorTest14 multi-nested member bounds are wrong."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest14 unexpected runtime error"; ERR
'SLEEP
SYSTEM
