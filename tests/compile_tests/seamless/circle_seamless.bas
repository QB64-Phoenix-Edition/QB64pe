$Console:Only
ChDir _StartDir$

' Concentric circles one pixel apart, alternating colors.
' With _SEAMLESS they must cover the whole disc: no background pixel may be left inside.
Const RMAX = 28
img& = _NewImage(64, 64, 32)
_Dest img&
Cls , _RGB32(0, 0, 0)

For i = 0 To RMAX
    If i Mod 2 Then c~& = _RGB32(255, 0, 0) Else c~& = _RGB32(0, 0, 255)
    Circle (32, 32), i, c~&, , , , _Seamless
Next

_Source img&
holes = 0
For y = 0 To 63
    For x = 0 To 63
        If (x - 32) ^ 2 + (y - 32) ^ 2 < (RMAX + 0.5) ^ 2 Then
            If Point(x, y) = _RGB32(0, 0, 0) Then holes = holes + 1
        End If
    Next
Next

_Dest _Console
Print "Holes:"; holes

AssertImage img&, "circle_seamless.png"
System

'$include:'../utilities/imageassert.bm'
