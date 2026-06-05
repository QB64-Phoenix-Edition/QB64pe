$Console:Only
OPTION _EXPLICIT

TYPE T221
    nums(0 TO 2) _Dynamic AS _BYTE
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T221

work(0).nums(0) = -120
work(0).nums(1) = 0
work(0).nums(2) = 120
ok = (work(0).nums(0) = -120 AND work(0).nums(1) = 0 AND work(0).nums(2) = 120)
IF ok THEN PRINT "PASS t221_signed_byte_dynamic" ELSE PRINT "FAIL t221_signed_byte_dynamic"
SYSTEM
