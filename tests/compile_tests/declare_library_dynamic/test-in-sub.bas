$CONSOLE:ONLY

' A DYNAMIC library called from inside a SUB. QB64-PE declares the function
' pointer itself using only its own C types, and the dlopen/GetProcAddress code
' goes to the main module, so nothing here depends on a user header - this SUB
' can be compiled into its own object file.

$IF 32BIT THEN

    DECLARE DYNAMIC LIBRARY "./lib32"
        FUNCTION add_values&(BYVAL v1 AS LONG, BYVAL v2 as LONG)
    END DECLARE

$ELSE

    $IF _ARM_ THEN

        DECLARE DYNAMIC LIBRARY "./libarm64"
            FUNCTION add_values& (BYVAL v1 AS LONG, BYVAL v2 AS LONG)
        END DECLARE

    $ELSE

        DECLARE DYNAMIC LIBRARY "./lib"
            FUNCTION add_values& (BYVAL v1 AS LONG, BYVAL v2 AS LONG)
        END DECLARE

    $END IF

$END IF

PRINT AddThem&(2, 3)
SYSTEM

FUNCTION AddThem& (a AS LONG, b AS LONG)
    AddThem& = add_values&(a, b)
END FUNCTION
