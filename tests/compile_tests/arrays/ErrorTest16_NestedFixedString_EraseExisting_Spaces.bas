$CONSOLE:ONLY

TYPE FixedTextType
    Txt(1 TO 2) AS STRING * 4
END TYPE

ON ERROR GOTO errhandler

DIM T AS FixedTextType

T.Txt(1) = "ABCD"
T.Txt(2) = "WXYZ"

ERASE T.Txt

IF T.Txt(1) = STRING$(4, 0) AND T.Txt(2) = STRING$(4, 0) THEN
    PRINT "PASS: ErrorTest16 ERASE reset nested fixed-length string member array."
ELSE
    PRINT "FAIL: ErrorTest16 ERASE did not reset nested fixed-length string member array."
END IF
'SLEEP
SYSTEM

errhandler:
PRINT "FAIL: ErrorTest16 unexpected runtime error"; ERR
'SLEEP
SYSTEM
