$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG

TYPE FixedStrings
    S4(0 TO 2) AS STRING * 4
    S8(0 TO 1, 0 TO 1) AS STRING * 8
    Label AS STRING * 6
END TYPE

DIM a AS FixedStrings
DIM b AS FixedStrings
DIM z4 AS STRING * 4

a.S4(0) = "AB"
a.S4(1) = "WXYZ"
a.S4(2) = "ABCDEFG"
a.S8(0, 0) = "HELLO"
a.S8(1, 1) = "123456789"
a.Label = "QB64PE"

CheckTrue "S4(0) len", LEN(a.S4(0)) = 4
CheckText "S4(0) trimmed", RTRIM$(a.S4(0)), "AB"
CheckText "S4(1) exact", a.S4(1), "WXYZ"
CheckText "S4(2) truncates", a.S4(2), "ABCD"

CheckTrue "S8(0,0) len", LEN(a.S8(0, 0)) = 8
CheckText "S8(0,0) trimmed", RTRIM$(a.S8(0, 0)), "HELLO"
CheckText "S8(1,1) truncates", a.S8(1, 1), "12345678"
CheckText "Label exact", a.Label, "QB64PE"

b = a
CheckText "copy S4(2)", b.S4(2), "ABCD"
CheckText "copy S8(1,1)", b.S8(1, 1), "12345678"
CheckText "copy Label", b.Label, "QB64PE"

ERASE a.S4
CheckTrue "ERASE fixed member first", a.S4(0) = z4
CheckTrue "ERASE fixed member last", a.S4(2) = z4
CheckText "ERASE fixed member keeps label", a.Label, "QB64PE"

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

SUB FinishTest
    IF FailCount = 0 THEN
        PRINT "RESULT: PASS"
    ELSE
        PRINT "RESULT: FAIL"; FailCount; "failure(s)"
    END IF
END SUB

