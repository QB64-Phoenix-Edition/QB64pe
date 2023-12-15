$EMBED:'./test.output','test'

$CONSOLE
$SCREENHIDE
_DEST _CONSOLE

dat$ = _EMBEDDED$("test")
FOR i% = 1 TO LEN(dat$)
    IF ASC(dat$, i%) = 10 OR ASC(dat$, i%) = 13 THEN EXIT FOR
    PRINT MID$(dat$, i%, 1);
NEXT i%

SYSTEM

