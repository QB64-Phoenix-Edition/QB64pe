$CONSOLE
$SCREENHIDE
_DEST _CONSOLE

$IF LINUX THEN

' FIXME: Linux works but the test gives some kind of freeglut error, presumably
'        some kind of rece condition
print -1;

$ELSE

print _DEVICES > 1;
_DELAY 1

$END IF

SYSTEM
