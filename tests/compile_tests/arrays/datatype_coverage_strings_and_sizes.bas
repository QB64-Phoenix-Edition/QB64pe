$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE DataTypesRecord
    Prefix AS LONG
    Fixed( 1 To 2) AS STRING * 4
    Var( - 1 To 0) AS STRING
    Sng( 0 To 1) AS SINGLE
    Dbl( 0 To 1) AS DOUBLE
    I64( 0 To 1) AS _INTEGER64
    U64( 0 To 1) AS _UNSIGNED _INTEGER64
    Tail AS LONG
END TYPE

DIM x(0) AS DataTypesRecord

x(0).Prefix = 10
x(0).Fixed(1) = "AB"
x(0).Fixed(2) = "WXYZ"
x(0).Var(-1) = "hello"
x(0).Var(0) = "QB64PE"
x(0).Sng(0) = 1.5
x(0).Sng(1) = 2.5
x(0).Dbl(0) = 10.25
x(0).Dbl(1) = 20.75
x(0).I64(0) = 1000000000
x(0).I64(1) = -2000000000
x(0).U64(0) = 3000000000
x(0).U64(1) = 4000000000
x(0).Tail = 20

CheckTrue "fixed bounds", LBOUND(x(0).Fixed) = 1 AND UBOUND(x(0).Fixed) = 2
CheckTrue "var bounds", LBOUND(x(0).Var) = -1 AND UBOUND(x(0).Var) = 0
CheckTrue "fixed len", LEN(x(0).Fixed(1)) = 4 AND LEN(x(0).Fixed(2)) = 4
CheckText "fixed text 1", RTRIM$(x(0).Fixed(1)), "AB"
CheckText "fixed text 2", RTRIM$(x(0).Fixed(2)), "WXYZ"
CheckText "var text 1", x(0).Var(-1), "hello"
CheckText "var text 2", x(0).Var(0), "QB64PE"
CheckNear "single values", x(0).Sng(0) + x(0).Sng(1), 4#, .000001
CheckNear "double values", x(0).Dbl(0) + x(0).Dbl(1), 31#, .000001
CheckTrue "int64 values", x(0).I64(0) = 1000000000 AND x(0).I64(1) = -2000000000
CheckTrue "uint64 values", x(0).U64(0) = 3000000000 AND x(0).U64(1) = 4000000000

ERASE x ( 0 ) . Fixed
CheckText "erase fixed 1", RTRIM$(x(0).Fixed(1)), ""
CheckText "erase fixed 2", RTRIM$(x(0).Fixed(2)), ""
CheckText "erase fixed keeps var", x(0).Var(-1), "hello"
CheckTrue "erase fixed keeps tail", x(0).Tail = 20

ERASE x ( 0 ) . Var
CheckText "erase var 1", x(0).Var(-1), ""
CheckText "erase var 2", x(0).Var(0), ""
CheckTrue "erase var keeps fixed", RTRIM$(x(0).Fixed(1)) = ""
CheckTrue "erase var keeps prefix/tail", x(0).Prefix = 10 AND x(0).Tail = 20

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
