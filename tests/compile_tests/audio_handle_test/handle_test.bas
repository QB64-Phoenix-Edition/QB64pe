Option _Explicit
$Console
$ScreenHide
_Dest _Console

' Try to open a non-existent file
Dim handle As Long

handle = _SndOpen("dummy.flac")
Print handle
If Not handle Then
    Print "This failure was expected!"
End If


handle = _SndCopy(handle)
Print handle
If Not handle Then
    Print "So was this!"
End If

System
