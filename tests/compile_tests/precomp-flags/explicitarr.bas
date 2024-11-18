'--- this will set _ASSERTS_ = 1
'$ASSERTS
'--- this will set _ASSERTS_ = 1 and _CONSOLE_ = 1
'$ASSERTS:CONSOLE
'--- this will set _CONSOLE_ = 1
$CONSOLE
'--- this will set/override to _CONSOLE_ = 2
'$CONSOLE:ONLY
'--- this will set _DEBUG_ = 1 and _SOCKETS_ = 1 (as $DEBUG contains _OPENHOST/CLIENT)
'$DEBUG
'--- this will set _SOCKETS_ = 1
'PRINT _OPENCLIENT("http:")
'--- this will set _EXPLICIT_ = 1 and _EXPLICITARRAY_ = 1
'OPTION _EXPLICIT
'--- this will set _EXPLICITARRAY_ = 1
OPTION _EXPLICITARRAY

'======================================
$IF _CONSOLE_ = 1 THEN
    _DEST _CONSOLE
$END IF
'======================================
$IF _EXPLICIT_ THEN
    PRINT "OPTION _EXPLICIT active. (Implies _EXPLICITARRAY)"
$ELSE
    PRINT "OPTION _EXPLICIT inactive."
$END IF
'======================================
$IF _EXPLICITARRAY_ THEN
    PRINT "OPTION _EXPLICITARRAY active."
$ELSE
    PRINT "OPTION _EXPLICITARRAY inactive."
$END IF
'======================================
$IF _ASSERTS_ THEN
    PRINT "$ASSERTS active."
$ELSE
    PRINT "$ASSERTS inactive."
$END IF
'======================================
$IF _CONSOLE_ THEN
    _DEST _CONSOLE
    PRINT "$CONSOLE active ";
    $IF _CONSOLE_ = 1 THEN
        PRINT "(w/o ONLY option)."
    $ELSEIF _CONSOLE_ = 2 THEN
        PRINT "(with ONLY option)."
    $END IF
$ELSE
    PRINT "$CONSOLE inactive."
$END IF
'======================================
$IF _DEBUG_ THEN
    PRINT "$DEBUG active."
$ELSE
    PRINT "$DEBUG inactive."
$END IF
'======================================
$IF _SOCKETS_ THEN
    PRINT "DEPENDENCY_SOCKETS active."
$ELSE
    PRINT "DEPENDENCY_SOCKETS inactive."
$END IF
'======================================

SYSTEM

