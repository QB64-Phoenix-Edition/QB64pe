$CONSOLE:ONLY
ON ERROR GOTO errhandler

DIM A(1 TO 2, 5 TO 6) AS LONG
DIM colIdx AS LONG

colIdx = UBOUND(A, 2) + 1
A(1, colIdx) = 1

PRINT "FAIL: ErrorTest04 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest04 normal 2D dim2 UBOUND overflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest04 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
