$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT _TRUE
    SYSTEM
$END IF

$CONSOLE
_DEST _CONSOLE

DIM img AS LONG: img = _NEWIMAGE(32, 32, 32)

_MOUSECURSOR img
_MOUSECURSOR img, (0, 0)
_MOUSECURSOR img, (16, 16)
_MOUSESHOW

_FREEIMAGE img

PRINT _TRUE
SYSTEM
