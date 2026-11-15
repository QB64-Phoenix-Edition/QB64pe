OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE ReturnData
    foo(0 TO 200) AS LONG
    x AS LONG
END TYPE

DIM SHARED recurseLevel AS LONG
DIM SHARED callCount AS LONG
DIM got AS ReturnData

recurseLevel = 0
callCount = 0

got = MakeIndexed

IF got.foo(7) <> 2 THEN
    PRINT "FAIL: foo(7) returned"; got.foo(7); "expected 2"
    SYSTEM 1
END IF

IF got.x <> 123 THEN
    PRINT "FAIL: outer return object was damaged; x ="; got.x; "expected 123"
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

FUNCTION MakeIndexed AS ReturnData
    callCount = callCount + 1

    IF recurseLevel = 0 THEN
        ' This value belongs to the outer return object and must survive
        ' the recursive call used by the array index expression.
        MakeIndexed.x = 123

        recurseLevel = 1

        ' Outermost MakeIndexed.foo(...) is the current return object.
        ' Inner MakeIndexed.x is a recursive call whose returned x
        ' supplies the array index.
        MakeIndexed.foo(MakeIndexed.x) = 2

        recurseLevel = 0
    ELSE
        MakeIndexed.x = 7
    END IF
END FUNCTION
