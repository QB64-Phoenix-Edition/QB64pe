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

G.Node(2).Leaf(1).Num(1) = 77
G.Node(2).Leaf(1).Num(2) = 88

ERASE G.Node(2).Leaf(1).Num

IF G.Node(2).Leaf(1).Num(1) = 0 AND G.Node(2).Leaf(1).Num(2) = 0 THEN
    PRINT "PASS: ErrorTest11 ERASE reset multi-nested member array to zero."
ELSE
    PRINT "FAIL: ErrorTest11 ERASE did not reset multi-nested member array."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest11 unexpected runtime error"; ERR
'SLEEP
SYSTEM
