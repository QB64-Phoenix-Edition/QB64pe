$Console:Only
$Unstable:TypeFields

Type FixedMixT
    codes(2) _Static As String * 8
    values(1) _Dynamic As Long
End Type

Dim failText As String
ReDim work(0 To 0) As FixedMixT

work(0).codes(0) = "aa"
work(0).codes(1) = "bbbbbbbb"
work(0).codes(2) = "ccc"

ReDim work(0).values(0 To 1)
work(0).values(0) = 900
work(0).values(1) = 901

If RTrim$(work(0).codes(0)) <> "aa" And failText = "" Then failText = "fixed string low"
If work(0).codes(1) <> "bbbbbbbb" And failText = "" Then failText = "fixed string exact"
If RTrim$(work(0).codes(2)) <> "ccc" And failText = "" Then failText = "fixed string high"
If LBound(work(0).values) <> 0 And failText = "" Then failText = "dynamic values lower bound"
If UBound(work(0).values) <> 1 And failText = "" Then failText = "dynamic values upper bound"
If work(0).values(0) <> 900 And failText = "" Then failText = "dynamic values low"
If work(0).values(1) <> 901 And failText = "" Then failText = "dynamic values high"

If failText = "" Then
    Print "PASS t210_dynfield_static_fixed_string_mix"
Else
    Print "FAIL t210_dynfield_static_fixed_string_mix: "; failText
End If
System
