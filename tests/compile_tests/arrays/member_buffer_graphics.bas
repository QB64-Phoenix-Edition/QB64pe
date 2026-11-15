$Console
$ScreenHide
$Unstable:TypeFields
Option _Explicit

Type PixelBuffers
    nativePixels(2 To 13) As Long
    dynPixels(0 To 0) _Dynamic As Long
End Type

Dim buffers As PixelBuffers
Dim imageHandle As Long
Dim expectedColor As _Unsigned Long
ReDim buffers.dynPixels(-3 To 8) As Long
imageHandle = _NewImage(16, 16, 32)
_Dest imageHandle
_Source imageHandle
expectedColor = _RGB32(19, 93, 173)
PSet (2, 2), expectedColor
Get (2, 2)-(2, 2), buffers.dynPixels()
Put (3, 3), buffers.dynPixels(), PSet
If Point(3, 3) <> expectedColor Then System 1
Get (2, 2)-(2, 2), buffers.nativePixels()
Put (4, 4), buffers.nativePixels(), PSet
If Point(4, 4) <> expectedColor Then System 2
_Dest 0
_Source 0
_FreeImage imageHandle
_Dest _Console
Print "PASS"
System 0
