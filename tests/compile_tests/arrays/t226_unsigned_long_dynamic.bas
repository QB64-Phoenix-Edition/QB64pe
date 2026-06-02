$Console:Only
OPTION _EXPLICIT

TYPE T226
    nums(0 TO 2) _DynamicField AS _UNSIGNED LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T226

work(0).nums(0) = 123456789
work(0).nums(1) = 987654321
work(0).nums(2) = 1111111110
ok = (work(0).nums(0) + work(0).nums(1) = work(0).nums(2))
IF ok THEN PRINT "PASS t226_unsigned_long_dynamic" ELSE PRINT "FAIL t226_unsigned_long_dynamic"
SYSTEM
