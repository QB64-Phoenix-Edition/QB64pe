$Console:Only
Option _Explicit

Type T263
    nums(0 TO 1) _DynamicField As Long
    tagText As String * 5
End Type

Dim ok As _Byte
ReDim work(0 To 0) As T263

work(0).nums(0) = 100
work(0).nums(1) = 101
work(0).tagText = "A0000"

ReDim _Retain work(0 To 2) As T263
work(1).nums(0) = 200
work(1).nums(1) = 201
work(1).tagText = "B1111"
work(2).nums(0) = 300
work(2).tagText = "C2222"

ReDim _Retain work(0 To 1) As T263

ok = (LBound(work) = 0 And UBound(work) = 1)
ok = ok And (work(0).nums(0) = 100 And work(0).nums(1) = 101 And work(0).tagText = "A0000")
ok = ok And (work(1).nums(0) = 200 And work(1).nums(1) = 201 And work(1).tagText = "B1111")

ReDim _Retain work(0 To 3) As T263
work(3).nums(1) = 401
work(3).tagText = "D3333"

ok = ok And (LBound(work) = 0 And UBound(work) = 3)
ok = ok And (work(0).nums(0) = 100 And work(1).nums(1) = 201)
ok = ok And (work(2).nums(0) = 0 And work(2).nums(1) = 0 And work(2).tagText = String$(5, 0))
ok = ok And (work(3).nums(0) = 0 And work(3).nums(1) = 401 And work(3).tagText = "D3333")
If ok Then Print "PASS t263_retain_parent_multi_resize_with_member" Else Print "FAIL t263_retain_parent_multi_resize_with_member"

System

