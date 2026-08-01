$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT "Title: "
    SYSTEM
$END IF

$CONSOLE
_DEST _CONSOLE

PRINT "Title: "; _TITLE$
SYSTEM
