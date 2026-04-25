$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE T
    Values(0 To 2) AS STRING * 5
END TYPE

DIM plain(0 TO 2) AS STRING * 5
DIM one AS T
DIM many(1) AS T
DIM i AS LONG, j AS LONG

plain(0) = "AB"
plain(1) = "XYZ"
plain(2) = "HELLO"

ERASE plain
FOR i = 0 TO 2
    FOR j = 1 TO 5
        IF ASC(plain(i), j) <> 0 THEN
            PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: ERASE plain mismatch at"; i; j
            SYSTEM
        END IF
    NEXT j
NEXT i

one.Values(0) = "AA"
one.Values(1) = "BB"
one.Values(2) = "CC"

ERASE one.Values
FOR i = 0 TO 2
    FOR j = 1 TO 5
        IF ASC(one.Values(i), j) <> 0 THEN
            PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: ERASE one.Values mismatch at"; i; j
            SYSTEM
        END IF
    NEXT j
NEXT i

one.Values(0) = "11"
one.Values(1) = "22"
one.Values(2) = "33"

REDIM one.Values(0 To 2)
FOR i = 0 TO 2
    FOR j = 1 TO 5
        IF ASC(one.Values(i), j) <> 0 THEN
            PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: REDIM one.Values mismatch at"; i; j
            SYSTEM
        END IF
    NEXT j
NEXT i

many(0).Values(1) = "AB"
many(1).Values(2) = "CD"

ERASE many(1).Values
FOR i = 0 TO 2
    FOR j = 1 TO 5
        IF ASC(many(1).Values(i), j) <> 0 THEN
            PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: ERASE many(1).Values mismatch at"; i; j
            SYSTEM
        END IF
    NEXT j
NEXT i

IF ASC(many(0).Values(1), 1) <> 65 OR ASC(many(0).Values(1), 2) <> 66 THEN
    PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: many(0) changed unexpectedly"
    SYSTEM
END IF

many(0).Values(0) = "X"
many(0).Values(2) = "Y"
many(1).Values(0) = "Z"

REDIM many(1).Values(0 To 2)
FOR i = 0 TO 2
    FOR j = 1 TO 5
        IF ASC(many(1).Values(i), j) <> 0 THEN
            PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: REDIM many(1).Values mismatch at"; i; j
            SYSTEM
        END IF
    NEXT j
NEXT i

IF ASC(many(0).Values(0), 1) <> 88 OR ASC(many(0).Values(2), 1) <> 89 THEN
    PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: many(0) changed unexpectedly after REDIM many(1).Values"
    SYSTEM
END IF

ERASE many
FOR i = 0 TO 1
    DIM k AS LONG
    FOR k = 0 TO 2
        FOR j = 1 TO 5
            IF ASC(many(i).Values(k), j) <> 0 THEN
                PRINT "FAIL 47_zero_fill_fixed_strings_erase_redim.bas: ERASE many mismatch at"; i; k; j
                SYSTEM
            END IF
        NEXT j
    NEXT k
NEXT i

PRINT "PASS 47_zero_fill_fixed_strings_erase_redim.bas"
SYSTEM
