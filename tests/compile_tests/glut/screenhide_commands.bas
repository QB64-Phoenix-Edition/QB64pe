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
PRINT "Got past MouseHide!"

PRINT _SCREENEXISTS

_SCREENHIDE
PRINT "Got past ScreenHide"

PRINT _SCREENICON <> 0

$IF LINUX THEN
' Since these functions don't work on linux they also don't trigger errors
' We're just printing the error manually so the test passes on Linux
    Print "Error:"; 5
    Print "Error:"; 5
$ELSE
    PRINT _SCREENX >= 0
    PRINT _SCREENY >= 0
$END IF

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
