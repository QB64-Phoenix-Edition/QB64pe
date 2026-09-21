$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarLocalData
    values(0 To 0) _Dynamic As String
End Type

Dim result As DynVarLocalData

result = MakeDynVarLocal

If LBound(result.values) <> -2 Or UBound(result.values) <> 0 Then
    Print "FAIL local dynamic varstring bounds"
    System 1
End If
If result.values(-2) <> "red" Or result.values(-1) <> "green" Or result.values(0) <> "blue" Then
    Print "FAIL local dynamic varstring deep copy"
    System 1
End If

Print "PASS Func_return_UDT_t051"
System 0

Function MakeDynVarLocal As DynVarLocalData
    Dim functionResult As DynVarLocalData

    Dim temp As DynVarLocalData

    ReDim temp.values(-2 To 0)
    temp.values(-2) = "red"
    temp.values(-1) = "green"
    temp.values(0) = "blue"

    functionResult = temp

    ' The hidden result must own independent qbs payloads and a descriptor clone.
    temp.values(-2) = "mutated"
    temp.values(-1) = String$(256, "x")
    ReDim temp.values(7 To 8)
    temp.values(7) = "replacement"
    MakeDynVarLocal = functionResult
End Function
