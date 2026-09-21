$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarBasicData
    values(0 To 1) _Dynamic As String
    marker As Long
End Type

Dim result As DynVarBasicData
Dim binaryValue As String

binaryValue = "A" + Chr$(0) + "B"
result = MakeDynVarBasic(binaryValue)

If LBound(result.values) <> -1 Or UBound(result.values) <> 1 Then
    Print "FAIL dynamic varstring bounds"
    System 1
End If
If result.values(-1) <> "alpha" Then
    Print "FAIL dynamic varstring alpha"
    System 1
End If
If result.values(0) <> binaryValue Then
    Print "FAIL dynamic varstring binary payload"
    System 1
End If
If result.values(1) <> "omega" Or result.marker <> 155 Then
    Print "FAIL dynamic varstring payload"
    System 1
End If

Print "PASS Func_return_UDT_t050"
System 0

Function MakeDynVarBasic (binaryText As String) As DynVarBasicData
    Dim functionResult As DynVarBasicData

    ReDim functionResult.values(-1 To 1)
    functionResult.values(-1) = "alpha"
    functionResult.values(0) = binaryText
    functionResult.values(1) = "omega"
    functionResult.marker = 155
    MakeDynVarBasic = functionResult
End Function
