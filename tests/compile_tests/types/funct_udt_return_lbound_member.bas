$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE LeafValue
    items(2 TO 4) AS LONG
END TYPE

TYPE BoxValue
    leaf AS LeafValue
END TYPE

PRINT LBOUND((BoxProbe.Make.leaf.items()), 1)
SYSTEM

FUNCTION BoxProbe.Make AS BoxValue
    BoxProbe.Make.leaf.items(2) = 7
END FUNCTION
