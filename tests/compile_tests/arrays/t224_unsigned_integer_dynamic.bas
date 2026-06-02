$Console:Only
OPTION _EXPLICIT

TYPE T224
    nums(0 TO 2) _DynamicField AS _UNSIGNED INTEGER
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T224

work(0).nums(0) = 0
work(0).nums(1) = 60000
work(0).nums(2) = 65535
ok = (work(0).nums(0) = 0 AND work(0).nums(1) = 60000 AND work(0).nums(2) = 65535)
IF ok THEN PRINT "PASS t224_unsigned_integer_dynamic" ELSE PRINT "FAIL t224_unsigned_integer_dynamic"
SYSTEM
