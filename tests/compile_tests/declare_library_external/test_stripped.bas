$CONSOLE:ONLY

$IF LNX THEN

' This file is missing the regular symbol table and only contains the dynamic symbol table
DECLARE LIBRARY "./libstripped"
    FUNCTION add_values&(BYVAL v1 AS LONG, BYVAL v2 as LONG)
END DECLARE

result = add_values&(2, 3)
PRINT result


$ELSE

' Windows can't use regular DECLARE LIBRARY to link against a dll, just skip this test
'
' Mac OS dylib files don't have a dynamic symbol table, so also nothing to test
PRINT 5

$END IF

SYSTEM
