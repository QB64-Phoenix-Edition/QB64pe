$Console:Only

Type LeafT
    title As String
    nums(1) _Dynamic As Long
    tailText As String
End Type

Type TreeT
    leaves(1) _Dynamic As LeafT
End Type

Dim failText As String
ReDim tree(0 To 0) As TreeT

ReDim tree(0).leaves(0 To 1)

tree(0).leaves(0).title = "leaf-zero"
tree(0).leaves(0).tailText = "tail-zero"
ReDim tree(0).leaves(0).nums(0 To 2)
tree(0).leaves(0).nums(0) = 1000
tree(0).leaves(0).nums(1) = 1001
tree(0).leaves(0).nums(2) = 1002

tree(0).leaves(1).title = "leaf-one"
tree(0).leaves(1).tailText = "tail-one"
ReDim tree(0).leaves(1).nums(5 To 6)
tree(0).leaves(1).nums(5) = 2005
tree(0).leaves(1).nums(6) = 2006

If tree(0).leaves(0).title <> "leaf-zero" And failText = "" Then failText = "nested title zero"
If tree(0).leaves(0).tailText <> "tail-zero" And failText = "" Then failText = "nested tail zero"
If LBound(tree(0).leaves(0).nums) <> 0 And failText = "" Then failText = "nested nums zero lower bound"
If UBound(tree(0).leaves(0).nums) <> 2 And failText = "" Then failText = "nested nums zero upper bound"
If tree(0).leaves(0).nums(2) <> 1002 And failText = "" Then failText = "nested nums zero payload"

If tree(0).leaves(1).title <> "leaf-one" And failText = "" Then failText = "nested title one"
If tree(0).leaves(1).tailText <> "tail-one" And failText = "" Then failText = "nested tail one"
If LBound(tree(0).leaves(1).nums) <> 5 And failText = "" Then failText = "nested nums one lower bound"
If UBound(tree(0).leaves(1).nums) <> 6 And failText = "" Then failText = "nested nums one upper bound"
If tree(0).leaves(1).nums(6) <> 2006 And failText = "" Then failText = "nested nums one payload"

If failText = "" Then
    Print "PASS t206_dynfield_nested_udt_owner"
Else
    Print "FAIL t206_dynfield_nested_udt_owner: "; failText
End If
System
