OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE ReturnData
    value AS LONG
END TYPE

DIM got AS ReturnData

got = MakeValue

IF got.value <> 123 THEN
    PRINT "FAIL: LHS member assignment returned"; got.value; "expected 123"
    SYSTEM 1
END IF

PRINT "PASS"
SYSTEM 0

FUNCTION MakeValue AS ReturnData
    MakeValue.value = 123
END FUNCTION
