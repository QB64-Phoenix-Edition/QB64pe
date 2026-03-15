OPTION _EXPLICIT

Declare Sub CheckTrue (label As String, condition As Long)
Declare Sub CheckText (label As String, actual As String, expected As String)
Declare Sub CheckNear (label As String, actual As Double, expected As Double, tolerance As Double)
Declare Sub FinishTest

DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE IntGridHolder
    G( 0 To 2 , 0 To 1) AS INTEGER
END TYPE

TYPE DoubleGridHolder
    G( 0 To 3) AS DOUBLE
END TYPE

DIM a AS IntGridHolder
DIM b AS IntGridHolder
DIM da AS DoubleGridHolder
DIM db AS DoubleGridHolder
DIM mi AS _MEM
DIM mdi AS _MEM
DIM mdb AS _MEM
DIM i AS LONG
DIM j AS LONG

FOR i = 0 TO 2
    FOR j = 0 TO 1
        a.G(i, j) = i * 10 + j + 1
        b.G(i, j) = 0
    NEXT j
NEXT i

CheckTrue "int LBOUND 1", LBOUND(a.G, 1) = 0
CheckTrue "int UBOUND 2", UBOUND(a.G, 2) = 1
CheckTrue "int _OFFSET base", _OFFSET(a.G) = _OFFSET(a.G(0, 0))

mi = _MEM(a.G)
CheckTrue "int _MEM size", mi.SIZE = 3 * 2 * 2
CheckTrue "int _MEM element size", mi.ELEMENTSIZE = 2

_MEMGET mi, mi.OFFSET, b.G
CheckTrue "int _MEMGET first", b.G(0, 0) = 1
CheckTrue "int _MEMGET last", b.G(2, 1) = 22

b.G(0, 0) = 700
b.G(2, 1) = 701
_MEMPUT mi, mi.OFFSET, b.G
CheckTrue "int _MEMPUT first", a.G(0, 0) = 700
CheckTrue "int _MEMPUT last", a.G(2, 1) = 701

_MEMFILL mi, mi.OFFSET, mi.SIZE, 0 AS INTEGER
CheckTrue "int _MEMFILL numeric first", a.G(0, 0) = 0
CheckTrue "int _MEMFILL numeric last", a.G(2, 1) = 0

FOR i = 0 TO 2
    FOR j = 0 TO 1
        b.G(i, j) = 50 + i * 10 + j
    NEXT j
NEXT i
_MEMFILL mi, mi.OFFSET, mi.SIZE, b.G
CheckTrue "int _MEMFILL array first", a.G(0, 0) = 50
CheckTrue "int _MEMFILL array last", a.G(2, 1) = 71

FOR i = 0 TO 3
    da.G(i) = i * 2.5 + .25
    db.G(i) = 0
NEXT i

mdi = _MEM(da.G)
mdb = _MEM(db.G)
CheckTrue "double _MEM size", mdi.SIZE = 4 * 8
CheckTrue "double _MEM element size", mdi.ELEMENTSIZE = 8
_MEMCOPY mdi, mdi.OFFSET, mdi.SIZE TO mdb, mdb.OFFSET
CheckNear "double _MEMCOPY first", db.G(0), .25, .0000001
CheckNear "double _MEMCOPY last", db.G(3), 7.75, .0000001

ERASE da . G
CheckNear "double ERASE first", da.G(0), 0#, .0000001
CheckNear "double ERASE last", da.G(3), 0#, .0000001

_MEMFREE mi
_MEMFREE mdi
_MEMFREE mdb

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
