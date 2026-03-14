$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE InnerRecord
    Cells( 2 To 4) AS INTEGER
    Mark AS LONG
END TYPE

TYPE OuterRecord
    Items( 0 To 1) AS InnerRecord
    Footer AS LONG
END TYPE

DIM x(0) AS OuterRecord

x(0).Items(0).Cells(2) = 12
x(0).Items(0).Cells(3) = 13
x(0).Items(0).Cells(4) = 14
x(0).Items(0).Mark = 100
x(0).Items(1).Cells(2) = 22
x(0).Items(1).Cells(3) = 23
x(0).Items(1).Cells(4) = 24
x(0).Items(1).Mark = 200
x(0).Footer = 300

CheckTrue "outer bounds", LBOUND(x(0).Items) = 0 AND UBOUND(x(0).Items) = 1
CheckTrue "inner bounds", LBOUND(x(0).Items(1).Cells) = 2 AND UBOUND(x(0).Items(1).Cells) = 4
CheckTrue "nested values 1", x(0).Items(0).Cells(2) = 12 AND x(0).Items(1).Cells(4) = 24
CheckTrue "nested values 2", x(0).Items(0).Mark = 100 AND x(0).Items(1).Mark = 200 AND x(0).Footer = 300

ERASE x ( 0 ) . Items ( 1 ) . Cells
CheckTrue "erase inner member array zeroes", x(0).Items(1).Cells(2) = 0 AND x(0).Items(1).Cells(4) = 0
CheckTrue "erase inner member array keeps inner scalar", x(0).Items(1).Mark = 200
CheckTrue "erase inner member array keeps outer", x(0).Items(0).Cells(2) = 12 AND x(0).Footer = 300

ERASE x ( 0 ) . Items
CheckTrue "erase outer member array clears nested 1", x(0).Items(0).Cells(2) = 0 AND x(0).Items(1).Cells(4) = 0
CheckTrue "erase outer member array clears nested 2", x(0).Items(0).Mark = 0 AND x(0).Items(1).Mark = 0
CheckTrue "erase outer member array keeps footer", x(0).Footer = 300

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
