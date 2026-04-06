$CONSOLE:ONLY
ON ERROR GOTO errhandler

DIM A(1 TO 3) AS LONG
DIM idx AS LONG

idx = UBOUND(A) + 1
A(idx) = 123

PRINT "FAIL: ErrorTest02 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest02 normal 1D UBOUND overflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest02 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
