$CONSOLE:ONLY

TYPE MatrixType
    Cell(2 TO 4, 7 TO 9) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM M AS MatrixType

IF LBOUND(M.Cell, 1) = 2 AND UBOUND(M.Cell, 1) = 4 AND LBOUND(M.Cell, 2) = 7 AND UBOUND(M.Cell, 2) = 9 THEN
    PRINT "PASS: ErrorTest13 nested 2D member bounds are correct."
ELSE
    PRINT "FAIL: ErrorTest13 nested 2D member bounds are wrong."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest13 unexpected runtime error"; ERR
'SLEEP
SYSTEM
