$CONSOLE:ONLY
OPTION _EXPLICIT

DIM a AS _UNSIGNED _BYTE
DIM b AS _UNSIGNED INTEGER
DIM c AS _UNSIGNED LONG
DIM d AS _UNSIGNED _INTEGER64

a = &B11110000
b = &B1111111100000000
c = &B11111111111111110000000000000000
d = &B1111111111111111111111111111111100000000000000000000000000000000

PRINT "*** Unsigned values ***"
PRINT a; ":"; _BIN$(a)
PRINT b; ":"; _BIN$(b)
PRINT c; ":"; _BIN$(c)
PRINT d; ":"; _BIN$(d)

PRINT "Rotating left by 5"
PRINT _ROL(a, 1); ":"; _BIN$(_ROL(a, 5))
PRINT _ROL(b, 1); ":"; _BIN$(_ROL(b, 5))
PRINT _ROL(c, 1); ":"; _BIN$(_ROL(c, 5))
PRINT _ROL(d, 1); ":"; _BIN$(_ROL(d, 5))

PRINT "Rotating right by 5"
PRINT _ROR(a, 1); ":"; _BIN$(_ROR(a, 5))
PRINT _ROR(b, 1); ":"; _BIN$(_ROR(b, 5))
PRINT _ROR(c, 1); ":"; _BIN$(_ROR(c, 5))
PRINT _ROR(d, 1); ":"; _BIN$(_ROR(d, 5))

PRINT "Rotating left past size"
PRINT _ROL(a, 1); ":"; _BIN$(_ROL(a, 9))
PRINT _ROL(b, 1); ":"; _BIN$(_ROL(b, 17))
PRINT _ROL(c, 1); ":"; _BIN$(_ROL(c, 33))
PRINT _ROL(d, 1); ":"; _BIN$(_ROL(d, 65))

PRINT "Rotating right past size"
PRINT _ROR(a, 1); ":"; _BIN$(_ROR(a, 9))
PRINT _ROR(b, 1); ":"; _BIN$(_ROR(b, 17))
PRINT _ROR(c, 1); ":"; _BIN$(_ROR(c, 33))
PRINT _ROR(d, 1); ":"; _BIN$(_ROR(d, 65))

PRINT "Rotating left by -5"
PRINT _ROL(a, 1); ":"; _BIN$(_ROL(a, -5))
PRINT _ROL(b, 1); ":"; _BIN$(_ROL(b, -5))
PRINT _ROL(c, 1); ":"; _BIN$(_ROL(c, -5))
PRINT _ROL(d, 1); ":"; _BIN$(_ROL(d, -5))

PRINT "Rotating right by -5"
PRINT _ROR(a, 1); ":"; _BIN$(_ROR(a, -5))
PRINT _ROR(b, 1); ":"; _BIN$(_ROR(b, -5))
PRINT _ROR(c, 1); ":"; _BIN$(_ROR(c, -5))
PRINT _ROR(d, 1); ":"; _BIN$(_ROR(d, -5))


DIM e AS _BYTE
DIM f AS INTEGER
DIM g AS LONG
DIM h AS _INTEGER64

e = -128
f = -32768
g = -2147483648
h = -9223372036854775808

PRINT "*** Signed values ***"
PRINT e; ":"; _BIN$(e)
PRINT f; ":"; _BIN$(f)
PRINT g; ":"; _BIN$(g)
PRINT h; ":"; _BIN$(h)

PRINT "Rotating left by 5"
PRINT _ROL(e, 1); ":"; _BIN$(_ROL(e, 5))
PRINT _ROL(f, 1); ":"; _BIN$(_ROL(f, 5))
PRINT _ROL(g, 1); ":"; _BIN$(_ROL(g, 5))
PRINT _ROL(h, 1); ":"; _BIN$(_ROL(h, 5))

PRINT "Rotating right by 5"
PRINT _ROR(e, 1); ":"; _BIN$(_ROR(e, 5))
PRINT _ROR(f, 1); ":"; _BIN$(_ROR(f, 5))
PRINT _ROR(g, 1); ":"; _BIN$(_ROR(g, 5))
PRINT _ROR(h, 1); ":"; _BIN$(_ROR(h, 5))

PRINT "Rotating left past size"
PRINT _ROL(e, 1); ":"; _BIN$(_ROL(e, 9))
PRINT _ROL(f, 1); ":"; _BIN$(_ROL(f, 17))
PRINT _ROL(g, 1); ":"; _BIN$(_ROL(g, 33))
PRINT _ROL(h, 1); ":"; _BIN$(_ROL(h, 65))

PRINT "Rotating right past size"
PRINT _ROR(e, 1); ":"; _BIN$(_ROR(e, 9))
PRINT _ROR(f, 1); ":"; _BIN$(_ROR(f, 17))
PRINT _ROR(g, 1); ":"; _BIN$(_ROR(g, 33))
PRINT _ROR(h, 1); ":"; _BIN$(_ROR(h, 65))

PRINT "Rotating left by -5"
PRINT _ROL(e, 1); ":"; _BIN$(_ROL(e, -5))
PRINT _ROL(f, 1); ":"; _BIN$(_ROL(f, -5))
PRINT _ROL(g, 1); ":"; _BIN$(_ROL(g, -5))
PRINT _ROL(h, 1); ":"; _BIN$(_ROL(h, -5))

PRINT "Rotating right by -5"
PRINT _ROR(e, 1); ":"; _BIN$(_ROR(e, -5))
PRINT _ROR(f, 1); ":"; _BIN$(_ROR(f, -5))
PRINT _ROR(g, 1); ":"; _BIN$(_ROR(g, -5))
PRINT _ROR(h, 1); ":"; _BIN$(_ROR(h, -5))


PRINT "*** Rotating some numeric literals left by 5 ***"
PRINT 240~%%; ":"; _BIN$(_ROL(240~%%, 5))
PRINT 65280~%; ":"; _BIN$(_ROL(65280~%, 5))
PRINT 4294901760~&; ":"; _BIN$(_ROL(4294901760~&, 5))
PRINT 18446744069414584320~&&; ":"; _BIN$(_ROL(18446744069414584320~&&, 5))

PRINT "*** Rotating some numeric literals right by 5 ***"
PRINT 240~%%; ":"; _BIN$(_ROR(240~%%, 5))
PRINT 65280~%; ":"; _BIN$(_ROR(65280~%, 5))
PRINT 4294901760~&; ":"; _BIN$(_ROR(4294901760~&, 5))
PRINT 18446744069414584320~&&; ":"; _BIN$(_ROR(18446744069414584320~&&, 5))

SYSTEM

