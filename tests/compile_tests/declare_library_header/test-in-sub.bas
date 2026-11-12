$CONSOLE:ONLY

' The library routine is called from inside a SUB/FUNCTION rather than at module
' level. fastmath.h defines Fast_Sqrt rather than declaring it, so the header can
' only be included once - which means CallIt and CallItToo have to be compiled
' into the same object file as the module-level code.

DECLARE LIBRARY "./fastmath"
   FUNCTION Fast_Sqrt&(BYVAL val AS LONG)
END DECLARE

PRINT CallIt&(2000);
CallItToo 3000
SYSTEM

FUNCTION CallIt& (v AS LONG)
    CallIt& = Fast_Sqrt&(v)
END FUNCTION

SUB CallItToo (v AS LONG)
    PRINT Fast_Sqrt&(v);
END SUB
