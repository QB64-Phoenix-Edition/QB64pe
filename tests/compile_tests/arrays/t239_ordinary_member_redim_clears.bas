$Console:Only
OPTION _EXPLICIT

TYPE T239
    nums(0 TO 2) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T239

work(0).nums(0) = 7
work(0).nums(1) = 8
work(0).nums(2) = 9
REDIM work(0).nums(0 TO 2)
ok = (work(0).nums(0) = 0 AND work(0).nums(1) = 0 AND work(0).nums(2) = 0)
IF ok THEN PRINT "PASS t239_ordinary_member_redim_clears" ELSE PRINT "FAIL t239_ordinary_member_redim_clears"
SYSTEM
