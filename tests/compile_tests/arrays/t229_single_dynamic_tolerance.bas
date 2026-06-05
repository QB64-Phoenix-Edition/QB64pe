$Console:Only
OPTION _EXPLICIT

TYPE T229
    nums(0 TO 2) _Dynamic AS SINGLE
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T229

work(0).nums(0) = 1.25
work(0).nums(1) = 2.5
work(0).nums(2) = work(0).nums(0) + work(0).nums(1)
ok = (ABS(work(0).nums(2) - 3.75) < .0001)
IF ok THEN PRINT "PASS t229_single_dynamic_tolerance" ELSE PRINT "FAIL t229_single_dynamic_tolerance"
SYSTEM
