$Console:Only

Type ClearT
    nums(2) _Dynamic As Long
End Type

Dim failText As String
ReDim work(0 To 0) As ClearT

ReDim work(0).nums(0 To 2)
work(0).nums(0) = 70
work(0).nums(1) = 71
work(0).nums(2) = 72

Erase work

ReDim work(0 To 0) As ClearT
ReDim work(0).nums(4 To 6)
work(0).nums(4) = 84
work(0).nums(5) = 85
work(0).nums(6) = 86

If LBound(work(0).nums) <> 4 And failText = "" Then failText = "reused lower bound"
If UBound(work(0).nums) <> 6 And failText = "" Then failText = "reused upper bound"
If work(0).nums(4) <> 84 And failText = "" Then failText = "reused low payload"
If work(0).nums(5) <> 85 And failText = "" Then failText = "reused middle payload"
If work(0).nums(6) <> 86 And failText = "" Then failText = "reused high payload"

If failText = "" Then
    Print "PASS t204_dynfield_erase_reuse"
Else
    Print "FAIL t204_dynfield_erase_reuse: "; failText
End If
System
