$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T277
    fixedNums(0 TO 2) _Static AS LONG
    dynNums(0 TO 1) _Dynamic AS LONG
    dynText(0 TO 1) _Dynamic AS STRING * 4
END TYPE

DIM ok AS _BYTE
REDIM work(1 TO 1) AS T277

work(1).fixedNums(0) = 10
work(1).fixedNums(1) = 11
work(1).fixedNums(2) = 12
REDIM work(1).dynNums(3 TO 4)
work(1).dynNums(3) = 103
work(1).dynNums(4) = 104
work(1).dynText(0) = "A000"
work(1).dynText(1) = "A111"

REDIM _RETAIN work(1 TO 2) AS T277
work(2).fixedNums(0) = 20
work(2).fixedNums(1) = 21
work(2).fixedNums(2) = 22
REDIM work(2).dynNums(-2 TO -1)
work(2).dynNums(-2) = -202
work(2).dynNums(-1) = -201
work(2).dynText(0) = "B000"
work(2).dynText(1) = "B111"

ok = (LBOUND(work) = 1 AND UBOUND(work) = 2)
ok = ok AND (work(1).fixedNums(0) = 10 AND work(1).fixedNums(2) = 12)
ok = ok AND (LBOUND(work(1).dynNums) = 3 AND UBOUND(work(1).dynNums) = 4)
ok = ok AND (work(1).dynNums(3) = 103 AND work(1).dynNums(4) = 104)
ok = ok AND (work(1).dynText(0) = "A000" AND work(1).dynText(1) = "A111")
ok = ok AND (work(2).fixedNums(0) = 20 AND work(2).fixedNums(2) = 22)
ok = ok AND (LBOUND(work(2).dynNums) = -2 AND UBOUND(work(2).dynNums) = -1)
ok = ok AND (work(2).dynNums(-2) = -202 AND work(2).dynNums(-1) = -201)

REDIM work(1 TO 1) AS T277
ok = ok AND (LBOUND(work) = 1 AND UBOUND(work) = 1)
ok = ok AND (work(1).fixedNums(0) = 0 AND work(1).fixedNums(1) = 0 AND work(1).fixedNums(2) = 0)
ok = ok AND (LBOUND(work(1).dynNums) = 0 AND UBOUND(work(1).dynNums) = 1)
ok = ok AND (work(1).dynNums(0) = 0 AND work(1).dynNums(1) = 0)
ok = ok AND (LBOUND(work(1).dynText) = 0 AND UBOUND(work(1).dynText) = 1)
ok = ok AND (work(1).dynText(0) = STRING$(4, 0) AND work(1).dynText(1) = STRING$(4, 0))

IF ok THEN PRINT "PASS t277_static_dynamic_parent_redim_reset_retain" ELSE PRINT "FAIL t277_static_dynamic_parent_redim_reset_retain"
SYSTEM
