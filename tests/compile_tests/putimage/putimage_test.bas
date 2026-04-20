OPTION _EXPLICIT
$CONSOLE:ONLY

CHDIR _STARTDIR$

TYPE TestStats
    total AS INTEGER
    failed AS INTEGER
END TYPE

DIM SHARED stats AS TestStats
DIM SHARED errCaught AS _BYTE

DIM SHARED srcText AS LONG: srcText = _NEWIMAGE(20, 10, 0)
PrepareTextSource srcText

TestTextToTextBasic
TestTextToTextReversed
TestTextToTextMirrored
TestTextToTextFlipped
TestTextToTextClipping

TestTextToGraphics32Basic
TestTextToGraphics32ViewClip
TestTextToGraphics32WindowNoScale
TestTextToGraphics32Reversed
TestTextToGraphics32Mirrored
TestTextToGraphics32Flipped
TestTextToGraphics32Clipping
TestGraphicsToGraphics32ViewClip
TestGraphicsToGraphics32WindowNoScale
TestGraphicsToGraphics32SourceViewClip

TestTextToGraphics8Basic
TestTextToGraphics8Mirrored
TestTextToGraphics8Flipped
TestTextToGraphics4Basic
TestTextToGraphics4Mirrored
TestTextToGraphics4Flipped

TestTextToTextViewPrint
TestTextToTextScaleIllegal
TestTextToGraphicsScaleIllegal

TestTextToGraphics2Illegal
TestGraphicsToTextIllegal

TestTextToGraphics32TTFFont

_FREEIMAGE srcText

IF stats.failed = 0 THEN
    PRINT "ALL TESTS PASSED"; stats.total
ELSE
    PRINT "TESTS FAILED"; stats.failed; "of"; stats.total
END IF

SYSTEM

err_text_to_gfx2:
errCaught = _TRUE
RESUME NEXT

err_gfx_to_text:
errCaught = _TRUE
RESUME NEXT

err_text_to_text_scale:
errCaught = _TRUE
RESUME NEXT

err_text_to_gfx_scale:
errCaught = _TRUE
RESUME NEXT

SUB ReportCheck (testName AS STRING, condition AS INTEGER)
    _DEST _CONSOLE
    stats.total = stats.total + 1
    IF condition THEN
        PRINT "PASS: "; testName
    ELSE
        stats.failed = stats.failed + 1
        PRINT "FAIL: "; testName
    END IF
END SUB

SUB PrepareTextSource (img AS LONG)
    DIM oldDest AS LONG: oldDest = _DEST

    _DEST img
    CLS

    COLOR 15, 1
    LOCATE 1, 1: PRINT "ABCDEFGHIJKLMNOPQRST";
    COLOR 14, 4
    LOCATE 2, 1: PRINT "abcdefghijklmnopqrst";
    COLOR 10, 2
    LOCATE 3, 1: PRINT "01234567890123456789";
    COLOR 12, 3
    LOCATE 4, 1: PRINT "!@#$%^&*()_+[]{}<>?/";

    COLOR 11, 5
    LOCATE 6, 4: PRINT "BOX";
    COLOR 15, 6
    LOCATE 7, 4: PRINT "REV";
    COLOR 0, 7
    LOCATE 8, 4: PRINT "INV";

    '_CLEARCOLOR _NONE, img
    _CLEARCOLOR ASC("?") + 256 * (12 + 16 * 3), img

    _DEST oldDest
END SUB

SUB PrepareTextDestination (img AS LONG)
    DIM oldDest AS LONG: oldDest = _DEST

    _DEST img
    CLS

    COLOR 7, 0
    DIM y AS LONG
    FOR y = 1 TO _HEIGHT(img)
        LOCATE y, 1
        PRINT STRING$(_WIDTH(img), ".");
    NEXT

    _DEST oldDest
END SUB

SUB PrepareGraphicsDestination32 (img AS LONG)
    DIM oldDest AS LONG: oldDest = _DEST

    _DEST img
    CLS , _RGB32(0, 0, 0)
    LINE (0, 0)-(_WIDTH(img) - 1, _HEIGHT(img) - 1), _RGB32(0, 0, 0), BF

    _DEST oldDest
END SUB

