$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T AS OneArrayType
DIM idx AS LONG

idx = LBOUND(T.Item) - 1
T.Item(idx) = 1

PRINT "FAIL: ErrorTest05 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest05 nested 1D member LBOUND underflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest05 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
