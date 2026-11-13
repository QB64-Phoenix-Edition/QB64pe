OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE ChildData
    value AS LONG
END TYPE

TYPE ReturnData
    child AS ChildData
    marker AS LONG
END TYPE

DIM got AS ReturnData

got = MakeNested

IF got.child.value <> 456 THEN
    PRINT "FAIL: nested LHS member returned"; got.child.value; "expected 456"
    SYSTEM 1
END IF

IF got.marker <> 789 THEN
    PRINT "FAIL: second LHS member returned"; got.marker; "expected 789"
    SYSTEM 1
END IF

PRINT "PASS"
SYSTEM 0

FUNCTION MakeNested AS ReturnData
    MakeNested.child.value = 456
    MakeNested.marker = 789
END FUNCTION
