$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT "Got past Title!"
    SYSTEM
$END IF

$CONSOLE
_DEST _CONSOLE

_TITLE "foobar"
PRINT "Got past Title!"
SYSTEM
