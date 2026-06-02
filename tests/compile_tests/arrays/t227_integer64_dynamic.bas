$Console:Only
OPTION _EXPLICIT

TYPE T227
    nums(0 TO 2) _DynamicField AS _INTEGER64
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T227

work(0).nums(0) = -2000000000
work(0).nums(1) = 3000000000
work(0).nums(2) = 1000000000
ok = (work(0).nums(0) + work(0).nums(1) = work(0).nums(2))
IF ok THEN PRINT "PASS t227_integer64_dynamic" ELSE PRINT "FAIL t227_integer64_dynamic"
SYSTEM
