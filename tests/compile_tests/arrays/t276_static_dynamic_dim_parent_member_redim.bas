$Console:Only
OPTION _EXPLICIT

TYPE T276
    fixedNums(0 TO 1) _Static AS LONG
    dynNums(0 TO 1) _Dynamic AS LONG
    dynNames(0 TO 1) _Dynamic AS STRING
END TYPE

DIM ok AS _BYTE
DIM work(0 TO 1) AS T276

work(0).fixedNums(0) = 10
work(0).fixedNums(1) = 11
work(0).dynNums(0) = 100
work(0).dynNums(1) = 101
work(0).dynNames(0) = "alpha"
work(0).dynNames(1) = "beta"

work(1).fixedNums(0) = 20
work(1).fixedNums(1) = 21
REDIM work(1).dynNums(4 TO 5)
work(1).dynNums(4) = 204
work(1).dynNums(5) = 205
REDIM work(1).dynNames(2 TO 4)
work(1).dynNames(2) = "two"
work(1).dynNames(3) = "three"
work(1).dynNames(4) = "four"

REDIM _RETAIN work(0).dynNums(0 TO 3)
work(0).dynNums(2) = 102
work(0).dynNums(3) = 103
ERASE work(1).dynNames
REDIM work(1).dynNames(-1 TO 0)
work(1).dynNames(-1) = "minus"
work(1).dynNames(0) = "zero"

ok = (LBOUND(work) = 0 AND UBOUND(work) = 1)
ok = ok AND (work(0).fixedNums(0) = 10 AND work(0).fixedNums(1) = 11)
ok = ok AND (LBOUND(work(0).dynNums) = 0 AND UBOUND(work(0).dynNums) = 3)
ok = ok AND (work(0).dynNums(0) = 100 AND work(0).dynNums(1) = 101 AND work(0).dynNums(2) = 102 AND work(0).dynNums(3) = 103)
ok = ok AND (work(0).dynNames(0) = "alpha" AND work(0).dynNames(1) = "beta")
ok = ok AND (work(1).fixedNums(0) = 20 AND work(1).fixedNums(1) = 21)
ok = ok AND (LBOUND(work(1).dynNums) = 4 AND UBOUND(work(1).dynNums) = 5)
ok = ok AND (work(1).dynNums(4) = 204 AND work(1).dynNums(5) = 205)
ok = ok AND (LBOUND(work(1).dynNames) = -1 AND UBOUND(work(1).dynNames) = 0)
ok = ok AND (work(1).dynNames(-1) = "minus" AND work(1).dynNames(0) = "zero")

IF ok THEN PRINT "PASS t276_static_dynamic_dim_parent_member_redim" ELSE PRINT "FAIL t276_static_dynamic_dim_parent_member_redim"
SYSTEM
