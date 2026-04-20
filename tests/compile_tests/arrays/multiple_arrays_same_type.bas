$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE MultiArrays
    Prefix AS LONG
    A( 1 To 3) AS INTEGER
    Flag AS _UNSIGNED _BYTE
    B( - 1 To 1 , 2 To 3) AS LONG
    Tail AS LONG
    C( 0 To 1 , 0 To 1 , 0 To 1) AS _BYTE
END TYPE

DIM x(0) AS MultiArrays
DIM i AS LONG
DIM j AS LONG
DIM k AS LONG

x(0).Prefix = 1000
x(0).Flag = 200
x(0).Tail = 3000

FOR i = 1 TO 3
    x(0).A(i) = i * 10
NEXT i

FOR i = -1 TO 1
    FOR j = 2 TO 3
        x(0).B(i, j) = i * 100 + j
    NEXT j
NEXT i

FOR i = 0 TO 1
    FOR j = 0 TO 1
        FOR k = 0 TO 1
            x(0).C(i, j, k) = i * 4 + j * 2 + k
        NEXT k
    NEXT j
NEXT i

CheckTrue "A bounds d1", LBOUND(x(0).A) = 1 AND UBOUND(x(0).A) = 3
CheckTrue "B bounds d1", LBOUND(x(0).B) = -1 AND UBOUND(x(0).B) = 1
CheckTrue "B bounds d2", LBOUND(x(0).B, 2) = 2 AND UBOUND(x(0).B, 2) = 3
CheckTrue "C bounds d3", LBOUND(x(0).C, 3) = 0 AND UBOUND(x(0).C, 3) = 1
CheckTrue "A values", x(0).A(1) = 10 AND x(0).A(3) = 30
CheckTrue "B values", x(0).B(-1, 2) = -98 AND x(0).B(1, 3) = 103
CheckTrue "C values", x(0).C(0, 1, 1) = 3 AND x(0).C(1, 1, 1) = 7
CheckTrue "layout order 1", _OFFSET(x(0).A) > _OFFSET(x(0).Prefix)
CheckTrue "layout order 2", _OFFSET(x(0).Flag) > _OFFSET(x(0).A)
CheckTrue "layout order 3", _OFFSET(x(0).B) > _OFFSET(x(0).Flag)
CheckTrue "layout order 4", _OFFSET(x(0).Tail) > _OFFSET(x(0).B)
CheckTrue "layout order 5", _OFFSET(x(0).C) > _OFFSET(x(0).Tail)

ERASE x ( 0 ) . B
CheckTrue "Erase member keeps prefix", x(0).Prefix = 1000
CheckTrue "Erase member keeps A", x(0).A(2) = 20
CheckTrue "Erase member zeroes B", x(0).B(-1, 2) = 0 AND x(0).B(1, 3) = 0
CheckTrue "Erase member keeps tail", x(0).Tail = 3000
CheckTrue "Erase member keeps C", x(0).C(1, 1, 1) = 7

FinishTest
'sleep
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
