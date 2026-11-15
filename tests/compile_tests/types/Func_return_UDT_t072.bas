$Console:Only
$Unstable:TypeFields
Option _Explicit

Type InlineExpressionArrayData
    numbers(-1 To 1) As Long
    names(1 To 2) _Static As String
    codes(0 To 1) _Static As String * 6
End Type

Dim numberValue As Long
Dim nameValue As String
Dim codeValue As String

numberValue = MakeInlineExpressionArrays(720).numbers(0)
nameValue = MakeInlineExpressionArrays(721).names(2)
codeValue = RTrim$(MakeInlineExpressionArrays(722).codes(1))

If numberValue <> 720 Then
    Print "FAIL direct inline numeric array member"
    System 1
End If
If nameValue <> "name-721" Then
    Print "FAIL direct static variable STRING array member"
    System 1
End If
If codeValue <> "C722" Then
    Print "FAIL direct static fixed STRING array member"
    System 1
End If

Print "PASS Func_return_UDT_t072"
System 0

Function MakeInlineExpressionArrays (seed As Long) As InlineExpressionArrayData
    Dim functionResult As InlineExpressionArrayData

    functionResult.numbers(-1) = seed - 1
    functionResult.numbers(0) = seed
    functionResult.numbers(1) = seed + 1
    functionResult.names(1) = "first"
    functionResult.names(2) = "name-" + _Trim$(Str$(seed))
    functionResult.codes(0) = "ZERO"
    functionResult.codes(1) = "C" + _Trim$(Str$(seed))
    MakeInlineExpressionArrays = functionResult
End Function
