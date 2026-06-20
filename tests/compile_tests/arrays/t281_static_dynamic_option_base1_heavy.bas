$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT
OPTION BASE 1

TYPE T281
    fixedNums(3) _Static AS LONG
    dynNums(2) _Dynamic AS LONG
    dynCodes(2) _Dynamic AS STRING * 3
END TYPE

DIM ok AS _BYTE
REDIM work(1 TO 2) AS T281

work(1).fixedNums(1) = 11
work(1).fixedNums(2) = 12
work(1).fixedNums(3) = 13
work(1).dynNums(1) = 101
work(1).dynNums(2) = 102
work(1).dynCodes(1) = "A1"
work(1).dynCodes(2) = "A2"

REDIM work(2).dynNums(4 TO 5)
work(2).fixedNums(1) = 21
work(2).fixedNums(2) = 22
work(2).fixedNums(3) = 23
work(2).dynNums(4) = 204
work(2).dynNums(5) = 205
REDIM work(2).dynCodes(6 TO 7)
work(2).dynCodes(6) = "B6"
work(2).dynCodes(7) = "B7"

REDIM _RETAIN work(1 TO 3) AS T281
work(3).fixedNums(1) = 31
work(3).fixedNums(3) = 33
work(3).dynNums(1) = 301
work(3).dynCodes(1) = "C1"

ERASE work(1).dynNums
REDIM work(1).dynNums(-1 TO 1)
work(1).dynNums(-1) = -101
work(1).dynNums(0) = 0
work(1).dynNums(1) = 101

ok = (LBOUND(work) = 1 AND UBOUND(work) = 3)
ok = ok AND (LBOUND(work(1).fixedNums) = 1 AND UBOUND(work(1).fixedNums) = 3)
ok = ok AND (work(1).fixedNums(1) = 11 AND work(1).fixedNums(3) = 13)
ok = ok AND (LBOUND(work(1).dynNums) = -1 AND UBOUND(work(1).dynNums) = 1)
ok = ok AND (work(1).dynNums(-1) = -101 AND work(1).dynNums(0) = 0 AND work(1).dynNums(1) = 101)
ok = ok AND (LBOUND(work(1).dynCodes) = 1 AND UBOUND(work(1).dynCodes) = 2)
ok = ok AND (work(1).dynCodes(1) = "A1 " AND work(1).dynCodes(2) = "A2 ")
ok = ok AND (work(2).fixedNums(1) = 21 AND work(2).fixedNums(3) = 23)
ok = ok AND (LBOUND(work(2).dynNums) = 4 AND UBOUND(work(2).dynNums) = 5)
ok = ok AND (work(2).dynNums(4) = 204 AND work(2).dynNums(5) = 205)
ok = ok AND (LBOUND(work(2).dynCodes) = 6 AND UBOUND(work(2).dynCodes) = 7)
ok = ok AND (work(2).dynCodes(6) = "B6 " AND work(2).dynCodes(7) = "B7 ")
ok = ok AND (work(3).fixedNums(1) = 31 AND work(3).fixedNums(2) = 0 AND work(3).fixedNums(3) = 33)
ok = ok AND (LBOUND(work(3).dynNums) = 1 AND UBOUND(work(3).dynNums) = 2)
ok = ok AND (work(3).dynNums(1) = 301 AND work(3).dynNums(2) = 0)
ok = ok AND (work(3).dynCodes(1) = "C1 " AND work(3).dynCodes(2) = STRING$(3, 0))

IF ok THEN PRINT "PASS t281_static_dynamic_option_base1_heavy" ELSE PRINT "FAIL t281_static_dynamic_option_base1_heavy"
SYSTEM
