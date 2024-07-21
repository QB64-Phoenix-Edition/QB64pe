DEFLNG A-Z
OPTION _EXPLICIT
$CONSOLE:ONLY
CHDIR _STARTDIR$

CONST TEST_FILE_FORMAT = "bmp"
CONST TOLERANCE_LIMIT = 0

ON ERROR GOTO error_handler

RESTORE file_list

DIM fileName AS STRING: READ fileName

DO
    DoImageFile fileName
    READ fileName
LOOP WHILE LEN(fileName)

SYSTEM

error_handler:
PRINT "Error: "; _ERRORMESSAGE$; " at line"; _ERRORLINE;

DIM incErrFile AS STRING: incErrFile = _INCLERRORFILE$
DIM incErrLine AS LONG: incErrLine = _INCLERRORLINE

IF LEN(incErrFile) THEN
    PRINT "( "; incErrFile + " at line"; incErrLine; ")"
ELSE
    PRINT
END IF

SYSTEM

file_list:
DATA "1bpp.ico"
DATA "24bpp.ico"
DATA "4bpp.cur"
DATA "4bpp.ico"
DATA "8bpp.ico"
DATA "8bpp.cur"
DATA "bard.ico"
DATA "broom.ico"
DATA "cat.ico"
DATA "computer.ico"
DATA "dumb1.ico"
DATA "dumb2.ico"
DATA "pencil.ico"
DATA "please_wait.ico"
DATA "question.ico"
DATA "really_huge_duck.ico"
DATA "sample3.ico"
DATA "sample4.ico"
DATA "soft_drink.ico"
DATA "taco.ico"
DATA "terra_globe.ico"
DATA "thumbs_up.ico"
DATA "volme_dialog.ico"
DATA "word-icon.ico"
DATA "x.ico"
DATA ""

SUB PrintImageDetails (handle AS LONG, testFileName AS STRING)
    DIM fullTestFileName AS STRING: fullTestFileName = testFileName + "." + TEST_FILE_FORMAT

    PRINT "Testing against "; fullTestFileName; " ("; _WIDTH(handle); "x"; _HEIGHT(handle); ")."
    '_SAVEIMAGE fullTestFileName, handle
    AssertImage2 handle, fullTestFileName, TOLERANCE_LIMIT

    PRINT
END SUB


SUB DoImageFile (testFileName AS STRING)
    DIM fileName AS STRING: fileName = testFileName

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
