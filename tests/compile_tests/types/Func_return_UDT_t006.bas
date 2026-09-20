$Console:Only
Option _Explicit

Type PairData
    a As Long
    b As Long
End Type

Declare Function MakeDeclared (a As Long, b As Long) As PairData

Dim result As PairData
result = MakeDeclared(31, 47)
If result.a <> 31 Or result.b <> 47 Then
    Print "FAIL DECLARE FUNCTION:"; result.a; result.b
    System 1
End If

Print "PASS Func_return_UDT_t006"
System 0

Function MakeDeclared (a As Long, b As Long) As PairData
    MakeDeclared.a = a
    MakeDeclared.b = b
End Function
