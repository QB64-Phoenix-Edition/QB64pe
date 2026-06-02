$Console:Only
OPTION _EXPLICIT

TYPE T240
    nums(0 TO 2) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T240

work(0).nums(2) = 222
ERASE work(0).nums
REDIM work(0).nums(1 TO 2)
work(0).nums(1) = 111
work(0).nums(2) = 222
ok = (LBOUND(work(0).nums) = 1 AND UBOUND(work(0).nums) = 2 AND work(0).nums(1) = 111 AND work(0).nums(2) = 222)
IF ok THEN PRINT "PASS t240_erase_member_then_redim" ELSE PRINT "FAIL t240_erase_member_then_redim"
SYSTEM
