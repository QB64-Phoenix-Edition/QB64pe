$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT
OPTION BASE 1

TYPE T273
    fixedNums(3) _Static AS LONG
    dynText(2) _Dynamic AS STRING * 4
END TYPE

DIM ok AS _BYTE
REDIM work(1 TO 1) AS T273

work(1).fixedNums(1) = 11
work(1).fixedNums(2) = 12
work(1).fixedNums(3) = 13
work(1).dynText(1) = "A001"
work(1).dynText(2) = "A002"

ok = (LBOUND(work(1).fixedNums) = 1 AND UBOUND(work(1).fixedNums) = 3)
ok = ok AND (LBOUND(work(1).dynText) = 1 AND UBOUND(work(1).dynText) = 2)
ok = ok AND (work(1).fixedNums(1) = 11 AND work(1).fixedNums(3) = 13)
ok = ok AND (work(1).dynText(1) = "A001" AND work(1).dynText(2) = "A002")

REDIM _RETAIN work(1).dynText(1 TO 3)
work(1).dynText(3) = "A003"

ok = ok AND (LBOUND(work(1).dynText) = 1 AND UBOUND(work(1).dynText) = 3)
ok = ok AND (work(1).dynText(1) = "A001" AND work(1).dynText(2) = "A002" AND work(1).dynText(3) = "A003")
ok = ok AND (work(1).fixedNums(1) = 11 AND work(1).fixedNums(2) = 12 AND work(1).fixedNums(3) = 13)

IF ok THEN PRINT "PASS t273_dynamic_static_option_base_1" ELSE PRINT "FAIL t273_dynamic_static_option_base_1"
SYSTEM
