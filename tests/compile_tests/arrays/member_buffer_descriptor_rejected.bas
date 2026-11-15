$Console:Only
$Unstable:TypeFields
Option _Explicit
Type InnerValue
    samples(0 To 0) _Dynamic As Long
End Type
Type OuterValue
    members(1 To 2) _Static As InnerValue
End Type
Dim owner As OuterValue
Dim fh As Long
Get #fh, , owner.members()
System
