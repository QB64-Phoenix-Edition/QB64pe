$Console:Only
ChDir _StartDir$

TILE$ = MKL$(&HAAFFBB55)

' Paint with tiling, white border color.
' background is black, circle is white.
'
' Result should fill the entire circle with pattern
test1& = _NewImage(128, 50, 9)
_Dest test1&

Circle (64, 25), 25, 7
Paint (64, 25), TILE$, 7

AssertImage test1&, "tile_border_black_bg.bmp"
System

'$include:'../utilities/imageassert.bm'
