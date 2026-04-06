$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T AS OneArrayType

T.Item(1) = 11
T.Item(2) = 22
T.Item(3) = 33

ERASE T.Item

IF T.Item(1) = 0 AND T.Item(2) = 0 AND T.Item(3) = 0 THEN
    PRINT "PASS: ErrorTest10 ERASE reset nested 1D member array to zero."
ELSE
    PRINT "FAIL: ErrorTest10 ERASE did not reset nested 1D member array."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest10 unexpected runtime error"; ERR
'SLEEP
SYSTEM
