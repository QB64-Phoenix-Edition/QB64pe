$CONSOLE:ONLY
ON ERROR GOTO errhandler

DIM A(1 TO 2, 5 TO 6) AS LONG
DIM rowIdx AS LONG

rowIdx = LBOUND(A, 1) - 1
A(rowIdx, 5) = 1

PRINT "FAIL: ErrorTest03 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest03 normal 2D dim1 LBOUND underflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest03 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
