$Console:Only
$Unstable:TypeFields
Option _Explicit

Type LeafDesc
    Nums(0 To 2) _Dynamic As Long
End Type

Type MidDesc
    Leaves(0 To 1) _Static As LeafDesc
End Type

Type OuterDesc
    Groups(0 To 0) _Dynamic As MidDesc
End Type

Dim src As OuterDesc
Dim dst As OuterDesc

src.Groups(0).Leaves(0).Nums(0) = 101
src.Groups(0).Leaves(0).Nums(1) = 102
src.Groups(0).Leaves(1).Nums(0) = 201
src.Groups(0).Leaves(1).Nums(1) = 202

dst = src

If dst.Groups(0).Leaves(0).Nums(0) <> 101 Then Print "FAIL type_nested_descriptor_payload_clone": System
If dst.Groups(0).Leaves(0).Nums(1) <> 102 Then Print "FAIL type_nested_descriptor_payload_clone": System
If dst.Groups(0).Leaves(1).Nums(0) <> 201 Then Print "FAIL type_nested_descriptor_payload_clone": System
If dst.Groups(0).Leaves(1).Nums(1) <> 202 Then Print "FAIL type_nested_descriptor_payload_clone": System

' Both source and destination descriptor graphs must remain valid and independent.
dst.Groups(0).Leaves(1).Nums(0) = 999
If src.Groups(0).Leaves(1).Nums(0) <> 201 Then Print "FAIL type_nested_descriptor_payload_clone": System
src.Groups(0).Leaves(0).Nums(1) = 555
If dst.Groups(0).Leaves(0).Nums(1) <> 102 Then Print "FAIL type_nested_descriptor_payload_clone": System

Print "PASS type_nested_descriptor_payload_clone"
System
