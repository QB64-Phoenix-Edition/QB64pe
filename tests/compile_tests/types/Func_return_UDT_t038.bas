$Console:Only
Option _Explicit

Type ReuseOwnerLeafData
    text As String
    value As Long
End Type

Type ReuseOwnerArrayData
    items(0 To 1) As ReuseOwnerLeafData
    callNumber As Long
End Type

Dim result As ReuseOwnerArrayData
Dim i As Long

' Execute one textual call site twice. The second result leaves both owner UDT
' elements untouched, so the reused hidden slot must contain fresh empty owners.
For i = 1 To 2
    result = MaybeOwnerArray(i = 1)
    If i = 1 Then
        If result.items(0).text <> "first-0" Or result.items(0).value <> 10 Then
            Print "FAIL first owner-array reuse item 0"
            System 1
        End If
        If result.items(1).text <> "first-1" Or result.items(1).value <> 11 Or result.callNumber <> 1 Then
            Print "FAIL first owner-array reuse item 1"
            System 1
        End If
    Else
        If result.items(0).text <> "" Or result.items(1).text <> "" Then
            Print "FAIL owner-array reused strings were not reset"
            System 1
        End If
        If result.items(0).value <> 0 Or result.items(1).value <> 0 Or result.callNumber <> 2 Then
            Print "FAIL owner-array reused fixed data was not reset"
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t038"
System 0

Function MaybeOwnerArray (writeText As Integer) As ReuseOwnerArrayData
    If writeText Then
        MaybeOwnerArray.items(0).text = "first-0"
        MaybeOwnerArray.items(0).value = 10
        MaybeOwnerArray.items(1).text = "first-1"
        MaybeOwnerArray.items(1).value = 11
        MaybeOwnerArray.callNumber = 1
    Else
        MaybeOwnerArray.callNumber = 2
    End If
End Function
