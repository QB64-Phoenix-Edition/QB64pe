$Console:Only
ChDir _StartDir$

TILE$ = MKL$(&HAAFFBB55)

' Paint with tiling, black border color.
' background is colored white, circle is black.
'
' Note the starting location for Paint is neither
' white or black, the starting color should not
' matter when using a border color.
'
' Result should fill the entire circle with pattern
test1& = _NewImage(128, 50, 9)
_Dest test1&

' Make the entire image white
Line (0, 0)-(127, 49), 7, BF

Circle (64, 25), 25, 0
Pset (64, 25), 15 ' Set the starting location a third color

Paint (64, 25), TILE$, 0

AssertImage test1&, "tile_border_white_bg.bmp"
System

'$include:'../utilities/imageassert.bm'
