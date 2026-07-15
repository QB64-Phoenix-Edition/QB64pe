OPTION _EXPLICIT
$CONSOLE:ONLY

CALL expect_bool((5 / 2) = 2, 0, "CALL with false comparison")
CALL expect_bool((5 \ 2) = 2, -1, "CALL with true comparison")
CALL expect_bool(((2 + 3) * 4) = 20, -1, "CALL with nested expression")
CALL expect_bool(("QB" + "64") = "QB64", -1, "CALL with string comparison")

SYSTEM

SUB expect_bool (actual&, expected&, msg$)
    IF actual& = expected& THEN
        PRINT "passed: "; msg$
    ELSE
        PRINT "failed: "; msg$
    END IF
END SUB
