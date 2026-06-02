$Console:Only

Type BucketT
    nums(1) _DynamicField As Long
End Type

Dim failText As String
ReDim work(0 To 1) As BucketT

ReDim work(0).nums(0 To 1)
ReDim work(1).nums(5 To 6)

work(0).nums(0) = 100
work(0).nums(1) = 101
work(1).nums(5) = 205
work(1).nums(6) = 206

ReDim _Retain work(0 To 2) As BucketT
ReDim work(2).nums(10 To 12)
work(2).nums(10) = 310
work(2).nums(11) = 311
work(2).nums(12) = 312

If work(0).nums(0) <> 100 And failText = "" Then failText = "work(0) retained low"
If work(0).nums(1) <> 101 And failText = "" Then failText = "work(0) retained high"
If LBound(work(1).nums) <> 5 And failText = "" Then failText = "work(1) retained lower bound"
If UBound(work(1).nums) <> 6 And failText = "" Then failText = "work(1) retained upper bound"
If work(1).nums(5) <> 205 And failText = "" Then failText = "work(1) retained low"
If work(1).nums(6) <> 206 And failText = "" Then failText = "work(1) retained high"
If work(2).nums(12) <> 312 And failText = "" Then failText = "new parent element descriptor"

If failText = "" Then
    Print "PASS t202_dynfield_parent_retain"
Else
    Print "FAIL t202_dynfield_parent_retain: "; failText
End If
System
