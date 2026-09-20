$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerNestedLeafData
    textValue As String
End Type

Type DynOwnerNestedMidData
    leaf As DynOwnerNestedLeafData
    notes(0 To 1) As String
End Type

Type DynOwnerNestedElemData
    middle As DynOwnerNestedMidData
    marker As Long
End Type

Type DynOwnerNestedResultData
    items(0 To 0) _Dynamic As DynOwnerNestedElemData
End Type

Dim result As DynOwnerNestedResultData

result = MakeDynOwnerNestedScalar

If LBound(result.items) <> 1 Or UBound(result.items) <> 2 Then
    Print "FAIL nested scalar owner bounds"
    System 1
End If
If result.items(1).middle.leaf.textValue <> "leaf-one" Then
    Print "FAIL nested scalar leaf one"
    System 1
End If
If result.items(1).middle.notes(0) <> "n10" Or result.items(1).middle.notes(1) <> "n11" Then
    Print "FAIL nested scalar notes one"
    System 1
End If
If result.items(2).middle.leaf.textValue <> "leaf-two" Then
    Print "FAIL nested scalar leaf two"
    System 1
End If
If result.items(2).middle.notes(0) <> "n20" Or result.items(2).middle.notes(1) <> "n21" Then
    Print "FAIL nested scalar notes two"
    System 1
End If
If result.items(1).marker <> 1691 Or result.items(2).marker <> 1692 Then
    Print "FAIL nested scalar markers"
    System 1
End If

Print "PASS Func_return_UDT_t062"
System 0

Function MakeDynOwnerNestedScalar As DynOwnerNestedResultData
    ReDim MakeDynOwnerNestedScalar.items(1 To 2)
    MakeDynOwnerNestedScalar.items(1).middle.leaf.textValue = "leaf-one"
    MakeDynOwnerNestedScalar.items(1).middle.notes(0) = "n10"
    MakeDynOwnerNestedScalar.items(1).middle.notes(1) = "n11"
    MakeDynOwnerNestedScalar.items(1).marker = 1691
    MakeDynOwnerNestedScalar.items(2).middle.leaf.textValue = "leaf-two"
    MakeDynOwnerNestedScalar.items(2).middle.notes(0) = "n20"
    MakeDynOwnerNestedScalar.items(2).middle.notes(1) = "n21"
    MakeDynOwnerNestedScalar.items(2).marker = 1692
End Function
