$Console:Only
OPTION _EXPLICIT

TYPE T265
    nums(0 TO 1) _Dynamic AS LONG
    tagText AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 1) AS T265

REDIM work(0).nums(0 TO 2)
work(0).nums(0) = 10
work(0).nums(1) = 11
work(0).nums(2) = 12
work(0).tagText = "zero"

REDIM work(1).nums(5 TO 6)
work(1).nums(5) = 50
work(1).nums(6) = 60
work(1).tagText = "one"

SWAP work(0), work(1)

ok = (LBOUND(work(0).nums) = 5 AND UBOUND(work(0).nums) = 6)
ok = ok AND (work(0).nums(5) = 50 AND work(0).nums(6) = 60 AND work(0).tagText = "one")
ok = ok AND (LBOUND(work(1).nums) = 0 AND UBOUND(work(1).nums) = 2)
ok = ok AND (work(1).nums(0) = 10 AND work(1).nums(1) = 11 AND work(1).nums(2) = 12 AND work(1).tagText = "zero")

IF ok THEN PRINT "PASS t265_swap_parent_elements_different_member_bounds" ELSE PRINT "FAIL t265_swap_parent_elements_different_member_bounds"
SYSTEM
