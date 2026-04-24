$CONSOLE:ONLY

TYPE test
    n AS STRING
    m AS LONG
    l AS STRING * 8
END TYPE

REDIM T(5) AS test
T(1).n = "dynamic"
T(1).m = 120000
T(1).l = "Qbasic"

FOR f = 1 TO 1000
    REDIM _PRESERVE T(5 + f) AS test
NEXT f

PRINT T(1).n
PRINT STR$(T(1).m)
PRINT T(1).l
PRINT STR$(UBOUND(T))

REDIM _RETAIN T(5) AS test
PRINT T(1).n
PRINT STR$(T(1).m)
PRINT T(1).l
PRINT STR$(UBOUND(T))

FOR f = 1 TO 1000
    REDIM _RETAIN T(5 + f) AS test
NEXT f

PRINT T(1).n
PRINT STR$(T(1).m)
PRINT T(1).l
PRINT STR$(UBOUND(T))

REDIM U(10, 100, 10) AS test

U(1, 2, 3).n = "3D test dynamic"
U(6, 5, 4).l = "fixed"
U(6, 2, 4).m = 199201

FOR a = 10 TO 6 STEP -1
    FOR b = 5 TO 6
        FOR c = 7 TO 4 STEP -1
            REDIM _RETAIN U(a, b, c) AS test
NEXT c, b, a


PRINT U(1, 2, 3).n
PRINT U(6, 5, 4).l
PRINT STR$(U(6, 2, 4).m)

'preserve "logic"

REDIM a(5 TO 10)
a(5) = 123
REDIM _PRESERVE a(20 TO 40)
PRINT a(20)
SYSTEM

