$CONSOLE:ONLY
$UNSTABLE:TYPEFIELDS

OPTION _EXPLICIT

TYPE T220
    headText AS STRING
    nums(0 TO 1) _DYNAMIC AS LONG
    tailText AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work AS T220

work.headText = "alpha"
work.nums(1) = 456
work.tailText = "omega"
ok = (work.headText = "alpha" AND work.nums(1) = 456 AND work.tailText = "omega")
IF ok THEN PRINT "PASS t220_2" ELSE PRINT "FAIL t220_2"
SYSTEM

