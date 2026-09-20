$Console:Only
Option _Explicit

Type LeafData
    x As Long
    y As Integer
End Type

Type MiddleData
    leaf As LeafData
    tag As Long
End Type

Type RootData
    middle As MiddleData
    tail As Double
End Type

Dim result As RootData
result = MakeRoot(101, -7, 303, 44.25)

If result.middle.leaf.x <> 101 Then
    Print "FAIL deep x:"; result.middle.leaf.x
    System 1
End If
If result.middle.leaf.y <> -7 Then
    Print "FAIL deep y:"; result.middle.leaf.y
    System 1
End If
If result.middle.tag <> 303 Then
    Print "FAIL middle tag:"; result.middle.tag
    System 1
End If
If Abs(result.tail - 44.25) > .0000001 Then
    Print "FAIL root tail:"; result.tail
    System 1
End If

Print "PASS Func_return_UDT_t011"
System 0

Function MakeRoot (x As Long, y As Integer, tag As Long, tail As Double) As RootData
    Dim temp As RootData

    temp.middle.leaf.x = x
    temp.middle.leaf.y = y
    temp.middle.tag = tag
    temp.tail = tail
    MakeRoot = temp
End Function
