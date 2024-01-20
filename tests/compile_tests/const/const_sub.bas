$CONSOLE:ONLY
CONST glob = 60

foo
baz

SYSTEM

' SUB/FUNCTIONs should be able to define their on CONST that are local to that SUB/FUNCTION
' They should also be able to access the global CONSTs
SUB foo()
    CONST a = 20
    CONST bar = a + 20 + glob

    PRINT bar
END SUB

' Separate SUB/FUNCTIONs should be able to define CONST values with the same names
SUB baz()
    CONST a = 40
    CONST bar = a + 20 + glob

    PRINT bar
END SUB
