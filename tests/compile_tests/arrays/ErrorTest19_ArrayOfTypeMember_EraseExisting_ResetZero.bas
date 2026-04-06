$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T(1 TO 2) AS OneArrayType

T(2).Item(1) = 7
T(2).Item(2) = 8
T(2).Item(3) = 9

ERASE T(2).Item

IF T(2).Item(1) = 0 AND T(2).Item(2) = 0 AND T(2).Item(3) = 0 THEN
    PRINT "PASS: ErrorTest19 ERASE reset member array inside UDT array element."
ELSE
    PRINT "FAIL: ErrorTest19 ERASE did not reset member array inside UDT array element."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest19 unexpected runtime error"; ERR
'SLEEP
SYSTEM
