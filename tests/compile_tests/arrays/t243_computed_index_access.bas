$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T243
    nums(0 TO 5) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
DIM idxA AS LONG
DIM idxB AS LONG
REDIM work(0) AS T243

idxA = 2
idxB = idxA + 3
work(0).nums(idxA) = 200
work(0).nums(idxB) = 500
ok = (work(0).nums(2) = 200 AND work(0).nums(5) = 500 AND work(0).nums(idxA) + work(0).nums(idxB) = 700)
IF ok THEN PRINT "PASS t243_computed_index_access" ELSE PRINT "FAIL t243_computed_index_access"
SYSTEM
