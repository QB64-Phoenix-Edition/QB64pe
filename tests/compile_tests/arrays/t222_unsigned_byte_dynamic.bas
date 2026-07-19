$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T222
    nums(0 TO 2) _Dynamic AS _UNSIGNED _BYTE
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T222

work(0).nums(0) = 0
work(0).nums(1) = 250
work(0).nums(2) = 255
ok = (work(0).nums(0) = 0 AND work(0).nums(1) = 250 AND work(0).nums(2) = 255)
IF ok THEN PRINT "PASS t222_unsigned_byte_dynamic" ELSE PRINT "FAIL t222_unsigned_byte_dynamic"
SYSTEM
