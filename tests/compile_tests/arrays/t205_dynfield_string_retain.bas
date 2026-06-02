$Console:Only

Type TextPackT
    names(1) _DynamicField As String
    marker As Long
End Type

Dim failText As String
ReDim pack(0 To 0) As TextPackT

pack(0).marker = 77
ReDim pack(0).names(0 To 2)
pack(0).names(0) = "alpha"
pack(0).names(1) = ""
pack(0).names(2) = "gamma-gamma"

ReDim _Retain pack(0).names(0 To 3)
pack(0).names(3) = "delta"

If pack(0).marker <> 77 And failText = "" Then failText = "scalar member changed"
If LBound(pack(0).names) <> 0 And failText = "" Then failText = "string lower bound"
If UBound(pack(0).names) <> 3 And failText = "" Then failText = "string upper bound"
If pack(0).names(0) <> "alpha" And failText = "" Then failText = "string retained first"
If pack(0).names(1) <> "" And failText = "" Then failText = "empty string retained"
If pack(0).names(2) <> "gamma-gamma" And failText = "" Then failText = "string retained last old"
If pack(0).names(3) <> "delta" And failText = "" Then failText = "string new slot"

If failText = "" Then
    Print "PASS t205_dynfield_string_retain"
Else
    Print "FAIL t205_dynfield_string_retain: "; failText
End If
System
