$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT _TRUE
    SYSTEM
$END IF

$CONSOLE
_DEST _CONSOLE

PRINT _SCREENX >= 0
SYSTEM
