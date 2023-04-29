OPTION _EXPLICIT
$CONSOLE:ONLY
CHDIR _STARTDIR$

DIM fh AS LONG

PRINT "Loading font using filename...";
fh = _LOADFONT("slick.ttf", 10)

IF fh > 0 THEN
    PRINT "done."
    PRINT "Font height ="; _FONTHEIGHT(fh)
    _FREEFONT fh
ELSE
    PRINT "failed!"
END IF


PRINT "Loading font from memory...";
fh = _LOADFONT(LoadFileFromDisk("slick.ttf"), 20, "memory")

IF fh > 0 THEN
    PRINT "done."
    PRINT "Font height ="; _FONTHEIGHT(fh)
    _FREEFONT fh
ELSE
    PRINT "failed!"
END IF


SYSTEM

' Loads a whole file from disk into memory
FUNCTION LoadFileFromDisk$ (path AS STRING)
    IF _FILEEXISTS(path) THEN
        DIM AS LONG fh: fh = FREEFILE

        OPEN path FOR BINARY ACCESS READ AS fh

        LoadFileFromDisk = INPUT$(LOF(fh), fh)

        CLOSE fh
    END IF
END FUNCTION

