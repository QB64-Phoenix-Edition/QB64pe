$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE ItemType
    Values(0 To 3) AS _UNSIGNED INTEGER
END TYPE

REDIM reference(0 TO 3) AS _UNSIGNED INTEGER
REDIM item(0) AS ItemType
DIM idx AS LONG

reference(0) = 1
reference(1) = 2
reference(2) = 40000
reference(3) = 65535
item(0).Values(0) = 1
item(0).Values(1) = 2
item(0).Values(2) = 40000
item(0).Values(3) = 65535

REDIM reference(0 TO 3) AS _UNSIGNED INTEGER
REDIM item(0).Values(0 To 3)

FOR idx = 0 TO 3
    IF item(0).Values(idx) <> reference(idx) THEN
        PRINT "FAIL 04_redim_uinteger.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        SYSTEM
    END IF
NEXT idx

PRINT "PASS 04_redim_uinteger.bas"

SYSTEM
