$CONSOLE:ONLY
$UNSTABLE:TYPEFIELDS
OPTION _EXPLICIT

TYPE DynamicStrings
    Names(0 TO 1) _DYNAMIC AS STRING
    Id AS LONG
END TYPE

REDIM src(0 TO 1) AS DynamicStrings
REDIM dst(0 TO 1) AS DynamicStrings

REDIM src(0).Names(-4 TO 300)
src(0).Names(-4) = "low"
src(0).Names(123) = "middle"
src(0).Names(300) = "high"
src(0).Id = 42

' Assignment must clone the descriptor payload and every owned qbs string.
dst() = src()

IF LBOUND(dst(0).Names) <> -4 THEN GOTO failed
IF UBOUND(dst(0).Names) <> 300 THEN GOTO failed
IF dst(0).Names(-4) <> "low" THEN GOTO failed
IF dst(0).Names(123) <> "middle" THEN GOTO failed
IF dst(0).Names(300) <> "high" THEN GOTO failed
IF dst(0).Id <> 42 THEN GOTO failed

src(0).Names(123) = "changed"
IF dst(0).Names(123) <> "middle" THEN GOTO failed

' _RETAIN exercises descriptor replacement while preserving existing payload.
REDIM _RETAIN src(0).Names(-8 TO 400)
IF src(0).Names(-4) <> "low" THEN GOTO failed
IF src(0).Names(123) <> "changed" THEN GOTO failed
IF src(0).Names(300) <> "high" THEN GOTO failed
src(0).Names(400) = "new-high"
IF src(0).Names(400) <> "new-high" THEN GOTO failed

ERASE src(0).Names
IF dst(0).Names(300) <> "high" THEN GOTO failed
ERASE src
IF dst(0).Names(-4) <> "low" THEN GOTO failed

PRINT "PASS 92_type_codegen_dynamic_varstrings.bas"
SYSTEM

failed:
PRINT "FAIL 92_type_codegen_dynamic_varstrings.bas"
SYSTEM
