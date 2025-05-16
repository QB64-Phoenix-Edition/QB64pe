$CONSOLE:ONLY

$IF WIN THEN

    $IF 32BIT THEN

        DECLARE STATIC LIBRARY "./lib-windows32"
            FUNCTION add_values&(BYVAL v1 AS LONG, BYVAL v2 as LONG)
        END DECLARE

    $ELSE

        $IF _ARM_ THEN

            DECLARE STATIC LIBRARY "./lib-windowsarm64"
                FUNCTION add_values& (BYVAL v1 AS LONG, BYVAL v2 AS LONG)
            END DECLARE

        $ELSE

            DECLARE STATIC LIBRARY "./lib-windows"
                FUNCTION add_values& (BYVAL v1 AS LONG, BYVAL v2 AS LONG)
            END DECLARE

        $END IF

    $END IF

$ELSEIF MAC THEN

    DECLARE STATIC LIBRARY "./lib-osx"
        FUNCTION add_values&(BYVAL v1 AS LONG, BYVAL v2 as LONG)
    END DECLARE

$ELSEIF LINUX THEN

    DECLARE STATIC LIBRARY "./lib-linux"
        FUNCTION add_values&(BYVAL v1 AS LONG, BYVAL v2 as LONG)
    END DECLARE

$END IF

result = add_values&(2, 3)
PRINT result
SYSTEM
