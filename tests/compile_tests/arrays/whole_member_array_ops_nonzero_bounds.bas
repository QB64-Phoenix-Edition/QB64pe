$CONSOLE:ONLY
OPTION _EXPLICIT


DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE MemOpsRecord
    Header AS LONG
    Grid( 2 To 4) AS INTEGER
    Footer AS LONG
END TYPE

DIM a(0) AS MemOpsRecord
DIM b(0) AS MemOpsRecord
DIM m AS _MEM

DIM i AS LONG

a(0).Header = 111
FOR i = 2 TO 4
    a(0).Grid(i) = i * 11
NEXT i
a(0).Footer = 222

b(0).Header = 333
FOR i = 2 TO 4
    b(0).Grid(i) = 0
NEXT i
b(0).Footer = 444

CheckTrue "bounds", LBOUND(a(0).Grid) = 2 AND UBOUND(a(0).Grid) = 4
m = _MEM(a(0).Grid)
CheckTrue "mem size", m.SIZE = 6
CheckTrue "mem element size", m.ELEMENTSIZE = 2

_MEMGET m, m.OFFSET, b(0).Grid
CheckTrue "memget copied", b(0).Grid(2) = 22 AND b(0).Grid(4) = 44
CheckTrue "memget keeps scalars", b(0).Header = 333 AND b(0).Footer = 444

b(0).Grid(2) = 7
b(0).Grid(3) = 8
b(0).Grid(4) = 9
_MEMPUT m, m.OFFSET, b(0).Grid
CheckTrue "memput copied back", a(0).Grid(2) = 7 AND a(0).Grid(4) = 9
CheckTrue "memput keeps scalars", a(0).Header = 111 AND a(0).Footer = 222

_MEMFILL m, m.OFFSET, m.SIZE, 0 AS INTEGER
CheckTrue "memfill zeroes", a(0).Grid(2) = 0 AND a(0).Grid(4) = 0
CheckTrue "memfill keeps scalars", a(0).Header = 111 AND a(0).Footer = 222

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
