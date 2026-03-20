$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE MixedLayout
    Prefix AS LONG
    A( 0 To 2) AS INTEGER
    Middle AS DOUBLE
    B( 0 To 1 , 0 To 1) AS LONG
    Tail AS _UNSIGNED _BYTE
END TYPE

DIM x(0 TO 0) AS MixedLayout
DIM memA AS _MEM
DIM memB AS _MEM

x(0).Prefix = 111
x(0).A(0) = 1
x(0).A(1) = 2
x(0).A(2) = 3
x(0).Middle = 4.5
x(0).B(0, 0) = 10
x(0).B(1, 1) = 20
x(0).Tail = 255

CheckTrue "offset A after Prefix", _OFFSET(x(0).A) > _OFFSET(x(0).Prefix)
CheckTrue "offset Middle after A", _OFFSET(x(0).Middle) > _OFFSET(x(0).A)
CheckTrue "offset B after Middle", _OFFSET(x(0).B) > _OFFSET(x(0).Middle)
CheckTrue "offset Tail after B", _OFFSET(x(0).Tail) > _OFFSET(x(0).B)

memA = _MEM(x(0).A)
memB = _MEM(x(0).B)
CheckTrue "memA element size", memA.ELEMENTSIZE = 2
CheckTrue "memB element size", memB.ELEMENTSIZE = 4

ERASE x ( 0 ) . A
CheckTrue "ERASE A zeroes member", x(0).A(0) = 0 AND x(0).A(2) = 0
CheckTrue "ERASE A keeps Prefix", x(0).Prefix = 111
CheckNear "ERASE A keeps Middle", x(0).Middle, 4.5, .0000001
CheckTrue "ERASE A keeps B", x(0).B(1, 1) = 20
CheckTrue "ERASE A keeps Tail", x(0).Tail = 255

ERASE x
CheckTrue "ERASE parent zeroes Prefix", x(0).Prefix = 0
CheckNear "ERASE parent zeroes Middle", x(0).Middle, 0#, .0000001
CheckTrue "ERASE parent zeroes B", x(0).B(1, 1) = 0
CheckTrue "ERASE parent zeroes Tail", x(0).Tail = 0

_MEMFREE memA
_MEMFREE memB

FinishTest
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
