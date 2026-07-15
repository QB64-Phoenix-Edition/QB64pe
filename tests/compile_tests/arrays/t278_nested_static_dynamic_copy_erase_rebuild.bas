$Console:Only
OPTION _EXPLICIT
$Unstable:TypeFields

TYPE Leaf278
    fixedNums(0 TO 1) _Static AS LONG
    fixedTag AS STRING * 4
    dynNums(0 TO 1) _Dynamic AS LONG
END TYPE

TYPE T278
    fixedSlots(0 TO 1) _Static AS LONG
    leaves(0 TO 1) _Dynamic AS Leaf278
END TYPE

DIM ok AS _BYTE
REDIM src(0 TO 0) AS T278
REDIM dst(0 TO 0) AS T278

src(0).fixedSlots(0) = 7
src(0).fixedSlots(1) = 8
src(0).leaves(0).fixedNums(0) = 10
src(0).leaves(0).fixedNums(1) = 11
src(0).leaves(0).fixedTag = "L0"
src(0).leaves(0).dynNums(0) = 100
src(0).leaves(0).dynNums(1) = 101
src(0).leaves(1).fixedNums(0) = 20
src(0).leaves(1).fixedNums(1) = 21
src(0).leaves(1).fixedTag = "L1"
REDIM src(0).leaves(1).dynNums(5 TO 6)
src(0).leaves(1).dynNums(5) = 205
src(0).leaves(1).dynNums(6) = 206

dst() = src()

src(0).fixedSlots(0) = -7
src(0).leaves(0).fixedNums(0) = -10
src(0).leaves(0).dynNums(0) = -100
ERASE src(0).leaves(1).dynNums
REDIM src(0).leaves(1).dynNums(-3 TO -2)
src(0).leaves(1).dynNums(-3) = -303
src(0).leaves(1).dynNums(-2) = -302

ok = (dst(0).fixedSlots(0) = 7 AND dst(0).fixedSlots(1) = 8)
ok = ok AND (dst(0).leaves(0).fixedNums(0) = 10 AND dst(0).leaves(0).fixedNums(1) = 11)
ok = ok AND (dst(0).leaves(0).fixedTag = "L0  ")
ok = ok AND (LBOUND(dst(0).leaves(0).dynNums) = 0 AND UBOUND(dst(0).leaves(0).dynNums) = 1)
ok = ok AND (dst(0).leaves(0).dynNums(0) = 100 AND dst(0).leaves(0).dynNums(1) = 101)
ok = ok AND (dst(0).leaves(1).fixedNums(0) = 20 AND dst(0).leaves(1).fixedNums(1) = 21)
ok = ok AND (dst(0).leaves(1).fixedTag = "L1  ")
ok = ok AND (LBOUND(dst(0).leaves(1).dynNums) = 5 AND UBOUND(dst(0).leaves(1).dynNums) = 6)
ok = ok AND (dst(0).leaves(1).dynNums(5) = 205 AND dst(0).leaves(1).dynNums(6) = 206)
ok = ok AND (src(0).leaves(1).fixedNums(0) = 20 AND src(0).leaves(1).fixedNums(1) = 21)
ok = ok AND (LBOUND(src(0).leaves(1).dynNums) = -3 AND UBOUND(src(0).leaves(1).dynNums) = -2)
ok = ok AND (src(0).leaves(1).dynNums(-3) = -303 AND src(0).leaves(1).dynNums(-2) = -302)

IF ok THEN PRINT "PASS t278_nested_static_dynamic_copy_erase_rebuild" ELSE PRINT "FAIL t278_nested_static_dynamic_copy_erase_rebuild"
SYSTEM
