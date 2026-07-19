$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T218
    idNum AS LONG
    numsA(0 TO 1) _Dynamic AS LONG
    middleNum AS LONG
    numsB(0 TO 1) _Dynamic AS LONG
    tailNum AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T218

work(0).idNum = 11
work(0).numsA(1) = 22
work(0).middleNum = 33
work(0).numsB(0) = 44
work(0).tailNum = 55
ok = (work(0).idNum = 11 AND work(0).numsA(1) = 22 AND work(0).middleNum = 33 AND work(0).numsB(0) = 44 AND work(0).tailNum = 55)
IF ok THEN PRINT "PASS t218_scalar_members_around_dynfields" ELSE PRINT "FAIL t218_scalar_members_around_dynfields"
SYSTEM
