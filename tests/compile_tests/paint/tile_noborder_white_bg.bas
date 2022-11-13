$Console:Only
ChDir _StartDir$

TILE$ = MKL$(&HAAFFBB55)

' Paint with tiling, no border color.
' background is colored white, circle is black.
'
' Result should fill the entire circle with pattern
test1& = _NewImage(128, 50, 9)
_Dest test1&

' Make the entire image white
Line (0, 0)-(127, 49), 7, BF

Circle (64, 25), 25, 0
Paint (64, 25), TILE$

AssertImage test1&, "tile_noborder_white_bg.png"
System

'$include:'../utilities/imageassert.bm'
