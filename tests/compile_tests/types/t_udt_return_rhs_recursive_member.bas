OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE ReturnData
    value AS LONG
END TYPE

DIM SHARED recurseLevel AS LONG
DIM SHARED callCount AS LONG
DIM got AS ReturnData

recurseLevel = 0
callCount = 0

got = MakeRecursive

IF got.value <> 42 THEN
    PRINT "FAIL: recursive RHS member returned"; got.value; "expected 42"
    SYSTEM 1
END IF

IF callCount <> 2 THEN
    PRINT "FAIL: expected exactly 2 function calls, got"; callCount
    SYSTEM 1
END IF

IF recurseLevel <> 0 THEN
    PRINT "FAIL: recursion guard was not restored"
    SYSTEM 1
END IF

PRINT "PASS"
SYSTEM 0

FUNCTION MakeRecursive AS ReturnData
    callCount = callCount + 1

    IF recurseLevel = 0 THEN
        recurseLevel = 1

        ' LHS MakeRecursive.value is the current function's return object.
        ' RHS MakeRecursive.value must be a recursive function call
        ' followed by direct member access.
        MakeRecursive.value = MakeRecursive.value + 2

        recurseLevel = 0
    ELSE
        MakeRecursive.value = 40
    END IF
END FUNCTION
