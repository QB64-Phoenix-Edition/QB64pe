$CONSOLE:ONLY

DECLARE LIBRARY "./fastmath"
   FUNCTION Fast_Sqrt&(BYVAL val AS LONG)
END DECLARE

value = Fast_Sqrt&(2000)
print value;
SYSTEM
