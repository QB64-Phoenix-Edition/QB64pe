$IF WINDOWS OR MACOSX THEN
    ' GLFW cannot create a window with OpenGL context in a macOS and Windows GitHub runner
    $CONSOLE:ONLY
    PRINT _TRUE
    PRINT _TRUE
    PRINT "Got past icon!"
    PRINT "Got past MouseHide!"
    PRINT "Got past MouseShow!"
    PRINT _FALSE
    PRINT "Got past ScreenHide"
    PRINT _FALSE
    PRINT "Error:"; 5
    PRINT "Error:"; 5
    PRINT "Title: foobar"
    PRINT _FALSE
    PRINT _TRUE
    PRINT "Got past ScreenShow!"
    SYSTEM
$END IF

$SCREENHIDE
$CONSOLE
_DEST _CONSOLE
ON ERROR GOTO errorhand

PRINT _DESKTOPHEIGHT > 0

PRINT _DESKTOPWIDTH > 0

_ICON
PRINT "Got past icon!"

_MOUSEHIDE
PRINT "Got past MouseHide!"

_MOUSESHOW
PRINT "Got past MouseShow!"

PRINT _SCREENEXISTS

_SCREENHIDE
PRINT "Got past ScreenHide"

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

SYSTEM

errorhand:
PRINT "Error:"; ERR
RESUME NEXT
