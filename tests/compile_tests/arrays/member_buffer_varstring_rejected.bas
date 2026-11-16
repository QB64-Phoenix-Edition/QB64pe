$Console:Only
$Unstable:TypeFields
Option _Explicit
Type OuterValue
    members(0 To 0) _Dynamic As String
End Type
Dim owner As OuterValue
Dim fh As Long
Get #fh, , owner.members()
System
