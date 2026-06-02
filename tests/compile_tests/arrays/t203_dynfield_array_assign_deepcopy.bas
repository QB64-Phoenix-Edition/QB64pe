$Console:Only

Type DataT
    values(2) _DynamicField As Long
End Type

Dim failText As String
ReDim src(0 To 1) As DataT
ReDim dst(0 To 1) As DataT

ReDim src(0).values(0 To 2)
ReDim src(1).values(3 To 5)

src(0).values(0) = 10
src(0).values(1) = 11
src(0).values(2) = 12
src(1).values(3) = 23
src(1).values(4) = 24
src(1).values(5) = 25

dst() = src()

src(0).values(1) = 9001
src(1).values(4) = 9002

If LBound(dst(0).values) <> 0 And failText = "" Then failText = "dst(0) lower bound"
If UBound(dst(0).values) <> 2 And failText = "" Then failText = "dst(0) upper bound"
If LBound(dst(1).values) <> 3 And failText = "" Then failText = "dst(1) lower bound"
If UBound(dst(1).values) <> 5 And failText = "" Then failText = "dst(1) upper bound"
If dst(0).values(0) <> 10 And failText = "" Then failText = "dst(0) copied low"
If dst(0).values(1) <> 11 And failText = "" Then failText = "dst(0) deep copy isolation"
If dst(0).values(2) <> 12 And failText = "" Then failText = "dst(0) copied high"
If dst(1).values(3) <> 23 And failText = "" Then failText = "dst(1) copied low"
If dst(1).values(4) <> 24 And failText = "" Then failText = "dst(1) deep copy isolation"
If dst(1).values(5) <> 25 And failText = "" Then failText = "dst(1) copied high"

If failText = "" Then
    Print "PASS t203_dynfield_array_assign_deepcopy"
Else
    Print "FAIL t203_dynfield_array_assign_deepcopy: "; failText
End If
System
