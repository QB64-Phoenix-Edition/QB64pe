$NoPrefix
$CONSOLE:ONLY

Dim dim_m As _Mem

Dim dim_s As String
Dim dim_s2 As String * 8

Dim dim_b As _Bit
Dim dim_b1 As Bit
Dim dim_b2 As _Unsigned _Bit
Dim dim_b3 As Unsigned _Bit
Dim dim_b4 As _Unsigned Bit
Dim dim_b5 As Unsigned Bit

Dim dim_bn As _Bit * 7
Dim dim_bn1 As Bit * 7
Dim dim_bn2 As _Unsigned _Bit * 7
Dim dim_bn3 As Unsigned _Bit * 7
Dim dim_bn4 As _Unsigned Bit * 7
Dim dim_bn5 As Unsigned Bit * 7

Dim dim_byte As _Byte
Dim dim_byte1 As Byte
Dim dim_byte2 As _Unsigned _Byte
Dim dim_byte3 As Unsigned _Byte
Dim dim_byte4 As _Unsigned Byte
Dim dim_byte5 As Unsigned Byte

Dim dim_i64 As _Integer64
Dim dim_i64_1 As Integer64
Dim dim_i64_2 As _Unsigned _Integer64
Dim dim_i64_3 As Unsigned _Integer64
Dim dim_i64_4 As _Unsigned Integer64
Dim dim_i64_5 As Unsigned Integer64

Dim dim_o As _Offset
Dim dim_o1 As Offset
Dim dim_o2 As _Unsigned _Offset
Dim dim_o3 As Unsigned _Offset
Dim dim_o4 As _Unsigned Offset
Dim dim_o5 As Unsigned Offset

Dim dim_i As Integer
Dim dim_i2 As _Unsigned Integer
Dim dim_i3 As Unsigned Integer

Dim dim_l As Long
Dim dim_l2 As _Unsigned Long
Dim dim_l3 As Unsigned Long

Dim dim_si As Single
Dim dim_d As Double
Dim dim_f As _Float

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
