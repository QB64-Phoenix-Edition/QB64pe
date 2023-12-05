$CONSOLE:ONLY
OPTION _EXPLICIT

DIM fileName AS STRING

READ fileName
WHILE LEN(fileName) > 0
    DIM img AS LONG: img = _LOADIMAGE(fileName)

    IF img < -1 THEN
        PRINT fileName; ": ("; _WIDTH(img); "x"; _HEIGHT(img); ") pixels"
        _FREEIMAGE img
    ELSE
        PRINT fileName; " is not a valid image file!"
    END IF

    READ fileName
WEND

SYSTEM

DATA good1.svg
DATA good2.svg
DATA bogus1.svg
DATA bogus2.svg
DATA
