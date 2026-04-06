$CONSOLE:ONLY

TYPE MatrixType
    Cell(2 TO 4, 7 TO 9) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM M AS MatrixType
DIM colIdx AS LONG

colIdx = UBOUND(M.Cell, 2) + 1
M.Cell(2, colIdx) = 1

PRINT "FAIL: ErrorTest18 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest18 nested 2D dim2 UBOUND overflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest18 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
