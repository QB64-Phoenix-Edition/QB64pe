$EMBED:'tests/compile_tests/embed/test.output','test'

$CONSOLE
$SCREENHIDE
_DEST _CONSOLE

dat$ = _EMBEDDED$("test")
PRINT LEFT$(dat$, LEN(dat$) - 2);

SYSTEM

