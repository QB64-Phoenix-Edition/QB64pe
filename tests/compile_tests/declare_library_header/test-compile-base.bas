$CONSOLE:ONLY

' This location is relative to the source file, not QB64-PE or the location
' QB64-PE was run from.
DECLARE LIBRARY "fastmath"
   FUNCTION Fast_Sqrt&(BYVAL val AS LONG)
END DECLARE

value = Fast_Sqrt&(2000)
print value;
SYSTEM
