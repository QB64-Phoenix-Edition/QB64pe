$Console:Only
$Unstable:TypeFields
Option _Explicit
Type OwnedValue
    textValue As String
End Type
Type OuterValue
    members(0 To 0) _Dynamic As OwnedValue
End Type
Dim owner As OuterValue
Dim fh As Long
Put #fh, , owner.members()
System
