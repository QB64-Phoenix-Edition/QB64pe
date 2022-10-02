$NoPrefix
$CONSOLE:ONLY

Dim m As _Mem

Dim s As String
Dim s2 As String * 8

Dim b As _Bit
Dim b1 As Bit
Dim b2 As _Unsigned _Bit
Dim b3 As Unsigned _Bit
Dim b4 As _Unsigned Bit
Dim b5 As Unsigned Bit

Dim bn As _Bit * 7
Dim bn1 As Bit * 7
Dim bn2 As _Unsigned _Bit * 7
Dim bn3 As Unsigned _Bit * 7
Dim bn4 As _Unsigned Bit * 7
Dim bn5 As Unsigned Bit * 7

Dim byt As _Byte
Dim byt1 As Byte
Dim byt2 As _Unsigned _Byte
Dim byt3 As Unsigned _Byte
Dim byt4 As _Unsigned Byte
Dim byt5 As Unsigned Byte

Dim i64 As _Integer64
Dim i64_1 As Integer64
Dim i64_2 As _Unsigned _Integer64
Dim i64_3 As Unsigned _Integer64
Dim i64_4 As _Unsigned Integer64
Dim i64_5 As Unsigned Integer64

Dim o As _Offset
Dim o1 As Offset
Dim o2 As _Unsigned _Offset
Dim o3 As Unsigned _Offset
Dim o4 As _Unsigned Offset
Dim o5 As Unsigned Offset

Dim i As Integer
Dim i2 As _Unsigned Integer
Dim i3 As Unsigned Integer

Dim l As Long
Dim l2 As _Unsigned Long
Dim l3 As Unsigned Long

Dim si As Single
Dim d As Double
Dim f As _Float

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

SYSTEM
