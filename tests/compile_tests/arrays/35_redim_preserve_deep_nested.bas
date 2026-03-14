$CONSOLE:ONLY
Option _Explicit

Type Level2Type
    Datas(0 To 3) As Integer
End Type

Type Level1Type
    Nodes(0 To 1) As Level2Type
End Type

Type RootType
    Groups(0 To 1) As Level1Type
End Type

ReDim reference(0 To 3) As Integer
ReDim items(0 To 1) As RootType
Dim idx As Long

For idx = 0 To 3
    reference(idx) = idx + 100
    items(1).Groups(0).Nodes(1).Datas(idx) = idx + 100
Next idx

ReDim _Preserve reference(0 To 3) As Integer
ReDim _Preserve items(1).Groups(0).Nodes(1).Datas(0 To 3)

For idx = 0 To 3
    If items(1).Groups(0).Nodes(1).Datas(idx) <> reference(idx) Then
        Print "FAIL 35_redim_preserve_deep_nested.bas: index"; idx; " expected "; reference(idx); " got "; items(1).Groups(0).Nodes(1).Datas(idx)
        System
    End If
Next idx

Print "PASS 35_redim_preserve_deep_nested.bas"
System
