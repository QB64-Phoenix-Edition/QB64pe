OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE test
    a(1 TO 50) AS LONG
END TYPE

DIM Parent(1) AS test
DIM e AS _MEM, f AS _MEM, g AS _MEM

e = _MEM(Parent(0).a)
f = _MEM(Parent(1).a)
g = _MEM(Parent())

if g.size = 400 and f.size = 200 and e.size = 200 and g.elementsize = 200 and f.elementsize = 4 and e.elementsize = 4 and len(parent(0).a()) = 200 _
and len(parent(0).a()) = 200 and len (parent())= 400 and len(parent(0).a(1)) = 4 then print "LEN/MEM PASS"

'PRINT g.SIZE
'PRINT f.SIZE
'PRINT e.SIZE
'PRINT "-----------"
'PRINT g.ELEMENTSIZE
'PRINT f.ELEMENTSIZE
'PRINT e.ELEMENTSIZE
'PRINT "-----------"
'PRINT LEN(Parent(0).a())
'PRINT LEN(Parent(1).a())
'PRINT LEN(Parent())
'PRINT LEN(Parent(0).a(1))

_MEMFREE e
_MEMFREE f
_MEMFREE g
'SLEEP
SYSTEM

