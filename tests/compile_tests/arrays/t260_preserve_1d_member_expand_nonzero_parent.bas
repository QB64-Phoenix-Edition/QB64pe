$Console:Only
OPTION _EXPLICIT

TYPE T260
    nums(1 TO 3) _Dynamic AS LONG
    tagText AS STRING * 4
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 1) AS T260

work(0).nums(1) = 10
work(0).nums(2) = 20
work(0).nums(3) = 30
work(0).tagText = "ZERO"

work(1).nums(1) = 101
work(1).nums(2) = 102
work(1).nums(3) = 103
work(1).tagText = "ONE1"

REDIM _PRESERVE work(1).nums(1 TO 5)
work(1).nums(4) = 104
work(1).nums(5) = 105

ok = (LBOUND(work(1).nums) = 1 AND UBOUND(work(1).nums) = 5)
ok = ok AND (work(1).nums(1) = 101 AND work(1).nums(2) = 102 AND work(1).nums(3) = 103)
ok = ok AND (work(1).nums(4) = 104 AND work(1).nums(5) = 105)
ok = ok AND (work(0).nums(1) = 10 AND work(0).nums(3) = 30 AND work(0).tagText = "ZERO")

IF ok THEN PRINT "PASS t260_preserve_1d_member_expand_nonzero_parent" ELSE PRINT "FAIL t260_preserve_1d_member_expand_nonzero_parent"
SYSTEM
