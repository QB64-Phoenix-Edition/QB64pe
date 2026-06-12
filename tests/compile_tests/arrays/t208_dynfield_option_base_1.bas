$Console:Only
$Unstable:TypeFields

Option Base 1

Type Ob1T
    nums(3) _Dynamic As Long
End Type

Dim failText As String
ReDim work(1 To 2) As Ob1T

ReDim work(1).nums(1 To 3)
ReDim work(2).nums(4 To 6)

work(1).nums(1) = 11
work(1).nums(2) = 12
work(1).nums(3) = 13
work(2).nums(4) = 24
work(2).nums(5) = 25
work(2).nums(6) = 26

If LBound(work) <> 1 And failText = "" Then failText = "parent lower bound"
If UBound(work) <> 2 And failText = "" Then failText = "parent upper bound"
If LBound(work(1).nums) <> 1 And failText = "" Then failText = "member one lower bound"
If UBound(work(1).nums) <> 3 And failText = "" Then failText = "member one upper bound"
If LBound(work(2).nums) <> 4 And failText = "" Then failText = "member two lower bound"
If UBound(work(2).nums) <> 6 And failText = "" Then failText = "member two upper bound"
If work(1).nums(3) <> 13 And failText = "" Then failText = "member one payload"
If work(2).nums(4) <> 24 And failText = "" Then failText = "member two payload"

If failText = "" Then
    Print "PASS t208_dynfield_option_base_1"
Else
    Print "FAIL t208_dynfield_option_base_1: "; failText
End If
System
