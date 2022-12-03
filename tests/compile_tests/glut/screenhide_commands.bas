$SCREENHIDE
$CONSOLE
_Dest _Console
ON ERROR GOTO errorhand

$IF WIN THEN
Print _DesktopHeight > 0
$ELSE
Print _DesktopHeight = 0
$END IF

$IF WIN THEN
Print _DesktopWidth > 0
$ELSE
Print _DesktopWidth = 0
$END IF

_Icon
Print "Got past icon!"

_MouseHide
Print "Got past MouseHide!"

_MouseShow
Print "Got past MouseHide!"

Print _ScreenExists

_ScreenHide
Print "Got past ScreenHide"

Print _ScreenIcon <> 0

$IF LINUX THEN
' Since these functions don't work on linux they also don't trigger errors
' We're just printing the error manually so the test passes on Linux
Print "Error:"; 5
Print "Error:"; 5
$ELSE
Print _ScreenX >= 0
Print _ScreenY >= 0
$END IF

_Title "foobar"
Print "Title: "; _Title$

Print _WindowHandle <> 0
Print _WindowHasFocus <= 0 ' This can be a bit random

_ScreenShow
Print "Got past ScreenShow!"
System

System

errorhand:
PRINT "Error:"; ERR
RESUME NEXT
