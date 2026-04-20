DEFLNG A-Z
OPTION _EXPLICIT
$CONSOLE:ONLY

CONST MAXLOOP = 30, MAXSIZE = 1000000
CONST WLeft = -1000, WRight = 250, WTop = 625, WBottom = -625
CONST VLeft = 110, VRight = 529, VTop = 5, VBottom = 179
CONST ColorRange = 15

DIM img AS LONG: img = _NEWIMAGE(640, 200, 8)
_DEST img

VIEW (VLeft, VTop)-(VRight, VBottom), 0, ColorRange
WINDOW (WLeft, WTop)-(WRight, WBottom)

DIM XLength AS LONG: XLength = VRight - VLeft
DIM YLength AS LONG: YLength = VBottom - VTop
DIM ColorWidth AS LONG: ColorWidth = MAXLOOP \ ColorRange

DIM AS LONG x, y
FOR y = 0 TO YLength
    DIM LogicY AS LONG: LogicY = PMAP(y, 3)
    PSET (WLeft, LogicY)
    DIM OldColor AS LONG: OldColor = 0

    FOR x = 0 TO XLength
        DIM LogicX AS LONG: LogicX = PMAP(x, 2)
        DIM MandelX AS LONG: MandelX = LogicX
        DIM MandelY AS LONG: MandelY = LogicY

        DIM i AS LONG
        FOR i = 1 TO MAXLOOP
            DIM RealNum AS LONG: RealNum = MandelX * MandelX
            DIM ImagNum AS LONG: ImagNum = MandelY * MandelY
            IF (RealNum + ImagNum) >= MAXSIZE THEN EXIT FOR
            MandelY = (MandelX * MandelY) \ 250 + LogicY
            MandelX = (RealNum - ImagNum) \ 500 + LogicX
        NEXT i

        DIM PColor AS LONG: PColor = i \ ColorWidth

        IF PColor <> OldColor THEN
            LINE -(LogicX, LogicY), (ColorRange - OldColor)
            OldColor = PColor
        END IF
    NEXT x

    LINE -(LogicX, LogicY), (ColorRange - OldColor)
NEXT y

VIEW
WINDOW

'_SAVEIMAGE "window_view_mandelbrot.png", img

_DEST _CONSOLE
AssertImage2 img, "window_view_mandelbrot.png", 0

_FREEIMAGE img
SYSTEM

'$INCLUDE:'../utilities/imageassert.bm'

