$Console:Only
OPTION _EXPLICIT

TYPE T271
    codes(0 TO 1) _Static AS STRING * 4
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 0) AS T271

work(0).codes(0) = "A000"
work(0).codes(1) = "A111"
work(0).nums(0) = 10
work(0).nums(1) = 11

REDIM _RETAIN work(0 TO 1) AS T271
work(1).codes(0) = "B000"
work(1).codes(1) = "B111"
work(1).nums(0) = 20
work(1).nums(1) = 21

ok = (LBOUND(work) = 0 AND UBOUND(work) = 1)
ok = ok AND (work(0).codes(0) = "A000" AND work(0).codes(1) = "A111")
ok = ok AND (work(0).nums(0) = 10 AND work(0).nums(1) = 11)
ok = ok AND (work(1).codes(0) = "B000" AND work(1).codes(1) = "B111")
ok = ok AND (work(1).nums(0) = 20 AND work(1).nums(1) = 21)

REDIM _RETAIN work(0).nums(0 TO 3)
work(0).nums(2) = 12
work(0).nums(3) = 13

ok = ok AND (LBOUND(work(0).nums) = 0 AND UBOUND(work(0).nums) = 3)
ok = ok AND (work(0).nums(0) = 10 AND work(0).nums(1) = 11 AND work(0).nums(2) = 12 AND work(0).nums(3) = 13)
ok = ok AND (work(0).codes(0) = "A000" AND work(0).codes(1) = "A111")

IF ok THEN PRINT "PASS t271_dynamic_static_parent_retain" ELSE PRINT "FAIL t271_dynamic_static_parent_retain"
SYSTEM
