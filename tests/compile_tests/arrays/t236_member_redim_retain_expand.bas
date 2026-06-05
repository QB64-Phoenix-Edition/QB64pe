$Console:Only
OPTION _EXPLICIT

TYPE T236
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T236

work(0).nums(0) = 11
work(0).nums(1) = 22
REDIM _RETAIN work(0).nums(0 TO 3)
work(0).nums(2) = 33
work(0).nums(3) = 44
ok = (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 3 AND work(0).nums(0) = 11 AND work(0).nums(1) = 22 AND work(0).nums(2) = 33 AND work(0).nums(3) = 44)
IF ok THEN PRINT "PASS t236_member_redim_retain_expand" ELSE PRINT "FAIL t236_member_redim_retain_expand"
SYSTEM
