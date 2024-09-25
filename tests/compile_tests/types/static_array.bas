$CONSOLE:ONLY

test

SYSTEM

SUB test ()
    STATIC m(3) AS _MEM

    STATIC s(3) AS STRING
    STATIC s2(3) AS STRING * 8

    STATIC b(3) AS _BIT
    STATIC b2(3) AS _UNSIGNED _BIT

    STATIC bn(3) AS _BIT * 7
    STATIC bn2(3) AS _UNSIGNED _BIT * 7

    STATIC byt(3) AS _BYTE
    STATIC byt2(3) AS _UNSIGNED _BYTE

    STATIC i64(3) AS _INTEGER64
    STATIC i64_2(3) AS _UNSIGNED _INTEGER64

    STATIC o(3) AS _OFFSET
    STATIC o2(3) AS _UNSIGNED _OFFSET

    STATIC i(3) AS INTEGER
    STATIC i2(3) AS _UNSIGNED INTEGER

    STATIC l(3) AS LONG
    STATIC l2(3) AS _UNSIGNED LONG

    STATIC si(3) AS SINGLE
    STATIC d(3) AS DOUBLE
    STATIC f(3) AS _FLOAT

    ' Just check a few of them
    s2(1) = "HI"
    b(1) = -1
    bn(1) = 20
    byt(1) = 50
    i64(1) = 23456
    i(1) = 70
    l(1) = 80
    si(1) = 2.2
    d(1) = 2.4
    f(1) = 2.8

    PRINT "TEST: "; s2(1); b(1); bn(1); byt(1); i64(1); i(1); l(1); si(1); d(1); f(1)
END SUB

