$CONSOLE:ONLY
OPTION _EXPLICIT


DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE IntegerFamily
    AByte( 0 To 3) AS _BYTE
    AUByte( 0 To 3) AS _UNSIGNED _BYTE
    AInt( 0 To 3) AS INTEGER
    AUInt( 0 To 3) AS _UNSIGNED INTEGER
    ALong( 0 To 3) AS LONG
    AULong( 0 To 3) AS _UNSIGNED LONG
    AI64( 0 To 3) AS _INTEGER64
    AUI64( 0 To 3) AS _UNSIGNED _INTEGER64
END TYPE

DIM t AS IntegerFamily
DIM i AS LONG
DIM j AS LONG
DIM k AS LONG

FOR i = 0 TO 3
    t.AByte(i) = -10 - i
    t.AUByte(i) = 200 + i
    t.AInt(i) = -1000 - i
    t.AUInt(i) = 60000 + i
    t.ALong(i) = -100000 - i
    t.AULong(i) = 2000000000 + i
    t.AI64(i) = -9000000000 - i
    t.AUI64(i) = 900000000000 + i
NEXT i

CheckTrue "_Byte first", t.AByte(0) = -10
CheckTrue "_Byte last", t.AByte(3) = -13
CheckTrue "_Unsigned _Byte first", t.AUByte(0) = 200
CheckTrue "_Unsigned _Byte last", t.AUByte(3) = 203
CheckTrue "Integer first", t.AInt(0) = -1000
CheckTrue "Integer last", t.AInt(3) = -1003
CheckTrue "_Unsigned Integer first", t.AUInt(0) = 60000
CheckTrue "_Unsigned Integer last", t.AUInt(3) = 60003
CheckTrue "Long first", t.ALong(0) = -100000
CheckTrue "Long last", t.ALong(3) = -100003
CheckTrue "_Unsigned Long first", t.AULong(0) = 2000000000
CheckTrue "_Unsigned Long last", t.AULong(3) = 2000000003
CheckTrue "_Integer64 first", t.AI64(0) = -9000000000
CheckTrue "_Integer64 last", t.AI64(3) = -9000000003
CheckTrue "_Unsigned _Integer64 first", t.AUI64(0) = 900000000000
CheckTrue "_Unsigned _Integer64 last", t.AUI64(3) = 900000000003
CheckTrue "LBOUND 1D", LBOUND(t.AInt) = 0
CheckTrue "UBOUND 1D", UBOUND(t.AInt) = 3

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
