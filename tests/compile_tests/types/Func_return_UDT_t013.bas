$Console:Only
Option _Explicit

Type ArrayData
    values(1 To 4) As Long
    code As String * 6
End Type

Dim result As ArrayData
Dim i As Long

result = MakeArrayData(100)

For i = 1 To 4
    If result.values(i) <> 100 + i Then
        Print "FAIL inline numeric array:"; i; result.values(i)
        System 1
    End If
Next
If result.code <> "ARRAY " Then
    Print "FAIL scalar fixed string: ["; result.code; "]"
    System 1
End If

Print "PASS Func_return_UDT_t013"
System 0

Function MakeArrayData (seed As Long) As ArrayData
    Dim i As Long
    For i = 1 To 4
        MakeArrayData.values(i) = seed + i
    Next
    MakeArrayData.code = "ARRAY"
End Function
