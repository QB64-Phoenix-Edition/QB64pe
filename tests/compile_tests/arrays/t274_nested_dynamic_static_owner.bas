$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE Leaf274
    fixedNums(0 TO 1) _Static AS LONG
    dynNums(0 TO 1) _Dynamic AS LONG
END TYPE

TYPE T274
    fixedCodes(0 TO 1) _Static AS STRING * 3
    items(0 TO 1) _Dynamic AS Leaf274
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 0) AS T274

work(0).fixedCodes(0) = "A0"
work(0).fixedCodes(1) = "A1"
work(0).items(0).fixedNums(0) = 10
work(0).items(0).fixedNums(1) = 11
work(0).items(0).dynNums(0) = 100
work(0).items(0).dynNums(1) = 101
work(0).items(1).fixedNums(0) = 20
work(0).items(1).fixedNums(1) = 21

REDIM work(0).items(1).dynNums(3 TO 4)
work(0).items(1).dynNums(3) = 203
work(0).items(1).dynNums(4) = 204

ok = (work(0).fixedCodes(0) = "A0 " AND work(0).fixedCodes(1) = "A1 ")
ok = ok AND (LBOUND(work(0).items) = 0 AND UBOUND(work(0).items) = 1)
ok = ok AND (LBOUND(work(0).items(0).fixedNums) = 0 AND UBOUND(work(0).items(0).fixedNums) = 1)
ok = ok AND (work(0).items(0).fixedNums(0) = 10 AND work(0).items(0).fixedNums(1) = 11)
ok = ok AND (work(0).items(0).dynNums(0) = 100 AND work(0).items(0).dynNums(1) = 101)
ok = ok AND (work(0).items(1).fixedNums(0) = 20 AND work(0).items(1).fixedNums(1) = 21)
ok = ok AND (LBOUND(work(0).items(1).dynNums) = 3 AND UBOUND(work(0).items(1).dynNums) = 4)
ok = ok AND (work(0).items(1).dynNums(3) = 203 AND work(0).items(1).dynNums(4) = 204)

IF ok THEN PRINT "PASS t274_nested_dynamic_static_owner" ELSE PRINT "FAIL t274_nested_dynamic_static_owner"
SYSTEM
