$Console:Only
OPTION _EXPLICIT

TYPE T214
    nums(1 TO 2) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(-1 TO 1) AS T214

work(-1).nums(1) = -101
work(0).nums(2) = 2
work(1).nums(1) = 101
ok = (LBOUND(work) = -1 AND UBOUND(work) = 1 AND work(-1).nums(1) = -101 AND work(0).nums(2) = 2 AND work(1).nums(1) = 101)
IF ok THEN PRINT "PASS t214_parent_negative_bounds" ELSE PRINT "FAIL t214_parent_negative_bounds"
SYSTEM
