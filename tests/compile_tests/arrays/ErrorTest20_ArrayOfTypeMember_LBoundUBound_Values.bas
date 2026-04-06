$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T(1 TO 2) AS OneArrayType

IF LBOUND(T) = 1 AND UBOUND(T) = 2 AND LBOUND(T(2).Item) = 1 AND UBOUND(T(2).Item) = 3 THEN
    PRINT "PASS: ErrorTest20 UDT array element member bounds are correct."
ELSE
    PRINT "FAIL: ErrorTest20 UDT array element member bounds are wrong."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest20 unexpected runtime error"; ERR
'SLEEP
SYSTEM
