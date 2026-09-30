$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerReuseElemData
    textValue As String
    numberValue As Long
End Type

Type DynOwnerReuseResultData
    items(0 To 0) _Dynamic As DynOwnerReuseElemData
    callNumber As Long
End Type

Dim result As DynOwnerReuseResultData
Dim i As Long

For i = 1 To 2
    result = MakeDynOwnerReuse(i = 1)
    If i = 1 Then
        If LBound(result.items) <> 5 Or UBound(result.items) <> 6 Then
            Print "FAIL owner reuse first bounds"
            System 1
        End If
        If result.items(5).textValue <> "five" Or result.items(6).textValue <> "six" Then
            Print "FAIL owner reuse first strings"
            System 1
        End If
        If result.items(5).numberValue <> 50 Or result.items(6).numberValue <> 60 Or result.callNumber <> 1 Then
            Print "FAIL owner reuse first payload"
            System 1
        End If
    Else
        ' Reusing the same hidden call-site slot must recursively free the old owner
        ' elements and recreate the declaration-bounds element with an empty qbs.
        If LBound(result.items) <> 0 Or UBound(result.items) <> 0 Then
            Print "FAIL owner reuse reset bounds"
            System 1
        End If
        If result.items(0).textValue <> "" Or result.items(0).numberValue <> 0 Or result.callNumber <> 2 Then
            Print "FAIL owner reuse reset payload"
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t061"
System 0

Function MakeDynOwnerReuse (writePayload As Integer) As DynOwnerReuseResultData
    Dim functionResult As DynOwnerReuseResultData

    If writePayload Then
        ReDim functionResult.items(5 To 6)
        functionResult.items(5).textValue = "five"
        functionResult.items(5).numberValue = 50
        functionResult.items(6).textValue = "six"
        functionResult.items(6).numberValue = 60
        functionResult.callNumber = 1
    Else
        functionResult.callNumber = 2
    End If
    MakeDynOwnerReuse = functionResult
End Function
