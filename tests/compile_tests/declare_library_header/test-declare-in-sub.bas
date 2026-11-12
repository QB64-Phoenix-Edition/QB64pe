$CONSOLE:ONLY

' The DECLARE LIBRARY block itself lives inside the SUB that uses it. The header
' include still lands at module scope, so the SUB has to be compiled alongside it.

PRINT Doubled&(1234);
SYSTEM

FUNCTION Doubled& (v AS LONG)
    DECLARE LIBRARY "./fastmath"
        FUNCTION Fast_Sqrt&(BYVAL val AS LONG)
    END DECLARE
    Doubled& = Fast_Sqrt&(v)
END FUNCTION
