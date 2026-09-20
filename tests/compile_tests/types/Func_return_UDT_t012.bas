$Console:Only
Option _Explicit

Type InnerData
    alwaysSet As Long
    conditionalSet As Long
End Type

Type OuterData
    item As InnerData
    serial As Long
End Type

Dim result As OuterData
Dim i As Long

For i = 1 To 20
    result = MakeOuter(i, -1)
    If result.item.alwaysSet <> i Or result.item.conditionalSet <> 999 Or result.serial <> i * 10 Then
        Print "FAIL set iteration:"; i; result.item.alwaysSet; result.item.conditionalSet; result.serial
        System 1
    End If

    result = MakeOuter(i, 0)
    If result.item.alwaysSet <> i Or result.serial <> i * 10 Then
        Print "FAIL reset values:"; i; result.item.alwaysSet; result.serial
        System 1
    End If
    If result.item.conditionalSet <> 0 Then
        Print "FAIL stale nested return storage:"; i; result.item.conditionalSet
        System 1
    End If
Next

result = MakeWrapped(77)
If result.item.alwaysSet <> 77 Or result.item.conditionalSet <> 999 Or result.serial <> 770 Then
    Print "FAIL nested FUNCTION result copy:"; result.item.alwaysSet; result.item.conditionalSet; result.serial
    System 1
End If

Print "PASS Func_return_UDT_t012"
System 0

Function MakeOuter (value As Long, setConditional As Integer) As OuterData
    MakeOuter.item.alwaysSet = value
    If setConditional Then MakeOuter.item.conditionalSet = 999
    MakeOuter.serial = value * 10
End Function

Function MakeWrapped (value As Long) As OuterData
    MakeWrapped = MakeOuter(value, -1)
End Function
