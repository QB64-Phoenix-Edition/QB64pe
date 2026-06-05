$Console:Only
OPTION _EXPLICIT

TYPE T280
    fixedNums(0 TO 1) _Static AS LONG
    dynNums(0 TO 1) _Dynamic AS LONG
    dynText(0 TO 1) _Dynamic AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 2) AS T280

work(0).fixedNums(0) = 10
work(0).fixedNums(1) = 11
REDIM work(0).dynNums(0 TO 2)
work(0).dynNums(0) = 100
work(0).dynNums(1) = 101
work(0).dynNums(2) = 102
work(0).dynText(0) = "zero-a"
work(0).dynText(1) = "zero-b"

work(2).fixedNums(0) = 30
work(2).fixedNums(1) = 31
REDIM work(2).dynNums(5 TO 6)
work(2).dynNums(5) = 305
work(2).dynNums(6) = 306
REDIM work(2).dynText(7 TO 8)
work(2).dynText(7) = "two-a"
work(2).dynText(8) = "two-b"

SWAP work(0), work(2)

ok = (work(0).fixedNums(0) = 30 AND work(0).fixedNums(1) = 31)
ok = ok AND (LBOUND(work(0).dynNums) = 5 AND UBOUND(work(0).dynNums) = 6)
ok = ok AND (work(0).dynNums(5) = 305 AND work(0).dynNums(6) = 306)
ok = ok AND (LBOUND(work(0).dynText) = 7 AND UBOUND(work(0).dynText) = 8)
ok = ok AND (work(0).dynText(7) = "two-a" AND work(0).dynText(8) = "two-b")
ok = ok AND (work(2).fixedNums(0) = 10 AND work(2).fixedNums(1) = 11)
ok = ok AND (LBOUND(work(2).dynNums) = 0 AND UBOUND(work(2).dynNums) = 2)
ok = ok AND (work(2).dynNums(0) = 100 AND work(2).dynNums(2) = 102)
ok = ok AND (work(2).dynText(0) = "zero-a" AND work(2).dynText(1) = "zero-b")

ERASE work(2).dynNums
REDIM work(2).dynNums(-4 TO -3)
work(2).dynNums(-4) = -404
work(2).dynNums(-3) = -403
ok = ok AND (work(2).fixedNums(0) = 10 AND work(2).fixedNums(1) = 11)
ok = ok AND (LBOUND(work(2).dynNums) = -4 AND UBOUND(work(2).dynNums) = -3)
ok = ok AND (work(2).dynNums(-4) = -404 AND work(2).dynNums(-3) = -403)

ERASE work
REDIM work(0 TO 0) AS T280
work(0).fixedNums(0) = 90
work(0).dynNums(0) = 900
work(0).dynText(0) = "fresh"
ok = ok AND (LBOUND(work) = 0 AND UBOUND(work) = 0)
ok = ok AND (work(0).fixedNums(0) = 90 AND work(0).fixedNums(1) = 0)
ok = ok AND (LBOUND(work(0).dynNums) = 0 AND UBOUND(work(0).dynNums) = 1)
ok = ok AND (work(0).dynNums(0) = 900 AND work(0).dynNums(1) = 0)
ok = ok AND (work(0).dynText(0) = "fresh" AND work(0).dynText(1) = "")

IF ok THEN PRINT "PASS t280_static_dynamic_swap_erase_parent_reuse" ELSE PRINT "FAIL t280_static_dynamic_swap_erase_parent_reuse"
SYSTEM
