$Console:Only
Option _Explicit

Type PairData
    a As Long
    b As Long
End Type

Dim result As PairData
result = MakeStatic(9)
If result.a <> 9 Or result.b <> 81 Then
    Print "FAIL STATIC UDT return:"; result.a; result.b
    System 1
End If

Print "PASS Func_return_UDT_t009"
System 0

Function MakeStatic (value As Long) As PairData Static
    MakeStatic.a = value
    MakeStatic.b = value * value
End Function
