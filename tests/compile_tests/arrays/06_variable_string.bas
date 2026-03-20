$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE VariableStrings
    Names( 0 To 2) AS STRING
    Tags( 0 To 1 , 0 To 1) AS STRING
    Title AS STRING
    Count AS LONG
END TYPE

DIM a(0 TO 1) AS VariableStrings
DIM b AS VariableStrings

a(0).Names(0) = "alpha"
a(0).Names(1) = "beta"
a(0).Names(2) = "gamma"
a(0).Tags(0, 0) = "red"
a(0).Tags(1, 1) = "blue"
a(0).Title = "source"
a(0).Count = 3

CheckText "Name 0", a(0).Names(0), "alpha"
CheckText "Name 2", a(0).Names(2), "gamma"
CheckText "Tag 1,1", a(0).Tags(1, 1), "blue"
CheckText "Title", a(0).Title, "source"
CheckTrue "Count", a(0).Count = 3

b = a(0)
CheckText "copy name", b.Names(1), "beta"
CheckText "copy tag", b.Tags(1, 1), "blue"
CheckText "copy title", b.Title, "source"

a(0).Names(1) = "changed"
CheckText "copy is independent for name", b.Names(1), "beta"

ERASE a ( 0 ) . Names
CheckText "ERASE member first", a(0).Names(0), ""
CheckText "ERASE member middle", a(0).Names(1), ""
CheckText "ERASE member last", a(0).Names(2), ""
CheckText "ERASE member keeps title", a(0).Title, "source"
CheckTrue "ERASE member keeps count", a(0).Count = 3

ERASE a
CheckText "ERASE parent string member", a(0).Tags(1, 1), ""
CheckText "ERASE parent title", a(0).Title, ""
CheckTrue "ERASE parent count", a(0).Count = 0

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
