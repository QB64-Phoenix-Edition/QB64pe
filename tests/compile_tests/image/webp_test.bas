OPTION _EXPLICIT
$CONSOLE:ONLY
CHDIR _STARTDIR$

CONST TEST_IMAGE_FORMAT = "bmp"
CONST TEST_IMAGE_01 = "1"
CONST TEST_IMAGE_02 = "2"
CONST TEST_IMAGE_03 = "3"
CONST TEST_IMAGE_04 = "4"
CONST TEST_IMAGE_05 = "5"
CONST TOLERANCE_LIMIT = 0

DoImageFile TEST_IMAGE_01
DoImageFile TEST_IMAGE_02
DoImageFile TEST_IMAGE_03
DoImageFile TEST_IMAGE_04
DoImageFile TEST_IMAGE_05

SYSTEM


SUB PrintImageDetails (handle AS LONG, testFileName AS STRING)
    _DEST handle

    DIM iWidth AS LONG: iWidth = _WIDTH(handle)
    DIM iHeight AS LONG: iHeight = _HEIGHT(handle)

    _DEST _CONSOLE

    DIM fullTestFileName AS STRING: fullTestFileName = testFileName + "." + TEST_IMAGE_FORMAT

    PRINT "Testing against "; fullTestFileName; " ("; iWidth; "x"; iHeight; ")."
    '_SAVEIMAGE testFileName, handle, TEST_IMAGE_FORMAT
    AssertImage2 handle, fullTestFileName, TOLERANCE_LIMIT

    PRINT
END SUB


SUB DoImageFile (testFileName AS STRING)
    DIM fileName AS STRING: fileName = testFileName + ".webp"

    PRINT "Loading image from storage "; fileName; " ... ";

    DIM h AS LONG: h = _LOADIMAGE(fileName, 32)

    IF h < -1 THEN
        PRINT "done."

        PrintImageDetails h, testFileName

        _FREEIMAGE h
    ELSE
        PRINT "failed!"
    END IF
END SUB

'$INCLUDE:'../utilities/imageassert.bm'
'$INCLUDE:'../utilities/base64.bm'
