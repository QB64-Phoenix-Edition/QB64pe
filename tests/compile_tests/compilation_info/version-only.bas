$CONSOLE:ONLY

' Only one of the three compilation-info functions is used here. The other two
' must not be emitted, and this must still link.
PRINT "vers=" + LEFT$(_COMPILERVERSION$, 9)

SYSTEM
