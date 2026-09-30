$Console:Only
Option _Explicit

Type InnerArrayData
    values(-2 To 2) As Integer
    tag As String * 5
End Type

Type OuterArrayData
    id As Long
    item As InnerArrayData
    totals(0 To 1) As Double
End Type

Dim result As OuterArrayData
Dim i As Long

result = MakeOuterArray(40)

If result.id <> 40 Then
    Print "FAIL outer id:"; result.id
    System 1
End If
For i = -2 To 2
    If result.item.values(i) <> 40 + i Then
        Print "FAIL nested array:"; i; result.item.values(i)
        System 1
    End If
Next
If result.item.tag <> "NEST " Then
    Print "FAIL nested fixed string: ["; result.item.tag; "]"
    System 1
End If
If Abs(result.totals(0) - 40.25) > .0000001 Then
    Print "FAIL totals(0):"; result.totals(0)
    System 1
End If
If Abs(result.totals(1) - 40.5) > .0000001 Then
    Print "FAIL totals(1):"; result.totals(1)
    System 1
End If

Print "PASS Func_return_UDT_t014"
System 0

Function MakeOuterArray (seed As Long) As OuterArrayData
    Dim functionResult As OuterArrayData

    Dim temp As OuterArrayData
    Dim i As Long

    temp.id = seed
    For i = -2 To 2
        temp.item.values(i) = seed + i
    Next
    temp.item.tag = "NEST"
    temp.totals(0) = seed + .25
    temp.totals(1) = seed + .5
    functionResult = temp
    MakeOuterArray = functionResult
End Function
