$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE BoxValue
    items(2 TO 4) AS LONG
END TYPE

PRINT MakeBox(5).items()
SYSTEM

FUNCTION MakeBox (itemValue AS LONG) AS BoxValue
    MakeBox.items(2) = itemValue
END FUNCTION
