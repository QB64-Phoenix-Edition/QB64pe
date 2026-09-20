$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerLocalElemData
    textValue As String
    numberValue As Long
End Type

Type DynOwnerLocalResultData
    items(0 To 0) _Dynamic As DynOwnerLocalElemData
End Type

Dim result As DynOwnerLocalResultData

result = MakeDynOwnerLocal("seed")

If LBound(result.items) <> 3 Or UBound(result.items) <> 4 Then
    Print "FAIL owner local bounds"
    System 1
End If
If result.items(3).textValue <> "seed-A" Or result.items(3).numberValue <> 30 Then
    Print "FAIL owner local item 3"
    System 1
End If
If result.items(4).textValue <> "seed-B" Or result.items(4).numberValue <> 40 Then
    Print "FAIL owner local item 4"
    System 1
End If

Print "PASS Func_return_UDT_t060"
System 0

Function MakeDynOwnerLocal (seedText As String) As DynOwnerLocalResultData
    Dim temp As DynOwnerLocalResultData

    ReDim temp.items(3 To 4)
    temp.items(3).textValue = seedText + "-A"
    temp.items(3).numberValue = 30
    temp.items(4).textValue = seedText + "-B"
    temp.items(4).numberValue = 40

    MakeDynOwnerLocal = temp

    ' The FUNCTION result must own an independent recursive descriptor clone.
    temp.items(3).textValue = "mutated"
    ReDim temp.items(20 To 20)
    temp.items(20).textValue = "replacement"
End Function
