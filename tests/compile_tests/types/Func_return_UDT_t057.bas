$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarRecursiveData
    values(0 To 0) _Dynamic As String
    depth As Long
End Type

Dim result As DynVarRecursiveData

result = MakeDynVarRecursive(3)

If LBound(result.values) <> 0 Or UBound(result.values) <> 0 Then
    Print "FAIL recursive dynamic varstring bounds"
    System 1
End If
If result.values(0) <> "basexxx" Or result.depth <> 3 Then
    Print "FAIL recursive dynamic varstring payload"
    System 1
End If

Print "PASS Func_return_UDT_t057"
System 0

Function MakeDynVarRecursive (level As Long) As DynVarRecursiveData
    Dim functionResult As DynVarRecursiveData

    Dim child As DynVarRecursiveData

    If level = 0 Then
        functionResult.values(0) = "base"
        functionResult.depth = 0
    Else
        child = MakeDynVarRecursive(level - 1)
        functionResult = child
        functionResult.values(0) = functionResult.values(0) + "x"
        functionResult.depth = level
    End If
    MakeDynVarRecursive = functionResult
End Function
