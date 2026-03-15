OPTION _EXPLICIT
OPTION BASE 1

TYPE T
    Values(3) AS LONG
END TYPE

REDIM reference(3) AS LONG
REDIM x(1) AS T 'x(1) -----> Option Base 1; x(0) -----> subscript out of range as expected
DIM i AS LONG

FOR i = 1 TO 3
    reference(i) = i + 100
    x(1).Values(i) = i + 100
NEXT i

REDIM _PRESERVE reference(3)
REDIM _PRESERVE x(1).Values(3)

FOR i = 1 TO 3
    IF x(1).Values(i) <> reference(i) THEN
        PRINT "FAIL 09_redim_preserve_same_bounds_option_base1_classic_udt.bas: index"; i; " expected "; reference(i); " got "; x(0).Values(i)
        SYSTEM
    END IF
NEXT i

PRINT "PASS 09_redim_preserve_same_bounds_option_base1_classic_udt.bas"

SYSTEM
