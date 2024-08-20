$CONSOLE:ONLY

'interfacing internal variables
DECLARE LIBRARY "errtest"
    FUNCTION GetErrorHistory$
    FUNCTION GetErrorGotoLine&
END DECLARE

'testing handler behavior
Status
ON ERROR GOTO mainHandler
Status
ERROR 5
NormalTest
ERROR 5
RecursiveTest 3
ERROR 5
ON ERROR GOTO _LASTHANDLER
Status

'return to test script
SYSTEM

'-----------------------------------------------------------------------
mainHandler:
PRINT " CALLED: main handler"
RESUME NEXT
'-----
nSubHandler:
PRINT " CALLED: normal sub handler"
RESUME NEXT
'-----
rSubHandler:
PRINT " CALLED: recursive sub handler"
RESUME NEXT

'-----------------------------------------------------------------------
SUB NormalTest
ON ERROR GOTO nSubHandler
Status
ERROR 5
ON ERROR GOTO _LASTHANDLER
Status
END SUB
'-----
SUB RecursiveTest (n%)
ON ERROR GOTO rSubHandler
Status
ERROR 5
IF n% > 1 THEN RecursiveTest n% - 1
ON ERROR GOTO _LASTHANDLER
Status
END SUB

'-----------------------------------------------------------------------
SUB Status
PRINT: PRINT "HISTORY: "; "<"; GetErrorHistory$; ">"
SELECT CASE GetErrorGotoLine&
    CASE 0: PRINT " ACTIVE: unhandled"
    CASE 1: PRINT " ACTIVE: main handler"
    CASE 2: PRINT " ACTIVE: normal sub handler"
    CASE 3: PRINT " ACTIVE: recursive sub handler"
END SELECT
END SUB