SUB PrepareGraphicsSource32 (img AS LONG)
    DIM oldDest AS LONG: oldDest = _DEST

    _DEST img
    CLS , _RGB32(0, 0, 0)
    LINE (0, 0)-(_WIDTH(img) - 1, _HEIGHT(img) - 1), _RGB32(255, 64, 32), BF
    LINE (8, 8)-(_WIDTH(img) - 9, _HEIGHT(img) - 9), _RGB32(32, 192, 255), BF
    LINE (16, 16)-(_WIDTH(img) - 17, _HEIGHT(img) - 17), _RGB32(255, 255, 64), BF

    _DEST oldDest
END SUB

SUB PrepareGraphicsDestinationIndexed (img AS LONG)
    DIM oldDest AS LONG
    oldDest = _DEST

    _DEST img
    CLS , 0
    LINE (0, 0)-(_WIDTH(img) - 1, _HEIGHT(img) - 1), 0, BF

    _DEST oldDest
END SUB

FUNCTION TextCell~% (img AS LONG, cx AS LONG, cy AS LONG)
    ' DOES NOT CLIP!
    DIM p AS _MEM: p = _MEMIMAGE(img)
    TextCell = _MEMGET(p, p.OFFSET + ((cy - 1) * _WIDTH(img) + (cx - 1)) * _SIZE_OF_INTEGER, _UNSIGNED INTEGER)
    _MEMFREE p
END FUNCTION

FUNCTION Pixel32~& (img AS LONG, x AS LONG, y AS LONG)
    ' DOES NOT CLIP!
    DIM p AS _MEM: p = _MEMIMAGE(img)
    Pixel32 = _MEMGET(p, p.OFFSET + (y * _WIDTH(img) + x) * _SIZE_OF_LONG, _UNSIGNED LONG)
    _MEMFREE p
END FUNCTION

FUNCTION PixelIndex~%% (img AS LONG, x AS LONG, y AS LONG)
    ' DOES NOT CLIP!
    DIM p AS _MEM: p = _MEMIMAGE(img)
    PixelIndex = _MEMGET(p, p.OFFSET + (y * _WIDTH(img) + x), _UNSIGNED _BYTE)
    _MEMFREE p
END FUNCTION

