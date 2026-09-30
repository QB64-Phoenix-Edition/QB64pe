$Console:Only
Option _Explicit

Type InnerData
    value As Long
    code As String * 6
End Type

Type OuterData
    id As Long
    item As InnerData
    amount As Double
End Type

Dim result As OuterData
result = MakeOuter(17, -42, "QB64", 12.5)

If result.id <> 17 Then
    Print "FAIL outer id:"; result.id
    System 1
End If
If result.item.value <> -42 Then
    Print "FAIL nested value:"; result.item.value
    System 1
End If
If result.item.code <> "QB64  " Then
    Print "FAIL nested fixed string: ["; result.item.code; "]"
    System 1
End If
If Abs(result.amount - 12.5) > .0000001 Then
    Print "FAIL outer amount:"; result.amount
    System 1
End If

Print "PASS Func_return_UDT_t010"
System 0

Function MakeOuter (id As Long, value As Long, code As String, amount As Double) As OuterData
    Dim functionResult As OuterData

    functionResult.id = id
    functionResult.item.value = value
    functionResult.item.code = code
    functionResult.amount = amount
    MakeOuter = functionResult
End Function
