$CONSOLE
_Dest _Console

_ScreenHide
Print _DesktopHeight > 0
Print _DesktopWidth > 0
_Icon
Print "Got Past Icon!"
_MouseHide
Print "Got Past MouseHide!"
_MouseShow
Print "Got Past MouseShow!"
Print _ScreenExists
Print _ScreenIcon <> 0
Print _ScreenX >= 0
Print _ScreenY >= 0
_Title "foobar"
Print "Title: "; _Title$

$IF WIN THEN
Print _WindowHandle <> 0
$ELSE
Print _WindowHandle = 0
$END IF

Print _WindowHasFocus <= 0 ' This can be a bit random

_ScreenShow
Print "Got past ScreenShow!"
System
