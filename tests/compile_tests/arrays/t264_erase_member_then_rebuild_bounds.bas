$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T264
    nums(0 TO 2) _Dynamic AS LONG
    marker AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T264

work(0).nums(0) = 7
work(0).nums(1) = 8
work(0).nums(2) = 9
work(0).marker = 1234

ERASE work(0).nums
REDIM work(0).nums(2 TO 4)

ok = (LBOUND(work(0).nums) = 2 AND UBOUND(work(0).nums) = 4)
ok = ok AND (work(0).nums(2) = 0 AND work(0).nums(3) = 0 AND work(0).nums(4) = 0)
ok = ok AND (work(0).marker = 1234)

work(0).nums(2) = 20
work(0).nums(4) = 40
ok = ok AND (work(0).nums(2) = 20 AND work(0).nums(4) = 40)

IF ok THEN PRINT "PASS t264_erase_member_then_rebuild_bounds" ELSE PRINT "FAIL t264_erase_member_then_rebuild_bounds"
SYSTEM
