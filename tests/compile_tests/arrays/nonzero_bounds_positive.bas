$CONSOLE:ONLY
OPTION _EXPLICIT


DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE PositiveBounds
    Grid( 5 To 7 , 10 To 11) AS INTEGER
    Keep AS LONG
END TYPE

DIM a(0) AS PositiveBounds
DIM b(0) AS PositiveBounds
DIM i AS LONG
DIM j AS LONG
DIM m AS _MEM

FOR i = 5 TO 7
    FOR j = 10 TO 11
        a(0).Grid(i, j) = i * 100 + j
    NEXT j
NEXT i
a(0).Keep = 1234
b(0).Keep = 5678

CheckTrue "bounds d1", LBOUND(a(0).Grid) = 5 AND UBOUND(a(0).Grid) = 7
CheckTrue "bounds d2", LBOUND(a(0).Grid, 2) = 10 AND UBOUND(a(0).Grid, 2) = 11
CheckTrue "values", a(0).Grid(5, 10) = 510 AND a(0).Grid(7, 11) = 711

m = _MEM(a(0).Grid)
CheckTrue "mem size", m.SIZE = 12
CheckTrue "mem element size", m.ELEMENTSIZE = 2
_MEMGET m, m.OFFSET, b(0).Grid
CheckTrue "memget copied", b(0).Grid(5, 10) = 510 AND b(0).Grid(7, 11) = 711
CheckTrue "memget keeps scalar", b(0).Keep = 5678

ERASE a ( 0 ) . Grid
CheckTrue "erase member zeroes", a(0).Grid(5, 10) = 0 AND a(0).Grid(7, 11) = 0
CheckTrue "erase member keeps scalar", a(0).Keep = 1234

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
