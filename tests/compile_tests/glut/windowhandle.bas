$CONSOLE
_Dest _Console

' _WindowHandle only returns an actual handle on Windows
$IF WIN THEN
Print _WindowHandle <> 0
$ELSE
Print _WindowHandle = 0
$END IF

System
