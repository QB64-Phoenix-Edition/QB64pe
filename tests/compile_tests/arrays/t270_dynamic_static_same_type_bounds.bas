$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T270
    fixedNums(0 TO 2) _Static AS LONG
    dynNums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 1) AS T270

work(0).fixedNums(0) = 10
work(0).fixedNums(1) = 11
work(0).fixedNums(2) = 12
work(1).fixedNums(0) = 20
work(1).fixedNums(1) = 21
work(1).fixedNums(2) = 22

REDIM work(0).dynNums(0 TO 2)
work(0).dynNums(0) = 100
work(0).dynNums(1) = 101
work(0).dynNums(2) = 102

REDIM work(1).dynNums(5 TO 6)
work(1).dynNums(5) = 205
work(1).dynNums(6) = 206

ok = (LBOUND(work(0).fixedNums) = 0 AND UBOUND(work(0).fixedNums) = 2)
ok = ok AND (LBOUND(work(1).fixedNums) = 0 AND UBOUND(work(1).fixedNums) = 2)
ok = ok AND (work(0).fixedNums(0) = 10 AND work(0).fixedNums(2) = 12)
ok = ok AND (work(1).fixedNums(0) = 20 AND work(1).fixedNums(2) = 22)
ok = ok AND (LBOUND(work(0).dynNums) = 0 AND UBOUND(work(0).dynNums) = 2)
ok = ok AND (LBOUND(work(1).dynNums) = 5 AND UBOUND(work(1).dynNums) = 6)
ok = ok AND (work(0).dynNums(0) = 100 AND work(0).dynNums(2) = 102)
ok = ok AND (work(1).dynNums(5) = 205 AND work(1).dynNums(6) = 206)

IF ok THEN PRINT "PASS t270_dynamic_static_same_type_bounds" ELSE PRINT "FAIL t270_dynamic_static_same_type_bounds"
SYSTEM
