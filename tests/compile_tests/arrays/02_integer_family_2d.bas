OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE IntegerFamily
    AByte( 0 To 1 , 0 To 2) AS _BYTE
    AUByte( 0 To 1 , 0 To 2) AS _UNSIGNED _BYTE
    AInt( 0 To 1 , 0 To 2) AS INTEGER
    AUInt( 0 To 1 , 0 To 2) AS _UNSIGNED INTEGER
    ALong( 0 To 1 , 0 To 2) AS LONG
    AULong( 0 To 1 , 0 To 2) AS _UNSIGNED LONG
    AI64( 0 To 1 , 0 To 2) AS _INTEGER64
    AUI64( 0 To 1 , 0 To 2) AS _UNSIGNED _INTEGER64
END TYPE

DIM t AS IntegerFamily
DIM i AS LONG
DIM j AS LONG
DIM k AS LONG

FOR i = 0 TO 1
    FOR j = 0 TO 2
        t.AByte(i, j) = -10 - i - j
        t.AUByte(i, j) = 200 + i + j
        t.AInt(i, j) = -1000 - i * 10 - j
        t.AUInt(i, j) = 60000 + i * 10 + j
        t.ALong(i, j) = -100000 - i * 10 - j
        t.AULong(i, j) = 2000000000 + i * 10 + j
        t.AI64(i, j) = -9000000000 - i * 10 - j
        t.AUI64(i, j) = 900000000000 + i * 10 + j
    NEXT j
NEXT i

CheckTrue "_Byte sample", t.AByte(1, 2) = -13
CheckTrue "_Unsigned _Byte sample", t.AUByte(1, 2) = 203
CheckTrue "Integer sample", t.AInt(1, 2) = -1012
CheckTrue "_Unsigned Integer sample", t.AUInt(1, 2) = 60012
CheckTrue "Long sample", t.ALong(1, 2) = -100012
CheckTrue "_Unsigned Long sample", t.AULong(1, 2) = 2000000012
CheckTrue "_Integer64 sample", t.AI64(1, 2) = -9000000012
CheckTrue "_Unsigned _Integer64 sample", t.AUI64(1, 2) = 900000000012
CheckTrue "LBOUND dim1", LBOUND(t.AInt, 1) = 0
CheckTrue "UBOUND dim2", UBOUND(t.AInt, 2) = 2

FinishTest
SYSTEM

SUB CheckTrue (label AS STRING, condition AS LONG)
    IF condition THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label
    END IF
END SUB

SUB CheckText (label AS STRING, actual AS STRING, expected AS STRING)
    IF actual = expected THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label; " expected=["; expected; "] actual=["; actual; "]"
    END IF
END SUB

SUB CheckNear (label AS STRING, actual AS DOUBLE, expected AS DOUBLE, tolerance AS DOUBLE)
    IF ABS(actual - expected) <= tolerance THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label; " expected="; expected; " actual="; actual
    END IF
END SUB

SUB FinishTest
    IF FailCount = 0 THEN
        PRINT "RESULT: PASS"
    ELSE
        PRINT "RESULT: FAIL"; FailCount; "failure(s)"
    END IF
END SUB
