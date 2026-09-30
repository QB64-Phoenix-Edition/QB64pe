$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarMixedData
    title As String
    values(0 To 1) _Dynamic As String
    tailText As String
End Type

Dim result As DynVarMixedData

result = MakeDynVarMixed("header", "tail")

If result.title <> "header" Or result.tailText <> "tail" Then
    Print "FAIL mixed owner scalar strings"
    System 1
End If
If LBound(result.values) <> 8 Or UBound(result.values) <> 9 Then
    Print "FAIL mixed owner dynamic bounds"
    System 1
End If
If result.values(8) <> "eight" Or result.values(9) <> "nine" Then
    Print "FAIL mixed owner dynamic strings"
    System 1
End If

Print "PASS Func_return_UDT_t054"
System 0

Function MakeDynVarMixed (headText As String, endText As String) As DynVarMixedData
    Dim functionResult As DynVarMixedData

    functionResult.title = headText
    ReDim functionResult.values(8 To 9)
    functionResult.values(8) = "eight"
    functionResult.values(9) = "nine"
    functionResult.tailText = endText
    MakeDynVarMixed = functionResult
End Function
