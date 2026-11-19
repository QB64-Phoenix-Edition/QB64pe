$CONSOLE:ONLY

' _COTH(x) is the hyperbolic cotangent, cosh(x) / sinh(x).
' It is odd, near 1 in size for large arguments, and undefined at 0.
DIM x AS DOUBLE

ON ERROR GOTO handler

PRINT "0.2: "; Near(_COTH(.2), 5.066489563439473)
PRINT "1: "; Near(_COTH(1), 1.3130352854993315)
PRINT "-1: "; Near(_COTH(-1), -1.3130352854993315)
PRINT "0.5: "; Near(_COTH(.5), 2.163953413738653)

' Arguments above 44.014845 raised error 5, where coth is 1 to double precision.
x = 50: PRINT "50: "; _COTH(x)
x = 800: PRINT "800: "; _COTH(x)
x = -800: PRINT "-800: "; _COTH(x)

' The pole at 0 is error 5, as _CSCH(0) raises it.
x = 0: PRINT "0: "; _COTH(x)

SYSTEM

handler:
PRINT "error"; ERR
RESUME NEXT

FUNCTION Near& (got AS DOUBLE, expected AS DOUBLE)
    Near& = ABS(got - expected) <= ABS(expected) * 1E-12
END FUNCTION
