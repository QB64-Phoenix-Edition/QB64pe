$CONSOLE:ONLY

test

SYSTEM

SUB test ()
    STATIC m AS _MEM

    STATIC s AS STRING
    STATIC s2 AS STRING * 8

    STATIC b AS _BIT
    STATIC b2 AS _UNSIGNED _BIT

    STATIC bn AS _BIT * 7
    STATIC bn2 AS _UNSIGNED _BIT * 7

    STATIC byt AS _BYTE
    STATIC byt2 AS _UNSIGNED _BYTE

    STATIC i64 AS _INTEGER64
    STATIC i64_2 AS _UNSIGNED _INTEGER64

    STATIC o AS _OFFSET
    STATIC o2 AS _UNSIGNED _OFFSET

    STATIC i AS INTEGER
    STATIC i2 AS _UNSIGNED INTEGER

    STATIC l AS LONG
    STATIC l2 AS _UNSIGNED LONG

    STATIC si AS SINGLE
    STATIC d AS DOUBLE
    STATIC f AS _FLOAT

    ' Just check a few of them
    s2 = "HI"
    b = -1
    bn = 20
    byt = 50
    i64 = 23456
    i = 70
    l = 80
    si = 2.2
    d = 2.4
    f = 2.8

    PRINT "TEST: "; s2; b; bn; byt; i64; i; l; si; d; f
END SUB

