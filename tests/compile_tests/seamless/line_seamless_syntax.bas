$Console:Only
ChDir _StartDir$

' _SEAMLESS shares the B/BF slot of LINE, and works with style, STEP,
' the last-point form and the default color. B and BF still behave as before.
img& = _NewImage(96, 48, 32)
_Dest img&
Cls , _RGB32(0, 0, 0)
Color _RGB32(255, 255, 255)

LINE (2, 2)-(30, 13), , _Seamless
LINE (2, 6)-(30, 17), _RGB32(255, 0, 0), _Seamless, &HF0F0
LINE -Step(-28, 11), _RGB32(0, 255, 0), _Seamless
LINE Step(4, 4)-(30, 44), _RGB32(0, 0, 255), _Seamless

LINE (36, 2)-(60, 20), _RGB32(255, 255, 0), B
LINE (36, 26)-(60, 44), _RGB32(0, 255, 255), BF
LINE (66, 2)-(92, 44), _RGB32(255, 0, 255), B, &HCCCC
LINE (66, 2)-(92, 44), _RGB32(255, 128, 0)

AssertImage img&, "line_seamless_syntax.png"
System

'$include:'../utilities/imageassert.bm'
