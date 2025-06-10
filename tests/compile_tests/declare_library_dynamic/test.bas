$CONSOLE:ONLY

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

result = add_values&(2, 3)
PRINT result
SYSTEM
