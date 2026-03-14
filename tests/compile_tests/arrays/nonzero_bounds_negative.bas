$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE NegativeBounds
    Grid( - 3 To - 1 , - 2 To 1) AS LONG
    Keep AS LONG
END TYPE

DIM x(0) AS NegativeBounds
DIM i AS LONG
DIM j AS LONG
DIM total AS LONG
DIM m AS _MEM

FOR i = -3 TO -1
    FOR j = -2 TO 1
        x(0).Grid(i, j) = i * 10 + j
        total = total + x(0).Grid(i, j)
    NEXT j
NEXT i
x(0).Keep = 999

CheckTrue "bounds d1", LBOUND(x(0).Grid) = -3 AND UBOUND(x(0).Grid) = -1
CheckTrue "bounds d2", LBOUND(x(0).Grid, 2) = -2 AND UBOUND(x(0).Grid, 2) = 1
CheckTrue "values", x(0).Grid(-3, -2) = -32 AND x(0).Grid(-1, 1) = -9
CheckTrue "sum", total = -246

m = _MEM(x(0).Grid)
CheckTrue "mem size", m.SIZE = 48
CheckTrue "mem element size", m.ELEMENTSIZE = 4

ERASE x ( 0 ) . Grid
CheckTrue "erase zeroes", x(0).Grid(-3, -2) = 0 AND x(0).Grid(-1, 1) = 0
CheckTrue "erase keeps scalar", x(0).Keep = 999

_MEMFREE m
FinishTest
'SLEEP
SYSTEM

SUB CheckTrue (label AS STRING, condition AS LONG)
    IF condition THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label
    END IF
END SUB

SUB CheckText (label AS STRING, actual AS STRING, expected AS STRING)
    IF actual = expected THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label; " expected=["; expected; "] actual=["; actual; "]"
    END IF
END SUB

SUB CheckNear (label AS STRING, actual AS DOUBLE, expected AS DOUBLE, tolerance AS DOUBLE)
    IF ABS(actual - expected) <= tolerance THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label; " expected="; expected; " actual="; actual
    END IF
END SUB

SUB FinishTest
    IF FailCount = 0 THEN
        PRINT "RESULT: PASS"
    ELSE
        PRINT "RESULT: FAIL"; FailCount; "failure(s)"
    END IF
END SUB