FUNCTION CountNonBlack32~& (img AS LONG, x1 AS LONG, y1 AS LONG, x2 AS LONG, y2 AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM c AS _UNSIGNED LONG
    DIM count AS _UNSIGNED LONG

    FOR y = y1 TO y2
        FOR x = x1 TO x2
            c = Pixel32(img, x, y)
            IF _RED32(c) <> 0 _ORELSE _GREEN32(c) <> 0 _ORELSE _BLUE32(c) <> 0 THEN count = count + 1
        NEXT
    NEXT

    CountNonBlack32 = count
END FUNCTION

FUNCTION CountNonBlackOutsideRect32~& (img AS LONG, x1 AS LONG, y1 AS LONG, x2 AS LONG, y2 AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM c AS _UNSIGNED LONG
    DIM count AS _UNSIGNED LONG

    FOR y = 0 TO _HEIGHT(img) - 1
        FOR x = 0 TO _WIDTH(img) - 1
            IF x >= x1 _ANDALSO x <= x2 _ANDALSO y >= y1 _ANDALSO y <= y2 THEN
                ' inside rectangle
            ELSE
                c = Pixel32(img, x, y)
                IF _RED32(c) <> 0 _ORELSE _GREEN32(c) <> 0 _ORELSE _BLUE32(c) <> 0 THEN count = count + 1
            END IF
        NEXT
    NEXT

    CountNonBlackOutsideRect32 = count
END FUNCTION

FUNCTION SumRegion32~&& (img AS LONG, x1 AS LONG, y1 AS LONG, x2 AS LONG, y2 AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM s AS _UNSIGNED _INTEGER64

    FOR y = y1 TO y2
        FOR x = x1 TO x2
            s = s + Pixel32(img, x, y)
        NEXT
    NEXT

    SumRegion32 = s
END FUNCTION

FUNCTION CountNonZeroIndexed~& (img AS LONG, x1 AS LONG, y1 AS LONG, x2 AS LONG, y2 AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM count AS _UNSIGNED LONG

    FOR y = y1 TO y2
        FOR x = x1 TO x2
            IF PixelIndex(img, x, y) <> 0 THEN count = count + 1
        NEXT
    NEXT

    CountNonZeroIndexed = count
END FUNCTION

FUNCTION RegionsEqualText%% (srcImg AS LONG, srcX AS LONG, srcY AS LONG, dstImg AS LONG, dstX AS LONG, dstY AS LONG, w AS LONG, h AS LONG, dstStepX AS LONG, dstStepY AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM sx AS LONG
    DIM sy AS LONG
    DIM dx AS LONG
    DIM dy AS LONG

    RegionsEqualText = _TRUE

    FOR y = 0 TO h - 1
        FOR x = 0 TO w - 1
            sx = srcX + x
            sy = srcY + y
            dx = dstX + x * dstStepX
            dy = dstY + y * dstStepY

            IF TextCell(srcImg, sx, sy) <> TextCell(dstImg, dx, dy) THEN
                RegionsEqualText = _FALSE
                EXIT FUNCTION
            END IF
        NEXT
    NEXT
END FUNCTION

FUNCTION RegionsEqual32%% (srcImg AS LONG, srcX AS LONG, srcY AS LONG, dstImg AS LONG, dstX AS LONG, dstY AS LONG, w AS LONG, h AS LONG, dstStepX AS LONG, dstStepY AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM sx AS LONG
    DIM sy AS LONG
    DIM dx AS LONG
    DIM dy AS LONG

    RegionsEqual32 = _TRUE

    FOR y = 0 TO h - 1
        FOR x = 0 TO w - 1
            sx = srcX + x
            sy = srcY + y
            dx = dstX + x * dstStepX
            dy = dstY + y * dstStepY

            IF Pixel32(srcImg, sx, sy) <> Pixel32(dstImg, dx, dy) THEN
                RegionsEqual32 = _FALSE
                EXIT FUNCTION
            END IF
        NEXT
    NEXT
END FUNCTION

FUNCTION RegionsEqualIndexed%% (srcImg AS LONG, srcX AS LONG, srcY AS LONG, dstImg AS LONG, dstX AS LONG, dstY AS LONG, w AS LONG, h AS LONG, dstStepX AS LONG, dstStepY AS LONG)
    DIM x AS LONG
    DIM y AS LONG
    DIM sx AS LONG
    DIM sy AS LONG
    DIM dx AS LONG
    DIM dy AS LONG

    RegionsEqualIndexed = _TRUE

    FOR y = 0 TO h - 1
        FOR x = 0 TO w - 1
            sx = srcX + x
            sy = srcY + y
            dx = dstX + x * dstStepX
            dy = dstY + y * dstStepY

            IF PixelIndex(srcImg, sx, sy) <> PixelIndex(dstImg, dx, dy) THEN
                RegionsEqualIndexed = _FALSE
                EXIT FUNCTION
            END IF
        NEXT
    NEXT
END FUNCTION

SUB TestTextToTextBasic
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst

    _PUTIMAGE (4, 3), srcText, dst
    '_SAVEIMAGE "TestTextToTextBasic", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF TextCell(dst, 5, 4) <> TextCell(srcText, 1, 1) THEN ok = _FALSE
    IF TextCell(dst, 10, 4) <> TextCell(srcText, 6, 1) THEN ok = _FALSE
    IF TextCell(dst, 7, 6) <> TextCell(srcText, 3, 3) THEN ok = _FALSE
    IF TextCell(dst, 24, 13) <> TextCell(srcText, 20, 10) THEN ok = _FALSE

    ReportCheck "text->text basic", ok
    _FREEIMAGE dst
END SUB

SUB TestTextToTextReversed
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst

    _PUTIMAGE (14, 7)-(9, 5), srcText, dst, (5, 2)-(0, 0)
    '_SAVEIMAGE "TestTextToTextReversed", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF TextCell(dst, 15, 8) <> TextCell(srcText, 6, 3) THEN ok = _FALSE
    IF TextCell(dst, 10, 6) <> TextCell(srcText, 1, 1) THEN ok = _FALSE
    IF TextCell(dst, 14, 8) <> TextCell(srcText, 5, 3) THEN ok = _FALSE
    IF TextCell(dst, 15, 7) <> TextCell(srcText, 6, 2) THEN ok = _FALSE

    ReportCheck "text->text reversed rect", ok
    _FREEIMAGE dst
END SUB

SUB TestTextToTextMirrored
    DIM ref AS LONG: ref = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination ref
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst

    _PUTIMAGE (5, 3), srcText, ref
    '_SAVEIMAGE "TestTextToTextMirroredRef", ref

    _PUTIMAGE (24, 3)-(5, 12), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToTextMirroredDst", dst

    DIM ok AS _BYTE: ok = RegionsEqualText(ref, 6, 4, dst, 25, 4, 20, 10, -1, 1)
    IF TextCell(dst, 25, 4) <> TextCell(srcText, 1, 1) THEN ok = _FALSE
    IF TextCell(dst, 6, 4) <> TextCell(srcText, 20, 1) THEN ok = _FALSE

    ReportCheck "text->text mirrored", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToTextFlipped
    DIM ref AS LONG: ref = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination ref
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst

    _PUTIMAGE (5, 3), srcText, ref
    '_SAVEIMAGE "TestTextToTextFlippedRef", ref

    _PUTIMAGE (24, 12)-(5, 3), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToTextFlippedDst", dst

    DIM ok AS _BYTE: ok = RegionsEqualText(ref, 6, 4, dst, 25, 13, 20, 10, -1, -1)
    IF TextCell(dst, 6, 4) <> TextCell(srcText, 20, 10) THEN ok = _FALSE
    IF TextCell(dst, 25, 13) <> TextCell(srcText, 1, 1) THEN ok = _FALSE

    ReportCheck "text->text flipped", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToTextClipping
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst

    _PUTIMAGE (-3, 0), srcText, dst
    '_SAVEIMAGE "TestTextToTextClipping", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF TextCell(dst, 1, 1) <> TextCell(srcText, 4, 1) THEN ok = _FALSE
    IF TextCell(dst, 2, 1) <> TextCell(srcText, 5, 1) THEN ok = _FALSE

    ReportCheck "text->text clipping", ok
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics32Basic
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _PUTIMAGE (16, 16), srcText, dst
    '_SAVEIMAGE "TestTextToGraphics32Basic", dst

    DIM nonBlack AS _UNSIGNED LONG: nonBlack = CountNonBlack32(dst, 16, 16, 16 + 20 * 8 - 1, 16 + 10 * 16 - 1)
    DIM ok AS _BYTE: ok = (nonBlack > 0)
    IF Pixel32(dst, 0, 0) <> _RGB32(0, 0, 0) THEN ok = _FALSE

    ReportCheck "text->graphics 32bpp basic", ok
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics32ViewClip
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _DEST dst
    VIEW (40, 40)-(120, 120)
    _PUTIMAGE (0, 0), srcText
    VIEW
    '_SAVEIMAGE "TestTextToGraphics32ViewClip", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF CountNonBlack32(dst, 40, 40, 120, 120) <= 0 THEN ok = _FALSE
    IF CountNonBlackOutsideRect32(dst, 40, 40, 120, 120) <> 0 THEN ok = _FALSE

    ReportCheck "text->graphics 32bpp VIEW clip", ok
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics32WindowNoScale
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _DEST dst
    VIEW (40, 40)-(199, 199)
    WINDOW SCREEN(0, 0)-(159, 159)
    _PUTIMAGE (0, 0), srcText
    WINDOW
    VIEW
    '_SAVEIMAGE "TestTextToGraphics32WindowNoScale", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF CountNonBlack32(dst, 40, 40, 199, 199) <= 0 THEN ok = _FALSE
    IF CountNonBlackOutsideRect32(dst, 40, 40, 199, 199) <> 0 THEN ok = _FALSE

    ReportCheck "text->graphics 32bpp WINDOW", ok
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics32Reversed
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 ref


    _PUTIMAGE (100, 80)-(147, 111), srcText, ref, (0, 0)-(5, 1)
    '_SAVEIMAGE "TestTextToGraphics32ReversedRef", ref

    _PUTIMAGE (200, 120)-(153, 89), srcText, dst, (5, 1)-(0, 0)
    '_SAVEIMAGE "TestTextToGraphics32ReversedDst", dst

    DIM nonBlackRef AS _UNSIGNED LONG: nonBlackRef = CountNonBlack32(ref, 100, 80, 147, 111)
    DIM nonBlackDst AS _UNSIGNED LONG: nonBlackDst = CountNonBlack32(dst, 153, 89, 200, 120)

    DIM sumRef AS _UNSIGNED _INTEGER64: sumRef = SumRegion32(ref, 100, 80, 147, 111)
    DIM sumDst AS _UNSIGNED _INTEGER64: sumDst = SumRegion32(dst, 153, 89, 200, 120)

    DIM ok AS _BYTE: ok = _TRUE
    IF nonBlackRef <= 0 THEN ok = _FALSE
    IF nonBlackDst <> nonBlackRef THEN ok = _FALSE
    IF sumDst <> sumRef THEN ok = _FALSE

    ReportCheck "text->graphics 32bpp reversed rect", ok

    _FREEIMAGE ref
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics32Mirrored
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 ref
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _PUTIMAGE (20, 20), srcText, ref
    '_SAVEIMAGE "TestTextToGraphics32MirroredRef", ref

    _PUTIMAGE (179, 20)-(20, 179), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToGraphics32MirroredDst", dst

    DIM ok AS _BYTE: ok = RegionsEqual32(ref, 20, 20, dst, 179, 20, 160, 160, -1, 1)

    ReportCheck "text->graphics 32bpp mirrored", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToGraphics32Flipped
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 ref
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _PUTIMAGE (20, 20), srcText, ref
    '_SAVEIMAGE "TestTextToGraphics32FlippedRef", ref

    _PUTIMAGE (179, 179)-(20, 20), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToGraphics32FlippedDst", dst

    DIM ok AS _BYTE: ok = RegionsEqual32(ref, 20, 20, dst, 179, 179, 160, 160, -1, -1)

    ReportCheck "text->graphics 32bpp flipped", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToGraphics32Clipping
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst


    _PUTIMAGE (-12, -10), srcText, dst
    '_SAVEIMAGE "TestTextToGraphics32Clipping", dst

    DIM ok AS _BYTE: ok = (CountNonBlack32(dst, 0, 0, 80, 80) > 0)

    ReportCheck "text->graphics 32bpp clipping", ok
    _FREEIMAGE dst
END SUB

SUB TestGraphicsToGraphics32ViewClip
    DIM src AS LONG: src = _NEWIMAGE(80, 80, 32)
    PrepareGraphicsSource32 src
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _DEST dst
    VIEW (40, 40)-(99, 99)
    _PUTIMAGE (0, 0), src, dst
    VIEW
    '_SAVEIMAGE "TestGraphicsToGraphics32ViewClip", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF CountNonBlack32(dst, 40, 40, 99, 99) <= 0 THEN ok = _FALSE
    IF CountNonBlackOutsideRect32(dst, 40, 40, 99, 99) <> 0 THEN ok = _FALSE
    IF Pixel32(dst, 40, 40) <> _RGB32(255, 64, 32) THEN ok = _FALSE
    IF Pixel32(dst, 48, 48) <> _RGB32(32, 192, 255) THEN ok = _FALSE
    IF Pixel32(dst, 56, 56) <> _RGB32(255, 255, 64) THEN ok = _FALSE
    IF Pixel32(dst, 99, 99) <> _RGB32(255, 255, 64) THEN ok = _FALSE

    ReportCheck "graphics->graphics 32bpp VIEW clip", ok
    _FREEIMAGE dst
    _FREEIMAGE src
END SUB

SUB TestGraphicsToGraphics32WindowNoScale
    DIM src AS LONG: src = _NEWIMAGE(80, 80, 32)
    PrepareGraphicsSource32 src
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 ref

    _PUTIMAGE (80, 80), src, ref

    _DEST dst
    VIEW (40, 40)-(199, 199)
    WINDOW SCREEN(0, 0)-(159, 159)
    _PUTIMAGE (40, 40), src, dst
    WINDOW
    VIEW
    '_SAVEIMAGE "TestGraphicsToGraphics32WindowNoScale", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF RegionsEqual32(ref, 80, 80, dst, 80, 80, 80, 80, 1, 1) = _FALSE THEN ok = _FALSE
    IF CountNonBlackOutsideRect32(dst, 80, 80, 159, 159) <> 0 THEN ok = _FALSE

    ReportCheck "graphics->graphics 32bpp WINDOW", ok
    _FREEIMAGE ref
    _FREEIMAGE dst
    _FREEIMAGE src
END SUB

SUB TestGraphicsToGraphics32SourceViewClip
    DIM src AS LONG: src = _NEWIMAGE(80, 80, 32)
    PrepareGraphicsSource32 src
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    _DEST src
    VIEW (20, 20)-(59, 59)
    _PUTIMAGE (30, 30), src, dst
    VIEW
    '_SAVEIMAGE "TestGraphicsToGraphics32SourceViewClip", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF CountNonBlack32(dst, 50, 50, 89, 89) <= 0 THEN ok = _FALSE
    IF CountNonBlackOutsideRect32(dst, 50, 50, 89, 89) <> 0 THEN ok = _FALSE

    ReportCheck "graphics->graphics 32bpp source VIEW", ok
    _FREEIMAGE dst
    _FREEIMAGE src
END SUB

SUB TestTextToGraphics8Basic
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 256)
    PrepareGraphicsDestinationIndexed dst

    _PUTIMAGE (24, 24), srcText, dst
    '_SAVEIMAGE "TestTextToGraphics8Basic", dst

    DIM ok AS _BYTE: ok = (CountNonZeroIndexed(dst, 24, 24, 24 + 20 * 8 - 1, 24 + 10 * 16 - 1) > 0)
    ReportCheck "text->graphics 8bpp basic", ok

    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics8Mirrored
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 256)
    PrepareGraphicsDestinationIndexed ref
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 256)
    PrepareGraphicsDestinationIndexed dst

    _PUTIMAGE (20, 20), srcText, ref
    '_SAVEIMAGE "TestTextToGraphics8MirroredRef", ref

    _PUTIMAGE (179, 20)-(20, 179), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToGraphics8MirroredDst", dst

    DIM ok AS _BYTE: ok = RegionsEqualIndexed(ref, 20, 20, dst, 179, 20, 160, 160, -1, 1)

    ReportCheck "text->graphics 8bpp mirrored", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToGraphics8Flipped
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 256)
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 256)

    PrepareGraphicsDestinationIndexed ref
    PrepareGraphicsDestinationIndexed dst

    _PUTIMAGE (20, 20), srcText, ref
    '_SAVEIMAGE "TestTextToGraphics8FlippedRef", ref

    _PUTIMAGE (179, 179)-(20, 20), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToGraphics8FlippedDst", dst

    DIM ok AS _BYTE: ok = RegionsEqualIndexed(ref, 20, 20, dst, 179, 179, 160, 160, -1, -1)

    ReportCheck "text->graphics 8bpp flipped", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToGraphics4Basic
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 12)
    PrepareGraphicsDestinationIndexed dst

    _PUTIMAGE (24, 24), srcText, dst
    '_SAVEIMAGE "TestTextToGraphics4Basic", dst

    DIM ok AS _BYTE: ok = (CountNonZeroIndexed(dst, 24, 24, 24 + 20 * 8 - 1, 24 + 10 * 16 - 1) > 0)
    ReportCheck "text->graphics 4bpp basic", ok

    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics4Mirrored
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 12)
    PrepareGraphicsDestinationIndexed ref
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 12)
    PrepareGraphicsDestinationIndexed dst

    _PUTIMAGE (20, 20), srcText, ref
    '_SAVEIMAGE "TestTextToGraphics4MirroredRef", ref

    _PUTIMAGE (179, 20)-(20, 179), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToGraphics4MirroredDst", dst

    DIM ok AS _BYTE: ok = RegionsEqualIndexed(ref, 20, 20, dst, 179, 20, 160, 160, -1, 1)

    ReportCheck "text->graphics 4bpp mirrored", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToGraphics4Flipped
    DIM ref AS LONG: ref = _NEWIMAGE(320, 200, 12)
    PrepareGraphicsDestinationIndexed ref
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 12)
    PrepareGraphicsDestinationIndexed dst

    _PUTIMAGE (20, 20), srcText, ref
    '_SAVEIMAGE "TestTextToGraphics4FlippedRef", ref

    _PUTIMAGE (179, 179)-(20, 20), srcText, dst, (0, 0)-(19, 9)
    '_SAVEIMAGE "TestTextToGraphics4FlippedDst", dst

    DIM ok AS _BYTE: ok = RegionsEqualIndexed(ref, 20, 20, dst, 179, 179, 160, 160, -1, -1)

    ReportCheck "text->graphics 4bpp flipped", ok

    _FREEIMAGE dst
    _FREEIMAGE ref
