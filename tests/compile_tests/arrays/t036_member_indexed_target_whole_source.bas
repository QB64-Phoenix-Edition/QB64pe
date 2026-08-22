$Console:Only
$Unstable:TypeFields
Type MemberBox
    values(0 To 2) _Static As Long
End Type
Dim boxA As MemberBox
Dim boxB As MemberBox
Dim scalarVal As Long

boxA.values(1) = boxB.values()
