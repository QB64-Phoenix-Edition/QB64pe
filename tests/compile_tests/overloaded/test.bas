OPTION _EXPLICIT
$CONSOLE:ONLY

' Most of tests are meant for testing few overloaded QB64-PE functions on ARM6 targets.
' However, these tests should not fail on other targets.

PRINT VAL("") ' 0
PRINT VAL("  42  ") ' 42
PRINT VAL("-999") ' -999"
PRINT VAL("bad123") ' 0
PRINT VAL("123bad") ' 123
PRINT VAL("123.456bad") ' 123.456
PRINT VAL("123") ' 123
PRINT VAL("123.456") ' 123.456
PRINT VAL("123.456.789") ' 123.456
PRINT VAL("123.456e2") ' 12345.6
PRINT VAL("123.456e-2") ' 1.23456
PRINT VAL("&H1A2B") ' 6699
PRINT VAL("&B11110000") ' 240
PRINT VAL("&O177") ' 127
PRINT VAL("1d-1") ' 0.1
PRINT VAL("1d2") ' 100
PRINT VAL("1D-1") ' 0.1
PRINT VAL("1D+2") ' 100
PRINT VAL("1e-1") ' 0.1
PRINT VAL("1e2") ' 100
PRINT VAL("1E-1") ' 0.1
PRINT VAL("1E+2") ' 100
PRINT VAL("1f-1") ' 0.1
PRINT VAL("1f2") ' 100
PRINT VAL("1F-1") ' 0.1
PRINT VAL("1F+2") ' 100
PRINT

' The following test the new behavior on ARM64 targets
PRINT VAL("18446744073709551615", _UNSIGNED _INTEGER64) ' 18446744073709551615
PRINT VAL("-9223372036854775808", _INTEGER64) ' -9223372036854775808
PRINT VAL("9223372036854775807", _INTEGER64) ' 9223372036854775807
PRINT VAL("-4611686018427387903", _INTEGER64) ' -4611686018427387903
PRINT VAL("4611686018427387903", _UNSIGNED _INTEGER64) ' 4611686018427387903
PRINT

' The following tests the new behavior for _MIN, _MAX and _CLAMP on ARM64 targets
PRINT _MIN(-9223372036854775808&&, 9223372036854775807&&) ' -9223372036854775808
PRINT _MAX(-9223372036854775808&&, 9223372036854775807&&) ' 9223372036854775807
PRINT _CLAMP(18446744073709551615~&&, 4611686018427387903&&, 0&&) ' 4611686018427387903
PRINT

' Different types of constants
CONST B%% = 101%%
CONST UB~%% = 201~%%
CONST I% = 32001%
CONST UI~% = 64001~%
CONST L& = 2000000001&
CONST UL~& = 4000000001~&
CONST I64&& = 8000000000000000001&&
CONST UI64~&& = 16000000000000000001~&&
CONST S! = 3141592E-6 ' 3.141592
CONST D# = 2718281D-6 ' 2.718281
CONST F## = 1618033F-6 ' 1.618033
CONST O&& = &O157255
CONST UO~&& = &O137357

TYPE Bar
    b AS _BYTE
    ub AS _UNSIGNED _BYTE
    i AS INTEGER
    ui AS _UNSIGNED INTEGER
    l AS LONG
    ul AS _UNSIGNED LONG
    i64 AS _INTEGER64
    ui64 AS _UNSIGNED _INTEGER64
    s AS SINGLE
    d AS DOUBLE
    f AS _FLOAT
    o AS _INTEGER64
    uo AS _UNSIGNED _INTEGER64
END TYPE

DIM Foo AS Bar
Foo.b = B%%
Foo.ub = UB~%%
Foo.i = I%
Foo.ui = UI~%
Foo.l = L&
Foo.ul = UL~&
Foo.i64 = I64&&
Foo.ui64 = UI64~&&
Foo.s = S!
Foo.d = D#
Foo.f = F##
Foo.o = O%&
Foo.uo = UO~%&

DIM Baz(0 TO 3) AS Bar
DIM Arr(0 TO 3) AS LONG

DIM n AS LONG
FOR n = 0 TO 3
    Arr(n) = n + 1
    Baz(n).b = B
    Baz(n).ub = UB
    Baz(n).i = I
    Baz(n).ui = UI
    Baz(n).l = L
    Baz(n).ul = UL
    Baz(n).i64 = I64
    Baz(n).ui64 = UI64
    Baz(n).s = S
    Baz(n).d = D
    Baz(n).f = F
    Baz(n).o = O
    Baz(n).uo = UO
NEXT

' These help us check if the underlying type promotion logic for _MIN, _MAX, and _CLAMP works correctly

' Constant cases
PRINT _MIN(B, UI) ' 101
PRINT _MIN(I64, L) ' 2000000001
PRINT _CLAMP(UI, UB, B) ' 201
PRINT _MIN(S, UI) ' 3.141592
PRINT _MAX(F, D) ' 2.718281
PRINT _CLAMP(F, S, D) '  2.718281
PRINT _MIN(UI, F) ' 1.618033
PRINT _MAX(F, B) ' 101
PRINT _CLAMP(F, B, S) ' 3.141592
PRINT

' Array of LONG cases
PRINT _MIN(Arr(0), Arr(1)) ' 1
PRINT _MIN(Arr(2), Arr(3)) ' 3
PRINT _CLAMP(Arr(0), Arr(1), Arr(2)) ' 2
PRINT

' UDT cases
PRINT _MIN(Foo.b, Foo.ub) ' 101
PRINT _MIN(Foo.i64, Foo.l) ' 2000000001
PRINT _CLAMP(Foo.ui, Foo.ub, Foo.b) ' 201
PRINT _MIN(Foo.s, Foo.ui) ' 3.141592
PRINT _MAX(Foo.f, Foo.d) ' 2.718281
PRINT _CLAMP(Foo.f, Foo.s, Foo.d) ' 2.718281
PRINT _MIN(Foo.ui, Foo.f) ' 1.618033
PRINT _MAX(Foo.f, Foo.b) ' 101
PRINT INT(_CLAMP(Foo.f, Foo.b, Foo.s)) ' 3
PRINT

' Array of UTD cases
PRINT _MIN(Baz(0).b, Baz(1).ub) ' 101
PRINT _MIN(Baz(2).i64, Baz(3).l) ' 2000000001
PRINT _CLAMP(Baz(0).ui, Baz(1).ub, Baz(2).b) ' 201
PRINT _MIN(Baz(3).s, Baz(0).ui) ' 3.141592
PRINT _MAX(Baz(1).f, Baz(2).d) ' 2.718281
PRINT _CLAMP(Baz(1).f, Baz(3).s, Baz(2).d) ' 2.718281
PRINT _MIN(Baz(0).ui, Baz(1).f) ' 1.618033
PRINT _MAX(Baz(1).f, Baz(2).b) ' 101
PRINT INT(_CLAMP(Baz(1).f, Baz(2).b, Baz(3).s)) ' 3
PRINT HEX$(_MAX(Baz(0).o, Baz(1).uo)) ' DEAD

SYSTEM
