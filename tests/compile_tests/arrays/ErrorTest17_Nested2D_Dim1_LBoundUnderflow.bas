$CONSOLE:ONLY

TYPE MatrixType
    Cell(2 TO 4, 7 TO 9) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM M AS MatrixType
DIM rowIdx AS LONG

rowIdx = LBOUND(M.Cell, 1) - 1
M.Cell(rowIdx, 7) = 1

PRINT "FAIL: ErrorTest17 no runtime error."
'SLEEP
SYSTEM

errhandler:
IF ERR = 9 THEN
    PRINT "PASS: ErrorTest17 nested 2D dim1 LBOUND underflow raised error 9."
ELSE
    PRINT "FAIL: ErrorTest17 expected error 9, got"; ERR
END IF
'SLEEP
SYSTEM
