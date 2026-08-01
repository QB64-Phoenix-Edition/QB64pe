$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT "Got Past ScreenHide!"
    SYSTEM
$END IF

$CONSOLE
_DEST _CONSOLE

_SCREENHIDE
PRINT "Got Past ScreenHide!"
SYSTEM
