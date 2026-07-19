$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T223
    nums(0 TO 2) _Dynamic AS INTEGER
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T223

work(0).nums(0) = -30000
work(0).nums(1) = 123
work(0).nums(2) = 30000
ok = (work(0).nums(0) = -30000 AND work(0).nums(1) = 123 AND work(0).nums(2) = 30000)
IF ok THEN PRINT "PASS t223_integer_dynamic" ELSE PRINT "FAIL t223_integer_dynamic"
SYSTEM
