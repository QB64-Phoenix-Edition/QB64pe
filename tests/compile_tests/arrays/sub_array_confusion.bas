$CONSOLE:ONLY

'Tests the following lines are not mis-parsed as an array assignment
show (6 / 2) = 3
show2 (6 / 2) = 2, 42

'Implicit array
a(6 / 2) = 5
PRINT a(3)

'Explicit array
DIM b(4)
b(6 / 2) = 7
PRINT b(3)

SYSTEM

SUB show (a&)
    PRINT a&
END SUB


SUB show2 (a&, b&)
    PRINT a&; b&
END SUB 