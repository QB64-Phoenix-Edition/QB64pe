$CONSOLE:ONLY
' The number of dimensions of an array parameter is not specified by the
' definition, rather the compiler works it out from the call sites. When arrays
' are handed down a chain of SUBs, the count can only travel one link per
' compile pass: The code is read top to bottom and we only keep track of
' dimension counts we know of, not the dependency tree between array arguments.
'
' A 3-deep chain therefore needs several passes. This check ensures the array
' does get resolved to the correct dimension count (if it doesn't then it will
' fail at runtime).
DIM two(1 TO 2, 1 TO 3)
DIM three(1 TO 2, 1 TO 3, 1 TO 4)

level3 two()
deep3 three()
SYSTEM

SUB innermost (k())
    PRINT "innermost dims:"; UBOUND(k, 1); UBOUND(k, 2)
END SUB

SUB middle (m())
    innermost m()
END SUB

SUB level3 (a())
    middle a()
END SUB

SUB deep3innermost (k3())
    PRINT "deep3 dims:"; UBOUND(k3, 1); UBOUND(k3, 2); UBOUND(k3, 3)
END SUB

SUB deep3middle (m3())
    deep3innermost m3()
END SUB

SUB deep3 (a3())
    deep3middle a3()
END SUB
