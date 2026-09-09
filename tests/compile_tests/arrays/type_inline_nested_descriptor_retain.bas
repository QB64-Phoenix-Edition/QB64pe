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
ReDim parentData(0 To 1) As OuterDesc

For idx = 0 To 3
    parentData(0).Items(idx).Values(0) = 300 + idx
    parentData(0).Items(idx).Values(1) = 400 + idx
Next idx

ReDim _Retain parentData(-1 To 0) As OuterDesc

For idx = 0 To 3
    If LBound(parentData(0).Items(idx).Values) <> 0 Then Print "FAIL type_inline_nested_descriptor_retain": System
    If UBound(parentData(0).Items(idx).Values) <> 1 Then Print "FAIL type_inline_nested_descriptor_retain": System
    If parentData(0).Items(idx).Values(0) <> 300 + idx Then Print "FAIL type_inline_nested_descriptor_retain": System
    If parentData(0).Items(idx).Values(1) <> 400 + idx Then Print "FAIL type_inline_nested_descriptor_retain": System
Next idx

' The newly introduced coordinate must have its own initialized descriptors.
parentData(-1).Items(0).Values(0) = 777
If parentData(0).Items(0).Values(0) <> 300 Then Print "FAIL type_inline_nested_descriptor_retain": System

Print "PASS type_inline_nested_descriptor_retain"
System
