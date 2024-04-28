$CONSOLE:ONLY
_DEFINE A-Z AS LONG
OPTION _EXPLICIT

DIM AS LONG x

PRINT 1 OR 2
PRINT 3 AND 1
PRINT NOT 2
PRINT 2 XOR 3
PRINT 10 EQV 5
PRINT 20 IMP 50

PRINT 20 MOD 3
PRINT 2 + 3
PRINT 2 - 3
PRINT 6 / 3
PRINT 7 \ 3
PRINT 7 * 20
PRINT 3 ^ 10

x = 20
PRINT -x

PRINT 2 = 2
PRINT 2 <> 3
PRINT 2 <> 3
PRINT 2 <= 3
PRINT 2 <= 3
PRINT 2 >= 3
PRINT 2 >= 3
PRINT 2 > 3
PRINT 2 < 3

' The left side which has no parens to indicate order should still equal the
' right side which has parens to enforce order.
PRINT (2 ^ 2 * 2) = ((2 ^ 2) * 2)
PRINT (2 ^ 2 + 2) = ((2 ^ 2) + 2)
PRINT (NOT 2 + 3) = (NOT (2 + 3))
PRINT (-2 ^ 2) = (-(2 ^ 2))
PRINT (NOT 2 ^ 3) = (NOT (2 ^ 3))
PRINT (3 * 6 / 2) = ((3 * 6) / 2)
PRINT (3 * 10 \ 3) = ((3 * 10) \ 3)

' Many levels of parens
PRINT (2 ^ (3 * (4 - (2 - (10 / (20 / 2))))))

CONST foo = "foo"
CONST bar = "bar"
PRINT foo + bar

' Combos
x = 1
PRINT NOT (x = 0) AND (x <> -1)
PRINT NOT (x = 0) _ANDALSO (x <> -1)
PRINT _NEGATE (x = 0) AND (x <> -1)
PRINT _NEGATE (x = 0) _ANDALSO (x <> -1)

x = 0
PRINT (3 * 2) + (4 - 2) / (5 ^ 2) < 5 AND NOT (x = 0)
PRINT (3 * 2) + (4 - 2) / (5 ^ 2) < 5 AND _NEGATE (x = 0)

CONST A = -100
CONST B = -10
CONST D = 10
CONST E = 100

' More NOT & _NEGATE tests
x = -1
PRINT NOT A
PRINT NOT D
PRINT NOT x
x = 0
PRINT NOT x
x = 1
PRINT NOT x

x = -1
PRINT _NEGATE A
PRINT _NEGATE D
PRINT _NEGATE x
x = 0
PRINT _NEGATE x
x = 1
PRINT _NEGATE x

x = 1000
PRINT NOT x > 2000 OR E = GetValue(123456)
PRINT NOT x > 2000 AND E <> GetValue(123456)

PRINT NOT x > 2000 _ORELSE E = GetValue(123456)
PRINT NOT x > 2000 _ANDALSO E <> GetValue(123456)

x = 0
x = _NEGATE (_NEGATE (_NEGATE (_NEGATE x)))

IF _NEGATE GetValue(x) THEN
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

x = 0
x = NOT (NOT (NOT (NOT x)))

IF NOT GetValue(x) THEN
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
