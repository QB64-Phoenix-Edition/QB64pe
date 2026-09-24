$Console:Only
Option _Explicit

Type ScalarExpressionData
    number As Long
    fraction As Double
    code As String * 8
End Type

Dim Shared makeCalls As Long
Dim numberValue As Long
Dim fractionValue As Double
Dim codeValue As String

numberValue = MakeScalarExpression(123, 1.5, "one").number
fractionValue = MakeScalarExpression(456, 7.25, "two").fraction
codeValue = RTrim$(MakeScalarExpression(789, 9.5, "QB64").code)

If numberValue <> 123 Then
    Print "FAIL direct numeric member"
    System 1
End If
If Abs(fractionValue - 7.25) > .0000001 Then
    Print "FAIL direct floating member"
    System 1
End If
If codeValue <> "QB64" Then
    Print "FAIL direct fixed STRING member"
    System 1
End If
If makeCalls <> 3 Then
    Print "FAIL scalar call count:"; makeCalls
    System 1
End If

Print "PASS Func_return_UDT_t070"
System 0

Function MakeScalarExpression (number As Long, fraction As Double, code As String) As ScalarExpressionData
    Dim functionResult As ScalarExpressionData

    makeCalls = makeCalls + 1
    functionResult.number = number
    functionResult.fraction = fraction
    functionResult.code = code
    MakeScalarExpression = functionResult
End Function
