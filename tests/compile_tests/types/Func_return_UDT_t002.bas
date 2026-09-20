$Console:Only
Option _Explicit

Type CoordData
    x As Long
    y As Long
End Type

Dim result As CoordData

result = MakeCoord(17, -9)
If result.x <> 17 Or result.y <> -9 Then
    Print "FAIL direct members:"; result.x; result.y
    System 1
End If

Print "PASS Func_return_UDT_t002"
System 0

Function MakeCoord (x As Long, y As Long) As CoordData
    MakeCoord.x = x
    MakeCoord.y = y
End Function
