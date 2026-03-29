$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT _TRUE
    PRINT _TRUE
    PRINT "Got Past Icon!"
    PRINT "Got Past MouseHide!"
    PRINT "Got Past MouseShow!"
    PRINT _TRUE
    PRINT _FALSE
    PRINT _TRUE
    PRINT _TRUE
    PRINT "Title: foobar"
    PRINT _TRUE
    PRINT _TRUE
    PRINT "Got past ScreenShow!"
    SYSTEM
$END IF

$CONSOLE
_DEST _CONSOLE

_SCREENHIDE
PRINT _DESKTOPHEIGHT > 0
PRINT _DESKTOPWIDTH > 0
_ICON
PRINT "Got Past Icon!"
_MOUSEHIDE
PRINT "Got Past MouseHide!"
_MOUSESHOW
PRINT "Got Past MouseShow!"
PRINT _SCREENEXISTS
PRINT _SCREENICON <> 0
PRINT _SCREENX >= 0
PRINT _SCREENY >= 0
_TITLE "foobar"
PRINT "Title: "; _TITLE$
PRINT _WINDOWHANDLE <> 0
PRINT _WINDOWHASFOCUS <= 0 ' This can be a bit random
_SCREENSHOW
PRINT "Got past ScreenShow!"
SYSTEM
