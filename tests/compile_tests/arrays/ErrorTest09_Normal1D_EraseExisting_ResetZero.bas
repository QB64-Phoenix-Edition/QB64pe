$CONSOLE:ONLY
ON ERROR GOTO errhandler

DIM A(1 TO 3) AS LONG

A(1) = 11
A(2) = 22
A(3) = 33

ERASE A

IF A(1) = 0 AND A(2) = 0 AND A(3) = 0 THEN
    PRINT "PASS: ErrorTest09 ERASE reset normal 1D array to zero."
ELSE
    PRINT "FAIL: ErrorTest09 ERASE did not reset normal 1D array."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest09 unexpected runtime error"; ERR
'SLEEP
SYSTEM
