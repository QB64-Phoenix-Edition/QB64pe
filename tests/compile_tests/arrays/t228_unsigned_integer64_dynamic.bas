$Console:Only
OPTION _EXPLICIT

TYPE T228
    nums(0 TO 2) _DynamicField AS _UNSIGNED _INTEGER64
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T228

work(0).nums(0) = 1000000000
work(0).nums(1) = 2000000000
work(0).nums(2) = 3000000000
ok = (work(0).nums(0) + work(0).nums(1) = work(0).nums(2))
IF ok THEN PRINT "PASS t228_unsigned_integer64_dynamic" ELSE PRINT "FAIL t228_unsigned_integer64_dynamic"
SYSTEM
