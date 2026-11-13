$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicExpressionArrayData
    values(0 To 0) _Dynamic As Long
    labels(0 To 0) _Dynamic As String * 8
End Type

Dim numberValue As Long
Dim labelValue As String

numberValue = MakeDynamicExpressionArrays(730).values(3)
labelValue = RTrim$(MakeDynamicExpressionArrays(731).labels(-1))

If numberValue <> 733 Then
    Print "FAIL direct dynamic numeric array member"
    System 1
End If
If labelValue <> "L731" Then
    Print "FAIL direct dynamic fixed STRING array member"
    System 1
End If

Print "PASS Func_return_UDT_t073"
System 0

Function MakeDynamicExpressionArrays (seed As Long) As DynamicExpressionArrayData
    Dim functionResult As DynamicExpressionArrayData

    ReDim functionResult.values(2 To 4)
    functionResult.values(2) = seed + 2
    functionResult.values(3) = seed + 3
    functionResult.values(4) = seed + 4
    ReDim functionResult.labels(-1 To 1)
    functionResult.labels(-1) = "L" + _Trim$(Str$(seed))
    functionResult.labels(0) = "CENTER"
    functionResult.labels(1) = "RIGHT"
    MakeDynamicExpressionArrays = functionResult
End Function
