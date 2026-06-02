$Console:Only
OPTION _EXPLICIT

TYPE T238
    nums(0 TO 1) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T238

work(0).nums(0) = 101
work(0).nums(1) = 202
REDIM _PRESERVE work(0).nums(0 TO 2)
work(0).nums(2) = 303
ok = (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 2 AND work(0).nums(0) = 101 AND work(0).nums(1) = 202 AND work(0).nums(2) = 303)
IF ok THEN PRINT "PASS t238_member_redim_preserve_expand" ELSE PRINT "FAIL t238_member_redim_preserve_expand"
SYSTEM
