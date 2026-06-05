$Console:Only
OPTION _EXPLICIT

TYPE T210
    nums(-2 TO 2) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T210

work(0).nums(-2) = 10
work(0).nums(-1) = 20
work(0).nums(0) = 30
work(0).nums(1) = 40
work(0).nums(2) = 50

ok = (LBOUND(work(0).nums) = -2 AND UBOUND(work(0).nums) = 2 AND work(0).nums(-2) = 10 AND work(0).nums(2) = 50)
IF ok THEN PRINT "PASS t210_negative_bounds_1d" ELSE PRINT "FAIL t210_negative_bounds_1d"
SYSTEM
