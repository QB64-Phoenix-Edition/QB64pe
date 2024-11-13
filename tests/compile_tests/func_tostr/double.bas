$CONSOLE:ONLY

num# = _PI(10): typ$ = "DOUBLE": mi% = 1: ma% = 16

PRINT "Printing _PI(10) as "; typ$; ":"
PRINT "  default:  "; _TOSTR$(num#)
FOR x% = 0 TO 20
    PRINT USING "## digits:  "; x%;
    num$ = _TOSTR$(num#, x%)
    PRINT num$; SPC(ma% + 5 - LEN(num$));
    IF x% < mi% THEN
        PRINT "(forced to at least 1 digit)"
    ELSEIF x% > ma% THEN
        PRINT "(cropped to max. digits for "; typ$; ")"
    ELSEIF x% > 1 _ANDALSO INSTR(num$, ".") > 0 _ANDALSO LEN(num$) < x% + 1 THEN
        PRINT "(rounding eliminated digit(s))"
    ELSE
        PRINT
    END IF
NEXT x%

SYSTEM

