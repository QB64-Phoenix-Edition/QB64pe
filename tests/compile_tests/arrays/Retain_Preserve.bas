$CONSOLE:ONLY

TYPE TestRec
    n AS STRING
    m AS LONG
    l AS STRING * 8
END TYPE

DIM f AS LONG
DIM loopA AS LONG
DIM loopB AS LONG
DIM loopC AS LONG

DIM NL AS STRING
DIM Actual AS STRING
DIM Expected AS STRING

DIM ExpectedDynamic AS STRING
DIM ExpectedDynamic3D AS STRING
DIM ExpectedFixed1 AS STRING * 8
DIM ExpectedFixed2 AS STRING * 8
DIM ExpectedM AS LONG
DIM ExpectedM3D AS LONG
DIM ExpectedPreserveValue AS LONG

NL = CHR$(10)

ExpectedDynamic = "dynamic"
ExpectedDynamic3D = "3D test dynamic"
ExpectedFixed1 = "Qbasic"
ExpectedFixed2 = "fixed"
ExpectedM = 120000
ExpectedM3D = 199201
ExpectedPreserveValue = 123

Actual = ""
Expected = ""

REDIM T(5) AS TestRec

T(1).n = "dynamic"
T(1).m = 120000
T(1).l = "Qbasic"

FOR f = 1 TO 1000
    REDIM _PRESERVE T(5 + f) AS TestRec
NEXT f

Actual = Actual + T(1).n + NL
Expected = Expected + ExpectedDynamic + NL

Actual = Actual + STR$(T(1).m) + NL
Expected = Expected + STR$(ExpectedM) + NL

Actual = Actual + T(1).l + NL
Expected = Expected + ExpectedFixed1 + NL

Actual = Actual + STR$(UBOUND(T)) + NL
Expected = Expected + STR$(1005) + NL


REDIM _RETAIN T(5) AS TestRec

Actual = Actual + T(1).n + NL
Expected = Expected + ExpectedDynamic + NL

Actual = Actual + STR$(T(1).m) + NL
Expected = Expected + STR$(ExpectedM) + NL

Actual = Actual + T(1).l + NL
Expected = Expected + ExpectedFixed1 + NL

Actual = Actual + STR$(UBOUND(T)) + NL
Expected = Expected + STR$(5) + NL


FOR f = 1 TO 1000
    REDIM _RETAIN T(5 + f) AS TestRec
NEXT f

Actual = Actual + T(1).n + NL
Expected = Expected + ExpectedDynamic + NL

Actual = Actual + STR$(T(1).m) + NL
Expected = Expected + STR$(ExpectedM) + NL

Actual = Actual + T(1).l + NL
Expected = Expected + ExpectedFixed1 + NL

Actual = Actual + STR$(UBOUND(T)) + NL
Expected = Expected + STR$(1005) + NL


REDIM U(10, 100, 10) AS TestRec

U(1, 2, 3).n = "3D test dynamic"
U(6, 5, 4).l = "fixed"
U(6, 2, 4).m = 199201

FOR loopA = 10 TO 6 STEP -1
    FOR loopB = 5 TO 6
        FOR loopC = 7 TO 4 STEP -1
            REDIM _RETAIN U(loopA, loopB, loopC) AS TestRec
        NEXT loopC
    NEXT loopB
NEXT loopA

Actual = Actual + U(1, 2, 3).n + NL
Expected = Expected + ExpectedDynamic3D + NL

Actual = Actual + U(6, 5, 4).l + NL
Expected = Expected + ExpectedFixed2 + NL

Actual = Actual + STR$(U(6, 2, 4).m) + NL
Expected = Expected + STR$(ExpectedM3D) + NL


' Preserve logic

REDIM PreserveCheck(5 TO 10) AS LONG
PreserveCheck(5) = 123

REDIM _PRESERVE PreserveCheck(20 TO 40) AS LONG

Actual = Actual + STR$(PreserveCheck(20)) + NL
Expected = Expected + STR$(ExpectedPreserveValue) + NL


IF Actual = Expected THEN
    PRINT "PASS"
ELSE
    PRINT "FAIL"
END IF

SYSTEM