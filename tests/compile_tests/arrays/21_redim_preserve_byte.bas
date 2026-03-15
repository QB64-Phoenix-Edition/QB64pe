' 21_redim_preserve_byte.bas
OPTION _EXPLICIT

TYPE ItemType
    Values(0 To 3) AS _BYTE
END TYPE

REDIM reference(0 TO 3) AS _BYTE
REDIM item(0) AS ItemType
DIM idx AS LONG

reference(0) = -5
reference(1) = 7
reference(2) = 12
reference(3) = 100
item(0).Values(0) = -5
item(0).Values(1) = 7
item(0).Values(2) = 12
item(0).Values(3) = 100

REDIM _PRESERVE reference(0 TO 3) AS _BYTE
REDIM _PRESERVE item(0).Values(0 To 3)

FOR idx = 0 TO 3
    IF item(0).Values(idx) <> reference(idx) THEN
        PRINT "FAIL 21_redim_preserve_byte.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        SYSTEM
    END IF
NEXT idx

PRINT "PASS 21_redim_preserve_byte.bas"

SYSTEM
