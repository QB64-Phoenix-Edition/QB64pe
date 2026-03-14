$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE FloatFamily
    S( 0 To 2 , 0 To 1) AS SINGLE
    D( 0 To 2 , 0 To 1) AS DOUBLE
    F( 0 To 2 , 0 To 1) AS _FLOAT
END TYPE

DIM a AS FloatFamily
DIM b AS FloatFamily
DIM i AS LONG
DIM j AS LONG


FOR i = 0 TO 2
    FOR j = 0 TO 1
        a.S(i, j) = i * 1.25 + j * .5
        a.D(i, j) = i * 10.125 + j * .25
        a.F(i, j) = i * 100.5 + j * .125
    NEXT j
NEXT i

CheckNear "Single sample", a.S(2, 1), 3#, .0001
CheckNear "Double sample", a.D(2, 1), 20.5, .0000001
CheckNear "_Float sample", a.F(2, 1), 201.125, .0000001
CheckTrue "LBOUND dim1", LBOUND(a.S, 1) = 0
CheckTrue "UBOUND dim2", UBOUND(a.S, 2) = 1

b = a
CheckNear "copy Single", b.S(2, 1), 3#, .0001
CheckNear "copy Double", b.D(2, 1), 20.5, .0000001
CheckNear "copy _Float", b.F(2, 1), 201.125, .0000001

ERASE a . S
CheckNear "ERASE Single first", a.S(0, 0), 0#, .0001
CheckNear "ERASE Single last", a.S(2, 1), 0#, .0001
CheckNear "ERASE keeps Double intact", a.D(2, 1), 20.5, .0000001
CheckNear "ERASE keeps _Float intact", a.F(2, 1), 201.125, .0000001

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
