$Console:Only

'Test assignment to and size of UDT with numeric values
Type num_t
    b As _Byte
    ub As _Unsigned _Byte
    i As Integer
    ui As _Unsigned Integer
    l As Long
    ul As _Unsigned Long
    i64 As _Integer64
    ui64 As _Unsigned _Integer64
    o As _Offset
    uo As _Unsigned _Offset
    s As Single
    d As Double
    f As _Float
End Type
Dim num As num_t
num.b = -100
num.ub = 200
num.i = -12345
num.ui = 54321
num.l = -1234567
num.ul = 7654321
num.i64 = -123412341234
num.ui64 = 432143214321
num.o = -1
num.uo = 1
num.s = 3.5
num.d = -1.25
num.f = 10.125
expected_size = Len(x%%) + Len(x~%%) + Len(x%) + Len(x~%) + Len(x&) + Len(x~&) + Len(x&&) + Len(x~&&) + Len(x%&) + Len(x~%&) + Len(x!) + Len(x#) + Len(x##)
Print "NUM VALUES: "; num.b; num.ub; num.i; num.ui; num.l; num.ul; num.i64; num.ui64; num.o; num.uo; num.s; num.d; num.f
Print "NUM SIZE: "; expected_size - Len(num)


'Test copying between UDT instances
Dim num2 As num_t
num2 = num
Print "NUM2 VALUES: "; num2.b; num2.ub; num2.i; num2.ui; num2.l; num2.ul; num2.i64; num2.ui64; num2.o; num2.uo; num2.s; num2.d; num2.f


'Test fixed length string in UDT is initialised to NUL
Type fstr_t
    a As Long
    s As String * 10
    b As Long
End Type
Dim fstr As fstr_t
fstr.a = 1000
fstr.b = -6666
Print "FSTR UNINIT: ";
For i = 1 To 10
    Print Asc(fstr.s, i);
Next i
Print


'Test assignment to fixed length string in UDT
fstr.s = "hello"
Print "FSTR: "; fstr.a; "["; fstr.s; "] "; fstr.b


'Test variable length string in UDT is initialised to 0 length
Type vstr_t
    a As Long
    s As String
    b As Long
End Type
Dim vstr As vstr_t
vstr.a = 1000
vstr.b = -6666
Print "VSTR LEN: "; Len(vstr.s)


'Test assignment to variable length string in UDT
vstr.s = "hello"
Print "VSTR: "; vstr.a; "["; vstr.s; "] "; vstr.b

System
