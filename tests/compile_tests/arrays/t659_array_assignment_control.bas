OPTION _EXPLICIT
$CONSOLE:ONLY

DIM values(0 TO 3) AS LONG

values(0) = 10
values(1) = 20
values(2) = values(0) + values(1)
values(3) = 99

expect_bool values(0) = 10, -1, "values(0) assignment"
expect_bool values(1) = 20, -1, "values(1) assignment"
expect_bool values(2) = 30, -1, "values(2) assignment"
expect_bool values(3) = 99, -1, "values(3) assignment"
expect_bool (values(0) + values(1)) = 30, -1, "array expression comparison"

SYSTEM


SUB expect_bool (actual&, expected&, msg$)
    IF actual& = expected& THEN
        PRINT "passed: "; msg$
    ELSE
        PRINT "failed: "; msg$
    END IF
END SUB
