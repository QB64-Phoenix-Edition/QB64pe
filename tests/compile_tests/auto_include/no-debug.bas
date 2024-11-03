'Test to prove vWatch availability and the pre-compiler state flag.

$CONSOLE:ONLY

$IF DEBUG_IS_ACTIVE THEN
    PRINT "Compiling with $DEBUG features enabled."
$ELSE
    PRINT "Compiling without $DEBUG features."
$END IF

SYSTEM

