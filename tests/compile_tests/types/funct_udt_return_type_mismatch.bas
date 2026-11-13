$CONSOLE:ONLY
OPTION _EXPLICIT

' PR #779: A whole UDT result cannot be assigned to a numeric scalar.
TYPE ReturnProbe
    payload AS LONG
END TYPE

DIM scalarValue AS LONG
scalarValue = MakeProbe
SYSTEM

FUNCTION MakeProbe AS ReturnProbe
    MakeProbe.payload = 7
END FUNCTION
