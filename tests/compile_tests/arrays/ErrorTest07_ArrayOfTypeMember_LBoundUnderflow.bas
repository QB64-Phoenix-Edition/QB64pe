$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T(1 TO 2) AS OneArrayType
DIM idx AS LONG

idx = LBOUND(T(1).Item) - 1
T(1).Item(idx) = 1

PRINT "FAIL: ErrorTest07 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest07 array element member LBOUND underflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest07 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
