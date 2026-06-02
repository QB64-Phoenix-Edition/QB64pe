$Console:Only
OPTION _EXPLICIT

TYPE T211
    nums(5 TO 7) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T211

work(0).nums(5) = 105
work(0).nums(6) = 106
work(0).nums(7) = 107

ok = (LBOUND(work(0).nums) = 5 AND UBOUND(work(0).nums) = 7 AND work(0).nums(5) + work(0).nums(7) = 212)
IF ok THEN PRINT "PASS t211_positive_nonzero_bounds_1d" ELSE PRINT "FAIL t211_positive_nonzero_bounds_1d"
SYSTEM
