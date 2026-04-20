OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE test
    a(1 to 50) AS LONG
END TYPE
DIM Parent(1) AS test

DIM e AS _MEM, f AS _MEM, g AS _MEM

e = _MEM(Parent(0).a)
f = _MEM(Parent(1).a)
g = _MEM(Parent())

PRINT g.SIZE
PRINT f.SIZE
PRINT e.SIZE

_MEMFREE e
_MEMFREE f
_MEMFREE g

SYSTEM