END SUB

SUB TestTextToTextViewPrint
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst
    DIM ref AS LONG: ref = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination ref

    _DEST dst
    VIEW PRINT 6 TO 10
    _PUTIMAGE (0, 0), srcText
    VIEW PRINT
    '_SAVEIMAGE "TestTextToTextViewPrint", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF TextCell(dst, 1, 2) <> TextCell(ref, 1, 2) THEN ok = _FALSE
    IF TextCell(dst, 1, 6) <> TextCell(srcText, 1, 6) THEN ok = _FALSE
    IF TextCell(dst, 20, 10) <> TextCell(srcText, 20, 10) THEN ok = _FALSE
    IF TextCell(dst, 1, 11) <> TextCell(ref, 1, 11) THEN ok = _FALSE

    ReportCheck "text->text VIEW PRINT", ok

    _FREEIMAGE ref
    _FREEIMAGE dst
END SUB

SUB TestTextToTextScaleIllegal
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)
    PrepareTextDestination dst

    errCaught = _FALSE
    ON ERROR GOTO err_text_to_text_scale
    _PUTIMAGE (0, 0)-(29, 15), srcText, dst
    ON ERROR GOTO 0

    ReportCheck "text->text scale illegal", errCaught
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphicsScaleIllegal
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 32)
    PrepareGraphicsDestination32 dst

    errCaught = _FALSE
    ON ERROR GOTO err_text_to_gfx_scale
    _PUTIMAGE (0, 0)-(79, 79), srcText, dst
    ON ERROR GOTO 0

    ReportCheck "text->graphics scale illegal", errCaught
    _FREEIMAGE dst
