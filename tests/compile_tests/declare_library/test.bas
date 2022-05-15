$CONSOLE:ONLY

DECLARE LIBRARY
    FUNCTION isdigit&(BYVAL c AS LONG)
END DECLARE

value = isdigit&(ASC("2"))
print value;
SYSTEM
