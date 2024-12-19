$CONSOLE:ONLY

_DEFINE A-Z AS LONG
OPTION _EXPLICIT

CONST FILE_NAME = "mem_checking_test.bmp"

CHDIR _STARTDIR$

DIM img AS LONG: img = _NEWIMAGE(1280, 720, 32)
_DEST img
_SOURCE img
CLS , _RGB32(255, 255, 0)

DIM i AS LONG
FOR i = 1 TO 20
    LINE (RND * 1280, RND * 720)-(RND * 1280, RND * 720), _RGB32(RND * 256, RND * 256, RND * 256), B
NEXT

Save32 0, 0, 1279, 719, 0, FILE_NAME

_DEST _CONSOLE
_SOURCE _CONSOLE

PRINT "Saved "; FILE_NAME

KILL FILE_NAME

SYSTEM

SUB Save32 (x1%, y1%, x2%, y2%, image&, Filename$)
    'Super special STEVE-Approved BMP Export routine for use with 32-bit color images.

    TYPE BMPFormat ' Description                          Bytes    QB64 Function
        ID AS STRING * 2 ' File ID("BM" text or 19778 AS Integer) 2      CVI("BM")
        Size AS LONG ' Total Size of the file                4      LOF
        Blank AS LONG ' Reserved                              4
        Offset AS LONG ' Start offset of image pixel data      4      (add one for GET)
        Hsize AS LONG ' Info header size (always 40)          4
        PWidth AS LONG ' Image width                            4      _WIDTH(handle&)
        PDepth AS LONG ' Image height (doubled in icons)        4      _HEIGHT(handle&)
        Planes AS INTEGER ' Number of planes (normally 1)          2
        BPP AS INTEGER ' Bits per pixel(palette 1, 4, 8, 24)    2      _PIXELSIZE(handle&)
        Compression AS LONG ' Compression type(normally 0)          4
        ImageBytes AS LONG ' (Width + padder) * Height              4
        Xres AS LONG ' Width in PELS per metre(normally 0)    4
        Yres AS LONG ' Depth in PELS per metre(normally 0)    4
        NumColors AS LONG ' Number of Colors(normally 0)          4      2 ^ BPP
        SigColors AS LONG ' Significant Colors(normally 0)        4
    END TYPE '                Total Header bytes =  54

    DIM BMP AS BMPFormat
    DIM x AS LONG, y AS LONG
    DIM temp AS STRING * 3
    DIM m AS _MEM, n AS _MEM
    DIM o AS _OFFSET
    m = _MEMIMAGE(image&)

    IF x1% > x2% THEN SWAP x1%, x2%
    IF y1% > y2% THEN SWAP y1%, y2%
    _SOURCE image&
    DIM pixelbytes&: pixelbytes& = 4
    DIM OffsetBITS&: OffsetBITS& = 54 'no palette in 24/32 bit
    DIM BPP%: BPP% = 24
    DIM NumColors&: NumColors& = 0 '24/32 bit say zero
    BMP.PWidth = (x2% - x1%) + 1
    BMP.PDepth = (y2% - y1%) + 1

    DIM ImageSize&: ImageSize& = BMP.PWidth * BMP.PDepth

    BMP.ID = "BM"
    BMP.Size = ImageSize& * 3 + 54
    BMP.Blank = 0
    BMP.Offset = 54
    BMP.Hsize = 40
    BMP.Planes = 1
    BMP.BPP = 24
    BMP.Compression = 0
    BMP.ImageBytes = ImageSize&
    BMP.Xres = 3780
    BMP.Yres = 3780
    BMP.NumColors = 0
    BMP.SigColors = 0

    DIM Compression&: Compression& = 0
    DIM WidthPELS&: WidthPELS& = 3780
    DIM DepthPELS&: DepthPELS& = 3780
    DIM SigColors&: SigColors& = 0
    DIM f: f = FREEFILE
    n = _MEMNEW(BMP.Size)
    _MEMPUT n, n.OFFSET, BMP
    o = n.OFFSET + 54

    $CHECKING:OFF
    y = y2% + 1
    DIM w&: w& = _WIDTH(image&)
    DO
        y = y - 1: x = x1% - 1
        DO
            x = x + 1
            _MEMGET m, m.OFFSET + (w& * y + x) * 4, temp
            _MEMPUT n, o, temp
            o = o + 3
        LOOP UNTIL x = x2%
    LOOP UNTIL y = y1%
    $CHECKING:ON

    _MEMFREE m
    OPEN Filename$ FOR BINARY AS #f
    DIM t$: t$ = SPACE$(BMP.Size)
    _MEMGET n, n.OFFSET, t$
    PUT #f, , t$
    _MEMFREE n
    CLOSE #f
END SUB
