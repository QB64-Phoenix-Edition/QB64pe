$Console:Only
OPTION _EXPLICIT

TYPE T217
    numsA(0 TO 2) _DynamicField AS LONG
    numsB(10 TO 12) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T217

work(0).numsA(0) = 7
work(0).numsA(2) = 9
work(0).numsB(10) = 100
work(0).numsB(12) = 120
ok = (work(0).numsA(0) = 7 AND work(0).numsA(2) = 9 AND work(0).numsB(10) = 100 AND work(0).numsB(12) = 120)
IF ok THEN PRINT "PASS t217_multi_members_independent" ELSE PRINT "FAIL t217_multi_members_independent"
SYSTEM
