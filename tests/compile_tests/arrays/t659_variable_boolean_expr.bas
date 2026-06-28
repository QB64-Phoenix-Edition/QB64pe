OPTION _EXPLICIT
$CONSOLE:ONLY

DIM AS LONG valueA, valueB

valueA = 5
valueB = 2

expect_bool (valueA / valueB) = 2, 0, "variable real division comparison"
expect_bool (valueA \ valueB) = 2, -1, "variable integer division comparison"
expect_bool ((valueA \ valueB) + 3) = 5, -1, "nested variable expression"
expect_bool ((valueA = 5) AND (valueB = 2)) = -1, -1, "combined boolean expression"
expect_bool ((valueA = 0) OR (valueB = 2)) = -1, -1, "boolean OR expression"

SYSTEM

SUB expect_bool (actual&, expected&, msg$)
    IF actual& = expected& THEN
        PRINT "passed: "; msg$
    ELSE
        PRINT "failed: "; msg$
    END IF
END SUB
