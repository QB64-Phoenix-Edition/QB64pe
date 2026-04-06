$CONSOLE:ONLY

TYPE OneArrayType
    Item(1 TO 3) AS LONG
END TYPE

ON ERROR GOTO errhandler

DIM T AS OneArrayType

IF LBOUND(T.Item) = 1 AND UBOUND(T.Item) = 3 THEN
    PRINT "PASS: ErrorTest12 nested 1D member LBOUND and UBOUND are correct."
ELSE
    PRINT "FAIL: ErrorTest12 nested 1D member bounds are wrong."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest12 unexpected runtime error"; ERR
'SLEEP
SYSTEM
