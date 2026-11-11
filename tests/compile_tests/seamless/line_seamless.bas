$Console:Only
ChDir _StartDir$

' A radar-like sweep of lines from the centre.
' Plain LINE leaves pinholes between neighbouring lines; with _SEAMLESS
' no background pixel may be left inside the disc.
Const RMAX = 28
img& = _NewImage(64, 64, 32)
_Dest img&
Cls , _RGB32(0, 0, 0)

steps = Int(2 * _Pi * RMAX * 1.5)
For i = 0 To steps - 1
    a = i * 2 * _Pi / steps
    t = i / steps
    LINE (32, 32)-(32 + RMAX * Cos(a), 32 + RMAX * Sin(a)), _RGB32(255, 255 * t, 255 - 255 * t), _Seamless
Next

_Source img&
holes = 0
For y = 0 To 63
    For x = 0 To 63
        If (x - 32) ^ 2 + (y - 32) ^ 2 < (RMAX - 1) ^ 2 Then
            If Point(x, y) = _RGB32(0, 0, 0) Then holes = holes + 1
        End If
    Next
Next

_Dest _Console
Print "Holes:"; holes

AssertImage img&, "line_seamless.png"
System

'$include:'../utilities/imageassert.bm'
