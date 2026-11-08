$Console:Only
ChDir _StartDir$

' _SEAMLESS combined with start/end arcs and aspect ratios.
Const RMAX = 20
img& = _NewImage(160, 64, 32)
_Dest img&
Cls , _RGB32(0, 0, 0)

For i = 0 To RMAX
    If i Mod 2 Then c~& = _RGB32(255, 255, 0) Else c~& = _RGB32(0, 255, 0)
    ' squashed ellipse
    Circle (26, 32), i, c~&, , , 0.5, _Seamless
    ' tall ellipse
    Circle (80, 32), i, c~&, , , 2, _Seamless
    ' three-quarter arc
    Circle (134, 32), i, c~&, _Pi / 2, 2 * _Pi, , _Seamless
Next

AssertImage img&, "circle_seamless_arc_aspect.png"
System

'$include:'../utilities/imageassert.bm'
