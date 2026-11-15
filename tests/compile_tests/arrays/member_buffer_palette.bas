$Console
$ScreenHide
$Unstable:TypeFields
Option _Explicit

Type PaletteBuffers
    inlineColors(2 To 17) _Static As Long
    liveColors(0 To 0) _Dynamic As Long
End Type

Dim colors(2 To 2) As PaletteBuffers
Dim topColors(0 To 15) As Long
Dim imageHandle As Long, colorIndex As Long
Dim baseColor As _Unsigned Long, inlineColor As _Unsigned Long, liveColor As _Unsigned Long
ReDim colors(2).liveColors(-7 To 8) As Long
For colorIndex = 0 To 15
    topColors(colorIndex) = -1
    colors(2).inlineColors(colorIndex + 2) = -1
    colors(2).liveColors(colorIndex - 7) = -1
Next
topColors(1) = 4
colors(2).inlineColors(3) = 4
colors(2).liveColors(-6) = 2

imageHandle = _NewImage(640, 480, 12)
_Dest imageHandle
Palette Using topColors
baseColor = _PaletteColor(1, imageHandle)
Palette Using colors(2).inlineColors()
inlineColor = _PaletteColor(1, imageHandle)
Palette Using colors(2).liveColors()
liveColor = _PaletteColor(1, imageHandle)
If inlineColor <> baseColor Or inlineColor = liveColor Then System 1
Palette Using colors(2).inlineColors
If _PaletteColor(1, imageHandle) <> inlineColor Then System 2
Palette Using colors(2).liveColors&
If _PaletteColor(1, imageHandle) <> liveColor Then System 3
Palette Using topColors(0)
If _PaletteColor(1, imageHandle) <> baseColor Then System 4
_Dest 0
_FreeImage imageHandle
_Dest _Console
Print "PASS"
System 0
