'Test to prove vWatch availability and the pre-compiler state flag.

$DEBUG
$CONSOLE:ONLY

$IF _DEBUG_ THEN
    PRINT "Compiling with $DEBUG features enabled."
$ELSE
    PRINT "Compiling without $DEBUG features."
$END IF

SYSTEM

