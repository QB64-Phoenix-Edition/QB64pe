$CONSOLE:ONLY
_DEFINE A-Z AS LONG
OPTION _EXPLICIT

CONST A = -100
CONST B = -10
CONST D = 10
CONST E = 100

DIM c AS LONG

PRINT "Logical op test:"
PRINT

c = _NEGATE (_NEGATE (_NEGATE (_NEGATE c)))

IF _NEGATE GetValue(c) THEN
    PRINT "_NEGATE: Test passed."
ELSE
    PRINT "_NEGATE: Test failed."
END IF

IF GetValue(D) < 0 _ANDALSO GetValue(E) < 0 THEN
    PRINT "_ANDALSO: Test failed."
ELSE
    PRINT "_ANDALSO: Test passed."
END IF

IF GetValue(A) < 0 _ORELSE GetValue(B) < 0 THEN
    PRINT "_ORELSE: Test passed."
ELSE
    PRINT "_ORELSE: Test failed."
END IF


PRINT
PRINT "Bitwise op test:"
PRINT

c = NOT (NOT (NOT (NOT c)))

IF NOT GetValue(c) THEN
    PRINT "NOT: Test passed."
ELSE
    PRINT "NOT: Test failed."
END IF

IF GetValue(D) < 0 AND GetValue(E) < 0 THEN
    PRINT "AND: Test failed."
ELSE
    PRINT "AND: Test passed."
END IF

IF GetValue(A) < 0 OR GetValue(B) < 0 THEN
    PRINT "OR: Test passed."
ELSE
    PRINT "OR: Test failed."
END IF

SYSTEM

FUNCTION GetValue& (x AS LONG)
    PRINT "Function called for value:"; x
    GetValue = x
END FUNCTION
