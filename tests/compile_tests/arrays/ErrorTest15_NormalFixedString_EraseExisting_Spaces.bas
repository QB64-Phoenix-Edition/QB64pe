$CONSOLE:ONLY
ON ERROR GOTO errhandler

DIM A(1 TO 2) AS STRING * 4

A(1) = "ABCD"
A(2) = "WXYZ"

TYPE b
    c(1 TO 2) AS STRING * 4
END TYPE

DIM D AS b
D.c(1) = "EFGH"
D.c(2) = "IJKL"

ERASE D.c()
ERASE A

IF A(1) = STRING$(4, 0) AND A(2) = STRING$(4, 0) THEN
    PRINT "PASS: ErrorTest15 ERASE reset normal fixed-length string array."
ELSE
    PRINT "FAIL: ErrorTest15 ERASE did not reset normal fixed-length string array."
END IF

IF D.c(1) = STRING$(4, 0) AND D.c(2) = STRING$(4, 0) THEN
    PRINT "PASS: ErrorTest15 ERASE reset nested fixed-length string array."
ELSE
    PRINT "FAIL: ErrorTest15 ERASE did not reset nested fixed-length string array."
END IF

'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest15 unexpected runtime error"; ERR
'SLEEP
SYSTEM

