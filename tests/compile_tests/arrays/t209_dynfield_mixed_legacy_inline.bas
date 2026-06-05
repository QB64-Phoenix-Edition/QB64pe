$Console:Only

Type MixedT
    fixedNums(2) As Long
    dynNums(1) _Dynamic As Long
End Type

Dim failText As String
ReDim work(0 To 0) As MixedT

work(0).fixedNums(0) = 700
work(0).fixedNums(1) = 701
work(0).fixedNums(2) = 702

ReDim work(0).dynNums(10 To 11)
work(0).dynNums(10) = 810
work(0).dynNums(11) = 811

If work(0).fixedNums(0) <> 700 And failText = "" Then failText = "legacy inline low"
If work(0).fixedNums(1) <> 701 And failText = "" Then failText = "legacy inline middle"
If work(0).fixedNums(2) <> 702 And failText = "" Then failText = "legacy inline high"
If LBound(work(0).dynNums) <> 10 And failText = "" Then failText = "dynamic lower bound"
If UBound(work(0).dynNums) <> 11 And failText = "" Then failText = "dynamic upper bound"
If work(0).dynNums(10) <> 810 And failText = "" Then failText = "dynamic low"
If work(0).dynNums(11) <> 811 And failText = "" Then failText = "dynamic high"

If failText = "" Then
    Print "PASS t209_dynfield_mixed_legacy_inline"
Else
    Print "FAIL t209_dynfield_mixed_legacy_inline: "; failText
End If
System
