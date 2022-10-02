$NoPrefix
$CONSOLE:ONLY

test

SYSTEM

Sub test ()
    Static m As _Mem

    Static s As String
    Static s2 As String * 8

    Static b As _Bit
    Static b1 As Bit
    Static b2 As _Unsigned _Bit
    Static b3 As Unsigned _Bit
    Static b4 As _Unsigned Bit
    Static b5 As Unsigned Bit

    Static bn As _Bit * 7
    Static bn1 As Bit * 7
    Static bn2 As _Unsigned _Bit * 7
    Static bn3 As Unsigned _Bit * 7
    Static bn4 As _Unsigned Bit * 7
    Static bn5 As Unsigned Bit * 7

    Static byt As _Byte
    Static byt1 As Byte
    Static byt2 As _Unsigned _Byte
    Static byt3 As Unsigned _Byte
    Static byt4 As _Unsigned Byte
    Static byt5 As Unsigned Byte

    Static i64 As _Integer64
    Static i64_1 As Integer64
    Static i64_2 As _Unsigned _Integer64
    Static i64_3 As Unsigned _Integer64
    Static i64_4 As _Unsigned Integer64
    Static i64_5 As Unsigned Integer64

    Static o As _Offset
    Static o1 As Offset
    Static o2 As _Unsigned _Offset
    Static o3 As Unsigned _Offset
    Static o4 As _Unsigned Offset
    Static o5 As Unsigned Offset

    Static i As Integer
    Static i2 As _Unsigned Integer
    Static i3 As Unsigned Integer

    Static l As Long
    Static l2 As _Unsigned Long
    Static l3 As Unsigned Long

    Static si As Single
    Static d As Double
    Static f As _Float

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
End Sub
