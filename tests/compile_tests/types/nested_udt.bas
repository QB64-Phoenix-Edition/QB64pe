$Console:Only

Type t1
    a As String
End Type

Type t2
    b As t1
    s As String
End Type

Type t3
    n As Long
    c As t2
End Type

'Test nested variable length string in UDT variable
Dim test As t3
test.c.s = "Hello"
test.c.b.a = " world"
Print test.c.s; test.c.b.a


'Test nested variable length string in UDT array
Dim arr(3) as t3
For i = 0 to 3
    arr(2).c.b.a = Str$(i)
    Print arr(2).c.b.a;
Next i

System