$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T AS OneArrayType
DIM idx AS LONG

idx = UBOUND(T.Item) + 1
T.Item(idx) = 1

PRINT "FAIL: ErrorTest06 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest06 nested 1D member UBOUND overflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest06 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
