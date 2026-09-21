$Console:Only
Option _Explicit

Type OwnedPartData
    text As String
    value As Long
End Type

Type TwoOwnerResultData
    partA As OwnedPartData
    partB As OwnedPartData
End Type

Dim firstResult As TwoOwnerResultData
Dim secondResult As TwoOwnerResultData

firstResult = MakeTwoOwners("alpha", "beta")
secondResult = firstResult
secondResult.partA.text = "changed"

If firstResult.partA.text <> "alpha" Or firstResult.partB.text <> "beta" Then
    Print "FAIL source owner aliasing: ["; firstResult.partA.text; "] ["; firstResult.partB.text; "]"
    System 1
End If
If secondResult.partA.text <> "changed" Or secondResult.partB.text <> "beta" Then
    Print "FAIL destination owner values: ["; secondResult.partA.text; "] ["; secondResult.partB.text; "]"
    System 1
End If
If firstResult.partA.value <> 11 Or firstResult.partB.value <> 22 Then
    Print "FAIL owner numeric values:"; firstResult.partA.value; firstResult.partB.value
    System 1
End If

Print "PASS Func_return_UDT_t027"
System 0

Function MakeTwoOwners (textA As String, textB As String) As TwoOwnerResultData
    Dim functionResult As TwoOwnerResultData

    functionResult.partA.text = textA
    functionResult.partA.value = 11
    functionResult.partB.text = textB
    functionResult.partB.value = 22
    MakeTwoOwners = functionResult
End Function
