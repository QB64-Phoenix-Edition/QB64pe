$CONSOLE:ONLY

' Fast_Sqrt is declared with no library name, so QB64-PE emits neither an include
' nor a prototype for it - the declaration has to come from somewhere else. Here
' that somewhere is a header pulled in by a *later* DECLARE LIBRARY block, which
' is why a nameless DECLARE LIBRARY is treated as needing the main object file
' rather than trying to work out where its declaration actually comes from.

DECLARE LIBRARY
    FUNCTION Fast_Sqrt&(BYVAL val AS LONG)
END DECLARE

DECLARE LIBRARY "./fastmath"
END DECLARE

PRINT FromASub&(500);
SYSTEM

FUNCTION FromASub& (v AS LONG)
    FromASub& = Fast_Sqrt&(v)
END FUNCTION
