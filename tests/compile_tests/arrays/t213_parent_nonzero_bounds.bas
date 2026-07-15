$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T213
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(3 TO 4) AS T213

work(3).nums(0) = 30
work(4).nums(1) = 41
ok = (LBOUND(work) = 3 AND UBOUND(work) = 4 AND work(3).nums(0) = 30 AND work(4).nums(1) = 41)
IF ok THEN PRINT "PASS t213_parent_nonzero_bounds" ELSE PRINT "FAIL t213_parent_nonzero_bounds"
SYSTEM
