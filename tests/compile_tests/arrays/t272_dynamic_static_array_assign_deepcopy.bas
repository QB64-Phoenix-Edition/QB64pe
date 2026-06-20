$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T272
    tags(0 TO 1) _Static AS STRING * 5
    values(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM src(0 TO 1) AS T272
REDIM dst(0 TO 1) AS T272

src(0).tags(0) = "S0A"
src(0).tags(1) = "S0B"
REDIM src(0).values(0 TO 2)
src(0).values(0) = 100
src(0).values(1) = 101
src(0).values(2) = 102

src(1).tags(0) = "S1A"
src(1).tags(1) = "S1B"
REDIM src(1).values(5 TO 6)
src(1).values(5) = 205
src(1).values(6) = 206

dst() = src()

src(0).tags(0) = "BAD0"
src(0).values(0) = -1
src(1).tags(1) = "BAD1"
src(1).values(6) = -2

ok = (dst(0).tags(0) = "S0A  " AND dst(0).tags(1) = "S0B  ")
ok = ok AND (LBOUND(dst(0).values) = 0 AND UBOUND(dst(0).values) = 2)
ok = ok AND (dst(0).values(0) = 100 AND dst(0).values(1) = 101 AND dst(0).values(2) = 102)
ok = ok AND (dst(1).tags(0) = "S1A  " AND dst(1).tags(1) = "S1B  ")
ok = ok AND (LBOUND(dst(1).values) = 5 AND UBOUND(dst(1).values) = 6)
ok = ok AND (dst(1).values(5) = 205 AND dst(1).values(6) = 206)

IF ok THEN PRINT "PASS t272_dynamic_static_array_assign_deepcopy" ELSE PRINT "FAIL t272_dynamic_static_array_assign_deepcopy"
SYSTEM
