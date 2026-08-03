$Console:Only
$Unstable:TypeFields
Type MemberBox
    scalarValue As Long
    values(0 To 2) _Static As Long
End Type
Dim targetBox As MemberBox
Dim sourceBox As MemberBox

targetBox.scalarValue = sourceBox.values()
