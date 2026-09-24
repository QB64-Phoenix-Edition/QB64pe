$Console:Only
Option _Explicit

Type PairText
    firstText As String
    secondText As String
    value As Long
End Type

Dim result As PairText
result = MakePairText("left", "right", 125)

If result.firstText <> "left" Then
    Print "FAIL first owner: ["; result.firstText; "]"
    System 1
End If
If result.secondText <> "right" Then
    Print "FAIL second owner: ["; result.secondText; "]"
    System 1
End If
If result.value <> 125 Then
    Print "FAIL pair value:"; result.value
    System 1
End If

Print "PASS Func_return_UDT_t020"
System 0

Function MakePairText (a As String, b As String, n As Long) As PairText
    Dim functionResult As PairText

    functionResult.firstText = a
    functionResult.secondText = b
    functionResult.value = n
    MakePairText = functionResult
End Function