END SUB

SUB TestTextToGraphics2Illegal
    DIM dst AS LONG: dst = _NEWIMAGE(320, 200, 1)
    PrepareGraphicsDestinationIndexed dst

    errCaught = _FALSE
    ON ERROR GOTO err_text_to_gfx2
    _PUTIMAGE (0, 0), srcText, dst
    ON ERROR GOTO 0

    ReportCheck "text->graphics 2bpp illegal", errCaught
    _FREEIMAGE dst
END SUB

SUB TestGraphicsToTextIllegal
    DIM src AS LONG: src = _NEWIMAGE(64, 64, 32)
    DIM dst AS LONG: dst = _NEWIMAGE(30, 16, 0)

    PrepareTextDestination dst
    _DEST src
    CLS , _RGB32(0, 0, 0)
    LINE (0, 0)-(63, 63), _RGB32(255, 0, 0), BF

    errCaught = _FALSE
    ON ERROR GOTO err_gfx_to_text
    _PUTIMAGE (1, 1), src, dst
    ON ERROR GOTO 0

    ReportCheck "graphics->text illegal", errCaught

    _FREEIMAGE dst
    _FREEIMAGE src
END SUB

SUB TestTextToGraphics32TTFFont
    DIM fnt AS LONG: fnt = _LOADFONT("../font/LiberationMono-Regular.ttf", 16, "monospace")
    IF fnt <= 0 THEN
        ReportCheck "text->graphics 32bpp TTF font", _FALSE
        EXIT SUB
    END IF

    DIM fW AS INTEGER: fW = _FONTWIDTH(fnt)
    DIM fH AS INTEGER: fH = _FONTHEIGHT(fnt)
    DIM src AS LONG: src = _NEWIMAGE(10, 4, 0)
    DIM oldDest AS LONG: oldDest = _DEST

    _DEST src
    _FONT fnt
    COLOR 15, 1
    CLS
    LOCATE 1, 1: PRINT "ABCDEFGHIJ";
    LOCATE 2, 1: PRINT "abcdefghij";
    _DEST oldDest

    DIM expectedW AS LONG: expectedW = 10 * fW
    DIM expectedH AS LONG: expectedH = 4 * fH
    DIM dst AS LONG: dst = _NEWIMAGE(expectedW + 40, expectedH + 40, 32)
    PrepareGraphicsDestination32 dst

    _PUTIMAGE (20, 20), src, dst
    '_SAVEIMAGE "TestTextToGraphics32TTFFont", dst

    DIM ok AS _BYTE: ok = _TRUE
    IF CountNonBlack32(dst, 20, 20, 20 + expectedW - 1, 20 + expectedH - 1) <= 0 THEN ok = _FALSE
    IF CountNonBlackOutsideRect32(dst, 20, 20, 20 + expectedW - 1, 20 + expectedH - 1) <> 0 THEN ok = _FALSE

    ReportCheck "text->graphics 32bpp TTF font", ok

    _FREEIMAGE src
    _FREEIMAGE dst
    _FREEFONT fnt
END SUB
