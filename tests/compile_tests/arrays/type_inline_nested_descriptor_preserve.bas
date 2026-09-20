$Console:Only
$Unstable:TypeFields
Option _Explicit

Type InnerDesc
    Values(0 To 1) _Dynamic As Long
End Type

Type OuterDesc
    Items(0 To 3) As InnerDesc
End Type

Dim idx As Long
ReDim parentData(0 To 0) As OuterDesc

For idx = 0 To 3
    parentData(0).Items(idx).Values(0) = 100 + idx
    parentData(0).Items(idx).Values(1) = 200 + idx
Next idx

ReDim _Preserve parentData(0 To 1) As OuterDesc

For idx = 0 To 3
    If LBound(parentData(0).Items(idx).Values) <> 0 Then Print "FAIL type_inline_nested_descriptor_preserve": System
    If UBound(parentData(0).Items(idx).Values) <> 1 Then Print "FAIL type_inline_nested_descriptor_preserve": System
    If parentData(0).Items(idx).Values(0) <> 100 + idx Then Print "FAIL type_inline_nested_descriptor_preserve": System
    If parentData(0).Items(idx).Values(1) <> 200 + idx Then Print "FAIL type_inline_nested_descriptor_preserve": System
Next idx

' The new parent element must own independently initialized descriptors.
parentData(1).Items(3).Values(1) = 999
If parentData(0).Items(3).Values(1) <> 203 Then Print "FAIL type_inline_nested_descriptor_preserve": System

Print "PASS type_inline_nested_descriptor_preserve"
System
