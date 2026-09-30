$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarTwoMembersData
    namesA(0 To 0) _Dynamic As String
    marker As Long
    namesB(0 To 1) _Dynamic As String
End Type

Dim result As DynVarTwoMembersData

result = MakeDynVarTwoMembers

If LBound(result.namesA) <> 1 Or UBound(result.namesA) <> 2 Then
    Print "FAIL first descriptor bounds"
    System 1
End If
If LBound(result.namesB) <> -2 Or UBound(result.namesB) <> 0 Then
    Print "FAIL second descriptor bounds"
    System 1
End If
If result.namesA(1) <> "A1" Or result.namesA(2) <> "A2" Then
    Print "FAIL first descriptor payload"
    System 1
End If
If result.namesB(-2) <> "B-2" Or result.namesB(-1) <> "B-1" Or result.namesB(0) <> "B0" Then
    Print "FAIL second descriptor payload"
    System 1
End If
If result.marker <> 163 Then
    Print "FAIL two descriptor marker"
    System 1
End If

Print "PASS Func_return_UDT_t058"
System 0

Function MakeDynVarTwoMembers As DynVarTwoMembersData
    Dim functionResult As DynVarTwoMembersData

    ReDim functionResult.namesA(1 To 2)
    ReDim functionResult.namesB(-2 To 0)
    functionResult.namesA(1) = "A1"
    functionResult.namesA(2) = "A2"
    functionResult.namesB(-2) = "B-2"
    functionResult.namesB(-1) = "B-1"
    functionResult.namesB(0) = "B0"
    functionResult.marker = 163
    MakeDynVarTwoMembers = functionResult
End Function
