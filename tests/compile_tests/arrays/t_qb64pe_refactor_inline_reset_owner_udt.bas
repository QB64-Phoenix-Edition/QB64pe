$Console:Only
$Unstable:TypeFields
Option _Explicit

Type ResetInnerOwner
    labelText As String
    numberValue As Long
End Type

Type ResetOwnerLeaf
    names(0 To 2) As String
    innerValue As ResetInnerOwner
    numberValue As Long
End Type

Type ResetOwnerHolder
    leaves(0 To 8191) _Static As ResetOwnerLeaf
    markerValue As Long
End Type

Dim item As ResetOwnerHolder

item.markerValue = 777
item.leaves(0).names(0) = "zero"
item.leaves(4096).names(1) = "middle"
item.leaves(8191).names(2) = "last"
item.leaves(4096).innerValue.labelText = "nested"
item.leaves(4096).innerValue.numberValue = 1234
item.leaves(4096).numberValue = 5678

Erase item.leaves

If item.leaves(0).names(0) <> "" Then Print "FAIL erase owner first string": System 1
If item.leaves(4096).names(1) <> "" Then Print "FAIL erase owner middle string": System 1
If item.leaves(8191).names(2) <> "" Then Print "FAIL erase owner last string": System 1
If item.leaves(4096).innerValue.labelText <> "" Then Print "FAIL erase nested string": System 1
If item.leaves(4096).innerValue.numberValue <> 0 Then Print "FAIL erase nested numeric": System 1
If item.leaves(4096).numberValue <> 0 Then Print "FAIL erase leaf numeric": System 1
If item.markerValue <> 777 Then Print "FAIL erase owner sibling": System 1

item.leaves(0).names(0) = "again zero"
item.leaves(4096).names(1) = "again middle"
item.leaves(8191).names(2) = "again last"
item.leaves(4096).innerValue.labelText = "again nested"
item.leaves(4096).innerValue.numberValue = 4321
item.leaves(4096).numberValue = 8765

ReDim item.leaves(0 To 8191) As ResetOwnerLeaf

If item.leaves(0).names(0) <> "" Then Print "FAIL redim owner first string": System 1
If item.leaves(4096).names(1) <> "" Then Print "FAIL redim owner middle string": System 1
If item.leaves(8191).names(2) <> "" Then Print "FAIL redim owner last string": System 1
If item.leaves(4096).innerValue.labelText <> "" Then Print "FAIL redim nested string": System 1
If item.leaves(4096).innerValue.numberValue <> 0 Then Print "FAIL redim nested numeric": System 1
If item.leaves(4096).numberValue <> 0 Then Print "FAIL redim leaf numeric": System 1
If item.markerValue <> 777 Then Print "FAIL redim owner sibling": System 1
If LBound(item.leaves) <> 0 Or UBound(item.leaves) <> 8191 Then Print "FAIL redim owner bounds": System 1

Print "PASS t_qb64pe_refactor_inline_reset_owner_udt"
System
