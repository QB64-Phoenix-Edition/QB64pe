$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T225
    nums(0 TO 3) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
DIM totalVal AS LONG
REDIM work(0) AS T225

work(0).nums(0) = -100000
work(0).nums(1) = 1000
work(0).nums(2) = 2000
work(0).nums(3) = 97000
totalVal = work(0).nums(0) + work(0).nums(1) + work(0).nums(2) + work(0).nums(3)
ok = (totalVal = 0)
IF ok THEN PRINT "PASS t225_long_dynamic_arithmetic" ELSE PRINT "FAIL t225_long_dynamic_arithmetic"
SYSTEM
