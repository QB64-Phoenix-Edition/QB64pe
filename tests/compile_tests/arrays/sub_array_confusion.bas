$CONSOLE:ONLY

'Tests the following lines are not mis-parsed as an array assignment
show (6 / 2) = 3
show2 (6 / 2) = 2, 42

SYSTEM

SUB show (a&)
    PRINT a&
END SUB


SUB show2 (a&, b&)
    PRINT a&; b&
END SUB 