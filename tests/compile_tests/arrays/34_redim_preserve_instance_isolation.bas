$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As Integer
End Type

ReDim items(0 To 1) As ItemType
ReDim reference(0 To 3) As Integer
Dim idx As Long

For idx = 0 To 3
    reference(idx) = idx + 10
    items(0).Values(idx) = idx + 10
    items(1).Values(idx) = idx + 1000
Next idx

ReDim _Preserve reference(0 To 3) As Integer
ReDim _Preserve items(0).Values(0 To 3)

For idx = 0 To 3
    If items(0).Values(idx) <> reference(idx) Then
        Print "FAIL 34_redim_preserve_instance_isolation.bas: target instance mismatch at"; idx
        System
    End If
    If items(1).Values(idx) <> idx + 1000 Then
        Print "FAIL 34_redim_preserve_instance_isolation.bas: non-target instance was modified at"; idx; " got "; items(1).Values(idx)
        System
    End If
Next idx

Print "PASS 34_redim_preserve_instance_isolation.bas"
System
