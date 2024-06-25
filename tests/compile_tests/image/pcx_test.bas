OPTION _EXPLICIT
$CONSOLE:ONLY
CHDIR _STARTDIR$

CONST TEST_IMAGE_FORMAT = "bmp"
CONST TEST_IMAGE_01 = "16color1"
CONST TEST_IMAGE_02 = "16color2"
CONST TEST_IMAGE_03 = "blood4"
CONST TEST_IMAGE_04 = "fm_d0010"
CONST TEST_IMAGE_05 = "gmarbles"
CONST TEST_IMAGE_06 = "lena"
CONST TEST_IMAGE_07 = "lena10"
CONST TEST_IMAGE_08 = "lena2"
CONST TEST_IMAGE_09 = "lena3"
CONST TEST_IMAGE_10 = "lena4"
CONST TEST_IMAGE_11 = "lena5"
CONST TEST_IMAGE_12 = "lena7"
CONST TEST_IMAGE_13 = "lena8"
CONST TEST_IMAGE_14 = "lena9"
CONST TEST_IMAGE_15 = "marbles"
CONST TEST_IMAGE_16 = "prey0013"
CONST TEST_IMAGE_17 = "sample_1280_853"
CONST TEST_IMAGE_18 = "sample_1920_1280"
CONST TEST_IMAGE_19 = "sample_640_426"
CONST TEST_IMAGE_20 = "swpic2"
CONST TEST_IMAGE_21 = "title"
CONST TOLERANCE_LIMIT = 0

DoImageFile TEST_IMAGE_01
DoImageFile TEST_IMAGE_02
DoImageFile TEST_IMAGE_03
DoImageFile TEST_IMAGE_04
DoImageFile TEST_IMAGE_05
DoImageFile TEST_IMAGE_06
DoImageFile TEST_IMAGE_07
DoImageFile TEST_IMAGE_08
DoImageFile TEST_IMAGE_09
DoImageFile TEST_IMAGE_10
DoImageFile TEST_IMAGE_11
DoImageFile TEST_IMAGE_12
DoImageFile TEST_IMAGE_13
DoImageFile TEST_IMAGE_14
DoImageFile TEST_IMAGE_15
DoImageFile TEST_IMAGE_16
DoImageFile TEST_IMAGE_17
DoImageFile TEST_IMAGE_18
DoImageFile TEST_IMAGE_19
DoImageFile TEST_IMAGE_20
DoImageFile TEST_IMAGE_21

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
    DIM fileName AS STRING: fileName = testFileName + ".pcx"

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
