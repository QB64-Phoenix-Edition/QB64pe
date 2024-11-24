$CONSOLE:ONLY

OPTION _EXPLICIT

CONST TEXT_HELLO = "hello"
CONST TEXT_WORLD = "world"

TYPE t
    f AS SINGLE
    i AS _OFFSET
END TYPE

DIM f AS SINGLE: f = 3.141592653589793239
DIM i AS _OFFSET: i = 255
DIM s AS STRING: s = TEXT_HELLO

DIM t AS t
t.f = f
t.i = i

DIM a(1) AS SINGLE
a(0) = f
a(1) = _IIF(_FALSE, 0, i) ' naughty!

PRINT _IIF(f > 0, f, i)
PRINT _IIF(i > 0, i, f)

PRINT _IIF(t.f > 0, t.f, t.i)
PRINT _IIF(t.i > 0, t.i, t.f)

PRINT _IIF(a(0) > 0, a(0), a(1))
PRINT _IIF(a(1) > 0, a(1), a(0))

PRINT _IIF(f > 0, 1.1!, 2)
PRINT _IIF(i > 0, 2, 1.1!)

PRINT _IIF(f = 0, "f", "i")
PRINT _IIF(i = 0, "i", "f")

' PRINT and LPRINT has bugs and throws error when using string comparisons
WRITE _IIF(s = TEXT_HELLO, TEXT_HELLO, TEXT_WORLD)
WRITE _IIF(s = TEXT_WORLD, 1, 2)

PRINT _IIF(i > 0, foo, bar)
PRINT _IIF(i < 0, bar, foo)

DIM age AS _BYTE: age = 17
PRINT _IIF(age >= 18, "over 18", "under 18")

SYSTEM

FUNCTION foo&
    PRINT "foo called!"
    foo = 512
END FUNCTION

FUNCTION bar&
    PRINT "bar called!"
    bar = 128
END FUNCTION
