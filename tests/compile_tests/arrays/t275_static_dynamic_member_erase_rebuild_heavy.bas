$Console:Only
OPTION _EXPLICIT

TYPE T275
    fixedNums(0 TO 2) _Static AS LONG
    fixedCodes(0 TO 1) _Static AS STRING * 4
    dynNums(0 TO 1) _Dynamic AS LONG
    dynCodes(0 TO 1) _Dynamic AS STRING * 3
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 1) AS T275

work(0).fixedNums(0) = 10
work(0).fixedNums(1) = 11
work(0).fixedNums(2) = 12
work(0).fixedCodes(0) = "A0"
work(0).fixedCodes(1) = "A1"
work(0).dynNums(0) = 100
work(0).dynNums(1) = 101
work(0).dynCodes(0) = "X0"
work(0).dynCodes(1) = "X1"

work(1).fixedNums(0) = 20
work(1).fixedNums(1) = 21
work(1).fixedNums(2) = 22
work(1).fixedCodes(0) = "B0"
work(1).fixedCodes(1) = "B1"
REDIM work(1).dynNums(5 TO 7)
work(1).dynNums(5) = 205
work(1).dynNums(6) = 206
work(1).dynNums(7) = 207
REDIM work(1).dynCodes(3 TO 4)
work(1).dynCodes(3) = "Y3"
work(1).dynCodes(4) = "Y4"

ERASE work(0).dynNums
ERASE work(0).dynCodes
REDIM work(0).dynNums(-2 TO 0)
work(0).dynNums(-2) = -200
work(0).dynNums(-1) = -100
work(0).dynNums(0) = 0
REDIM work(0).dynCodes(8 TO 9)
work(0).dynCodes(8) = "N8"
work(0).dynCodes(9) = "N9"

ok = (work(0).fixedNums(0) = 10 AND work(0).fixedNums(2) = 12)
ok = ok AND (work(0).fixedCodes(0) = "A0  " AND work(0).fixedCodes(1) = "A1  ")
ok = ok AND (LBOUND(work(0).dynNums) = -2 AND UBOUND(work(0).dynNums) = 0)
ok = ok AND (work(0).dynNums(-2) = -200 AND work(0).dynNums(-1) = -100 AND work(0).dynNums(0) = 0)
ok = ok AND (LBOUND(work(0).dynCodes) = 8 AND UBOUND(work(0).dynCodes) = 9)
ok = ok AND (work(0).dynCodes(8) = "N8 " AND work(0).dynCodes(9) = "N9 ")
ok = ok AND (work(1).fixedNums(0) = 20 AND work(1).fixedNums(2) = 22)
ok = ok AND (work(1).fixedCodes(0) = "B0  " AND work(1).fixedCodes(1) = "B1  ")
ok = ok AND (LBOUND(work(1).dynNums) = 5 AND UBOUND(work(1).dynNums) = 7)
ok = ok AND (work(1).dynNums(5) = 205 AND work(1).dynNums(7) = 207)
ok = ok AND (LBOUND(work(1).dynCodes) = 3 AND UBOUND(work(1).dynCodes) = 4)
ok = ok AND (work(1).dynCodes(3) = "Y3 " AND work(1).dynCodes(4) = "Y4 ")

IF ok THEN PRINT "PASS t275_static_dynamic_member_erase_rebuild_heavy" ELSE PRINT "FAIL t275_static_dynamic_member_erase_rebuild_heavy"
SYSTEM
