$Console:Only

Type RecT
    id As Long
    vals(2) _DynamicField As Long
End Type

Dim failText As String
ReDim rec(0 To 2) As RecT

ReDim rec(0).vals(0 To 2)
ReDim rec(1).vals(0 To 4)
ReDim rec(2).vals(2 To 5)

rec(0).id = 10
rec(1).id = 20
rec(2).id = 30

rec(0).vals(0) = 101
rec(0).vals(1) = 102
rec(0).vals(2) = 103

rec(1).vals(0) = 201
rec(1).vals(1) = 202
rec(1).vals(2) = 203
rec(1).vals(3) = 204
rec(1).vals(4) = 205

rec(2).vals(2) = 302
rec(2).vals(3) = 303
rec(2).vals(4) = 304
rec(2).vals(5) = 305

If LBound(rec(0).vals) <> 0 And failText = "" Then failText = "rec(0) lower bound"
If UBound(rec(0).vals) <> 2 And failText = "" Then failText = "rec(0) upper bound"
If LBound(rec(1).vals) <> 0 And failText = "" Then failText = "rec(1) lower bound"
If UBound(rec(1).vals) <> 4 And failText = "" Then failText = "rec(1) upper bound"
If LBound(rec(2).vals) <> 2 And failText = "" Then failText = "rec(2) lower bound"
If UBound(rec(2).vals) <> 5 And failText = "" Then failText = "rec(2) upper bound"

If rec(0).vals(2) <> 103 And failText = "" Then failText = "rec(0) payload"
If rec(1).vals(4) <> 205 And failText = "" Then failText = "rec(1) payload"
If rec(2).vals(2) <> 302 And failText = "" Then failText = "rec(2) payload low"
If rec(2).vals(5) <> 305 And failText = "" Then failText = "rec(2) payload high"

If failText = "" Then
    Print "PASS t200_dynfield_numeric_per_parent"
Else
    Print "FAIL t200_dynfield_numeric_per_parent: "; failText
End If
System
