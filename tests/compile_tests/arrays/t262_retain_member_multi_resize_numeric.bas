$Console:Only
OPTION _EXPLICIT

TYPE T262
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T262

work(0).nums(0) = 11
work(0).nums(1) = 22

REDIM _RETAIN work(0).nums(0 TO 4)
work(0).nums(2) = 33
work(0).nums(3) = 44
work(0).nums(4) = 55

REDIM _RETAIN work(0).nums(0 TO 2)
ok = (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 2)
ok = ok AND (work(0).nums(0) = 11 AND work(0).nums(1) = 22 AND work(0).nums(2) = 33)

REDIM _RETAIN work(0).nums(0 TO 6)
work(0).nums(5) = 66
work(0).nums(6) = 77

ok = ok AND (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 6)
ok = ok AND (work(0).nums(0) = 11 AND work(0).nums(1) = 22 AND work(0).nums(2) = 33)
ok = ok AND (work(0).nums(3) = 0 AND work(0).nums(4) = 0 AND work(0).nums(5) = 66 AND work(0).nums(6) = 77)

IF ok THEN PRINT "PASS t262_retain_member_multi_resize_numeric" ELSE PRINT "FAIL t262_retain_member_multi_resize_numeric"
SYSTEM
