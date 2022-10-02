$NoPrefix
$CONSOLE:ONLY

Dim m(3) As _Mem

Dim s(3) As String
Dim s2(3) As String * 8

Dim b(3) As _Bit
Dim b1(3) As Bit
Dim b2(3) As _Unsigned _Bit
Dim b3(3) As Unsigned _Bit
Dim b4(3) As _Unsigned Bit
Dim b5(3) As Unsigned Bit

Dim bn(3) As _Bit * 7
Dim bn1(3) As Bit * 7
Dim bn2(3) As _Unsigned _Bit * 7
Dim bn3(3) As Unsigned _Bit * 7
Dim bn4(3) As _Unsigned Bit * 7
Dim bn5(3) As Unsigned Bit * 7

Dim byt(3) As _Byte
Dim byt1(3) As Byte
Dim byt2(3) As _Unsigned _Byte
Dim byt3(3) As Unsigned _Byte
Dim byt4(3) As _Unsigned Byte
Dim byt5(3) As Unsigned Byte

Dim i64(3) As _Integer64
Dim i64_1(3) As Integer64
Dim i64_2(3) As _Unsigned _Integer64
Dim i64_3(3) As Unsigned _Integer64
Dim i64_4(3) As _Unsigned Integer64
Dim i64_5(3) As Unsigned Integer64

Dim o(3) As _Offset
Dim o1(3) As Offset
Dim o2(3) As _Unsigned _Offset
Dim o3(3) As Unsigned _Offset
Dim o4(3) As _Unsigned Offset
Dim o5(3) As Unsigned Offset

Dim i(3) As Integer
Dim i2(3) As _Unsigned Integer
Dim i3(3) As Unsigned Integer

Dim l(3) As Long
Dim l2(3) As _Unsigned Long
Dim l3(3) As Unsigned Long

Dim si(3) As Single
Dim d(3) As Double
Dim f(3) As _Float

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

SYSTEM
