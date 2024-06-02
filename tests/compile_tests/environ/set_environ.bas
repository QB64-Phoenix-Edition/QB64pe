$CONSOLE:ONLY
ON ERROR GOTO ehandler

'Test setting with =
ENVIRON "FOO=BAR"
PRINT ENVIRON$("FOO")


'Test settings with space
ENVIRON "VAR VAL"
PRINT ENVIRON$("VAR")


'Test setting value with spaces with = separator
ENVIRON "ABC=DEF GHI"
PRINT ENVIRON$("ABC")


'Test setting value with spaces with space separator
ENVIRON "JKL MNO PQR"
PRINT ENVIRON$("JKL")


'Test overwriting existing
ENVIRON "X=XY"
ENVIRON "X=ZZZ"
PRINT ENVIRON$("X")


'Test unset variable with = separator
ENVIRON "NAME=LUKE"
ENVIRON "NAME="
PRINT "["; ENVIRON$("NAME"); "]"


'Test unset variable with space separator
ENVIRON "TEXT BOO"
ENVIRON "TEXT "
PRINT "["; ENVIRON$("TEXT"); "]"

'Test no separator
ENVIRON "NOSEP"

SYSTEM

ehandler:
print "Error"; ERR; "line"; _ERRORLINE
RESUME NEXT
