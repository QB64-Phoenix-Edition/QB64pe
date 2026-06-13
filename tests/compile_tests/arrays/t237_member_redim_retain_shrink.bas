$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T237
    nums(0 TO 3) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T237

work(0).nums(0) = 10
work(0).nums(1) = 20
work(0).nums(2) = 30
work(0).nums(3) = 40
REDIM _RETAIN work(0).nums(0 TO 1)
ok = (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 1 AND work(0).nums(0) = 10 AND work(0).nums(1) = 20)
IF ok THEN PRINT "PASS t237_member_redim_retain_shrink" ELSE PRINT "FAIL t237_member_redim_retain_shrink"
SYSTEM
