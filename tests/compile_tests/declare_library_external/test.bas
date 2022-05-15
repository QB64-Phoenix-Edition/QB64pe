$CONSOLE:ONLY

$IF WIN THEN

' Windows can't use regular DECLARE LIBRARY to link against a dll, just skip this test
PRINT 5

$ELSE

DECLARE LIBRARY "./lib"
    FUNCTION add_values&(BYVAL v1 AS LONG, BYVAL v2 as LONG)
END DECLARE

result = add_values&(2, 3)
PRINT result

$END IF

SYSTEM
