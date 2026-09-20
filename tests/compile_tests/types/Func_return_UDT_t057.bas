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
    Dim child As DynVarRecursiveData

    If level = 0 Then
        MakeDynVarRecursive.values(0) = "base"
        MakeDynVarRecursive.depth = 0
    Else
        child = MakeDynVarRecursive(level - 1)
        MakeDynVarRecursive = child
        MakeDynVarRecursive.values(0) = MakeDynVarRecursive.values(0) + "x"
        MakeDynVarRecursive.depth = level
    End If
End Function
