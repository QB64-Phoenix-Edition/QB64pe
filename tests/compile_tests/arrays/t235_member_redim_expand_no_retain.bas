$Console:Only
OPTION _EXPLICIT

TYPE T235
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T235

work(0).nums(0) = 77
REDIM work(0).nums(0 TO 3)
ok = (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 3 AND work(0).nums(0) = 0 AND work(0).nums(3) = 0)
IF ok THEN PRINT "PASS t235_member_redim_expand_no_retain" ELSE PRINT "FAIL t235_member_redim_expand_no_retain"
SYSTEM
