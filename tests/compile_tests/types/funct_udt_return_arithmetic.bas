$CONSOLE:ONLY
OPTION _EXPLICIT

' PR #779: A whole UDT result cannot be used as an arithmetic operand.
TYPE ReturnProbe
    payload AS LONG
END TYPE

DIM scalarValue AS LONG
scalarValue = MakeProbe + 1
SYSTEM

FUNCTION MakeProbe AS ReturnProbe
    MakeProbe.payload = 7
END FUNCTION
