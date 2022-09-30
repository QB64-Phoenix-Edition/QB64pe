$NoPrefix
$CONSOLE:ONLY

test

SYSTEM

Sub test ()
    Static m(3) As _Mem

    Static s(3) As String
    Static s2(3) As String * 8

    Static b(3) As _Bit
    Static b1(3) As Bit
    Static b2(3) As _Unsigned _Bit
    Static b3(3) As Unsigned _Bit
    Static b4(3) As _Unsigned Bit
    Static b5(3) As Unsigned Bit

    Static bn(3) As _Bit * 7
    Static bn1(3) As Bit * 7
    Static bn2(3) As _Unsigned _Bit * 7
    Static bn3(3) As Unsigned _Bit * 7
    Static bn4(3) As _Unsigned Bit * 7
    Static bn5(3) As Unsigned Bit * 7

    Static byt(3) As _Byte
    Static byt1(3) As Byte
    Static byt2(3) As _Unsigned _Byte
    Static byt3(3) As Unsigned _Byte
    Static byt4(3) As _Unsigned Byte
    Static byt5(3) As Unsigned Byte

    Static i64(3) As _Integer64
    Static i64_1(3) As Integer64
    Static i64_2(3) As _Unsigned _Integer64
    Static i64_3(3) As Unsigned _Integer64
    Static i64_4(3) As _Unsigned Integer64
    Static i64_5(3) As Unsigned Integer64

    Static o(3) As _Offset
    Static o1(3) As Offset
    Static o2(3) As _Unsigned _Offset
    Static o3(3) As Unsigned _Offset
    Static o4(3) As _Unsigned Offset
    Static o5(3) As Unsigned Offset

    Static i(3) As Integer
    Static i2(3) As _Unsigned Integer
    Static i3(3) As Unsigned Integer

    Static l(3) As Long
    Static l2(3) As _Unsigned Long
    Static l3(3) As Unsigned Long

    Static si(3) As Single
    Static d(3) As Double
    Static f(3) As _Float

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
End Sub
