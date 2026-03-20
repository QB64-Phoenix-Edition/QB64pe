$CONSOLE:ONLY
OPTION _EXPLICIT

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE IntegerFamily
    AByte( 0 To 1 , 0 To 1 , 0 To 1) AS _BYTE
    AUByte( 0 To 1 , 0 To 1 , 0 To 1) AS _UNSIGNED _BYTE
    AInt( 0 To 1 , 0 To 1 , 0 To 1) AS INTEGER
    AUInt( 0 To 1 , 0 To 1 , 0 To 1) AS _UNSIGNED INTEGER
    ALong( 0 To 1 , 0 To 1 , 0 To 1) AS LONG
    AULong( 0 To 1 , 0 To 1 , 0 To 1) AS _UNSIGNED LONG
    AI64( 0 To 1 , 0 To 1 , 0 To 1) AS _INTEGER64
    AUI64( 0 To 1 , 0 To 1 , 0 To 1) AS _UNSIGNED _INTEGER64
END TYPE

DIM t AS IntegerFamily
DIM i AS LONG
DIM j AS LONG
DIM k AS LONG

FOR i = 0 TO 1
    FOR j = 0 TO 1
        FOR k = 0 TO 1
            t.AByte(i, j, k) = -10 - i - j - k
            t.AUByte(i, j, k) = 200 + i + j + k
            t.AInt(i, j, k) = -1000 - i * 100 - j * 10 - k
            t.AUInt(i, j, k) = 60000 + i * 100 + j * 10 + k
            t.ALong(i, j, k) = -100000 - i * 100 - j * 10 - k
            t.AULong(i, j, k) = 2000000000 + i * 100 + j * 10 + k
            t.AI64(i, j, k) = -9000000000 - i * 100 - j * 10 - k
            t.AUI64(i, j, k) = 900000000000 + i * 100 + j * 10 + k
        NEXT k
    NEXT j
NEXT i

CheckTrue "_Byte sample", t.AByte(1, 1, 1) = -13
CheckTrue "_Unsigned _Byte sample", t.AUByte(1, 1, 1) = 203
CheckTrue "Integer sample", t.AInt(1, 1, 1) = -1111
CheckTrue "_Unsigned Integer sample", t.AUInt(1, 1, 1) = 60111
CheckTrue "Long sample", t.ALong(1, 1, 1) = -100111
CheckTrue "_Unsigned Long sample", t.AULong(1, 1, 1) = 2000000111
CheckTrue "_Integer64 sample", t.AI64(1, 1, 1) = -9000000111
CheckTrue "_Unsigned _Integer64 sample", t.AUI64(1, 1, 1) = 900000000111
CheckTrue "LBOUND dim3", LBOUND(t.AInt, 3) = 0
CheckTrue "UBOUND dim2", UBOUND(t.AInt, 2) = 1

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
