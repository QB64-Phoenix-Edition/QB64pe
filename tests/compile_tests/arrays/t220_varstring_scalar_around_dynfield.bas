$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T220
    headText AS STRING
    nums(0 TO 1) _Dynamic AS LONG
    tailText AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T220

work(0).headText = "alpha"
work(0).nums(1) = 456
work(0).tailText = "omega"
ok = (work(0).headText = "alpha" AND work(0).nums(1) = 456 AND work(0).tailText = "omega")
IF ok THEN PRINT "PASS t220_varstring_scalar_around_dynfield" ELSE PRINT "FAIL t220_varstring_scalar_around_dynfield"
SYSTEM
