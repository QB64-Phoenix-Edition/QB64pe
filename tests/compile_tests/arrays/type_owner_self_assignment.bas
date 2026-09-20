$Console:Only
$Unstable:TypeFields
Option _Explicit

Type OwnerDesc
    LabelText As String
    Values(0 To 1) _Dynamic As Long
End Type

Dim item As OwnerDesc

item.LabelText = "owner"
item.Values(0) = 111
item.Values(1) = 222

item = item

If item.LabelText <> "owner" Then Print "FAIL type_owner_self_assignment": System
If LBound(item.Values) <> 0 Then Print "FAIL type_owner_self_assignment": System
If UBound(item.Values) <> 1 Then Print "FAIL type_owner_self_assignment": System
If item.Values(0) <> 111 Then Print "FAIL type_owner_self_assignment": System
If item.Values(1) <> 222 Then Print "FAIL type_owner_self_assignment": System

Print "PASS type_owner_self_assignment"
System
