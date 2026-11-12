$CONSOLE:ONLY

' The values themselves change with every build, so check their shapes instead:
' __DATE__ is always 11 characters and __TIME__ always 8, and the version string
' always starts with the same prefix.
PRINT "vers=" + LEFT$(_COMPILERVERSION$, 9)
ShowBuildTime

SYSTEM

' From inside a SUB, to prove the call is noticed wherever it appears
SUB ShowBuildTime
    PRINT "date=" + _TOSTR$(LEN(_COMPILEDATE$))
    PRINT "time=" + _TOSTR$(LEN(_COMPILETIME$))
END SUB
