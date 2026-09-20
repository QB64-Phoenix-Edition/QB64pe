$Console:Only
$Unstable:TypeFields
Option _Explicit

Type StaticArrayData
    values(0 To 2) _Static As Long
    names(1 To 2) _Static As String * 4
End Type

Dim result As StaticArrayData

result = MakeStaticArray(7)

If result.values(0) <> 7 Or result.values(1) <> 8 Or result.values(2) <> 9 Then
    Print "FAIL _Static numeric array"
    System 1
End If
If result.names(1) <> "ONE " Or result.names(2) <> "TWO " Then
    Print "FAIL _Static fixed STRING array"
    System 1
End If

Print "PASS Func_return_UDT_t015"
System 0

Function MakeStaticArray (seed As Long) As StaticArrayData
    Dim temp As StaticArrayData

    temp.values(0) = seed
    temp.values(1) = seed + 1
    temp.values(2) = seed + 2
    temp.names(1) = "ONE"
    temp.names(2) = "TWO"
    MakeStaticArray = temp
End Function
