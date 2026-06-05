$Console:Only
OPTION _EXPLICIT

TYPE T230
    nums(0 TO 2) _Dynamic AS DOUBLE
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T230

work(0).nums(0) = 1.125#
work(0).nums(1) = 2.25#
work(0).nums(2) = work(0).nums(0) * work(0).nums(1)
ok = (ABS(work(0).nums(2) - 2.53125#) < .0000001#)
IF ok THEN PRINT "PASS t230_double_dynamic_tolerance" ELSE PRINT "FAIL t230_double_dynamic_tolerance"
SYSTEM
