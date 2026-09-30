$Console:Only
Option _Explicit

Type PairData
    number As Long
    value As Double
End Type

Dim result As PairData

result = MakePair(123, 45.5)
If result.number <> 123 Then
    Print "FAIL number:"; result.number
    System 1
End If
If Abs(result.value - 45.5) > .0000001 Then
    Print "FAIL value:"; result.value
    System 1
End If

Print "PASS Func_return_UDT_t001"
System 0

Function MakePair (number As Long, value As Double) As PairData
    Dim functionResult As PairData

    Dim temp As PairData

    temp.number = number
    temp.value = value
    functionResult = temp
    MakePair = functionResult
End Function
