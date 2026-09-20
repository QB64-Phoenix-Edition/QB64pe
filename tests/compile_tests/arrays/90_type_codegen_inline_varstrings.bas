$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE InlineStrings
    Names(0 TO 1023) AS STRING
    Numbers(0 TO 63) AS LONG
    Tail AS STRING
END TYPE

REDIM src(0 TO 1) AS InlineStrings
REDIM dst(0 TO 1) AS InlineStrings
DIM i AS LONG

FOR i = 0 TO 1023 STEP 127
    src(0).Names(i) = "S" + LTRIM$(STR$(i))
NEXT
src(0).Names(1023) = "LAST"
src(0).Numbers(63) = 6300
src(0).Tail = "source-tail"

' Whole-array assignment exercises deep copy of the inline variable-STRING members.
dst() = src()

IF dst(0).Names(0) <> "S0" THEN GOTO failed
IF dst(0).Names(254) <> "S254" THEN GOTO failed
IF dst(0).Names(1023) <> "LAST" THEN GOTO failed
IF dst(0).Numbers(63) <> 6300 THEN GOTO failed
IF dst(0).Tail <> "source-tail" THEN GOTO failed

' The destination must own independent qbs objects.
src(0).Names(254) = "changed"
src(0).Tail = "changed-tail"
IF dst(0).Names(254) <> "S254" THEN GOTO failed
IF dst(0).Tail <> "source-tail" THEN GOTO failed

' Exercise member clear and parent-array free paths.
ERASE src(0).Names
IF src(0).Names(254) <> "" THEN GOTO failed
IF dst(0).Names(254) <> "S254" THEN GOTO failed

ERASE src
IF dst(0).Names(1023) <> "LAST" THEN GOTO failed

PRINT "PASS 90_type_codegen_inline_varstrings.bas"
SYSTEM

failed:
PRINT "FAIL 90_type_codegen_inline_varstrings.bas"
SYSTEM
