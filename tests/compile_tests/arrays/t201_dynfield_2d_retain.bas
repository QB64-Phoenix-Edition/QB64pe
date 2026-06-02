$Console:Only

Type GridT
    cells(1, 1) _DynamicField As Long
End Type

Dim failText As String
ReDim grid(0 To 0) As GridT

ReDim grid(0).cells(0 To 1, 0 To 1)
grid(0).cells(0, 0) = 11
grid(0).cells(0, 1) = 12
grid(0).cells(1, 0) = 21
grid(0).cells(1, 1) = 22

ReDim _Retain grid(0).cells(0 To 2, 0 To 2)
grid(0).cells(2, 2) = 33

If LBound(grid(0).cells, 1) <> 0 And failText = "" Then failText = "first dimension lower bound"
If UBound(grid(0).cells, 1) <> 2 And failText = "" Then failText = "first dimension upper bound"
If LBound(grid(0).cells, 2) <> 0 And failText = "" Then failText = "second dimension lower bound"
If UBound(grid(0).cells, 2) <> 2 And failText = "" Then failText = "second dimension upper bound"

If grid(0).cells(0, 0) <> 11 And failText = "" Then failText = "retained cell 0,0"
If grid(0).cells(0, 1) <> 12 And failText = "" Then failText = "retained cell 0,1"
If grid(0).cells(1, 0) <> 21 And failText = "" Then failText = "retained cell 1,0"
If grid(0).cells(1, 1) <> 22 And failText = "" Then failText = "retained cell 1,1"
If grid(0).cells(2, 2) <> 33 And failText = "" Then failText = "new retained range write"

If failText = "" Then
    Print "PASS t201_dynfield_2d_retain"
Else
    Print "FAIL t201_dynfield_2d_retain: "; failText
End If
System
