$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarLeafData
    values(0 To 0) _Dynamic As String
End Type

Type DynVarNestedData
    leaf As DynVarLeafData
    marker As Long
End Type

Dim result As DynVarNestedData

result = MakeDynVarNested

If LBound(result.leaf.values) <> 3 Or UBound(result.leaf.values) <> 4 Then
    Print "FAIL nested dynamic varstring bounds"
    System 1
End If
If result.leaf.values(3) <> "three" Or result.leaf.values(4) <> "four" Then
    Print "FAIL nested dynamic varstring payload"
    System 1
End If
If result.marker <> 158 Then
    Print "FAIL nested dynamic varstring marker"
    System 1
End If

Print "PASS Func_return_UDT_t053"
System 0

Function MakeDynVarNested As DynVarNestedData
    ReDim MakeDynVarNested.leaf.values(3 To 4)
    MakeDynVarNested.leaf.values(3) = "three"
    MakeDynVarNested.leaf.values(4) = "four"
    MakeDynVarNested.marker = 158
End Function
