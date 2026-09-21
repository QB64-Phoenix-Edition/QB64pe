$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicLocalResultData
    values(0 To 0) _Dynamic As Long
End Type

Dim result As DynamicLocalResultData

result = MakeDynamicLocal

If LBound(result.values) <> -2 Or UBound(result.values) <> 1 Then
    Print "FAIL local dynamic assignment bounds"
    System 1
End If
If result.values(-2) <> 20 Or result.values(-1) <> 21 Or result.values(0) <> 22 Or result.values(1) <> 23 Then
    Print "FAIL local dynamic assignment payload"
    System 1
End If

Print "PASS Func_return_UDT_t043"
System 0

Function MakeDynamicLocal As DynamicLocalResultData
    Dim functionResult As DynamicLocalResultData

    Dim temp As DynamicLocalResultData

    ReDim temp.values(-2 To 1)
    temp.values(-2) = 20
    temp.values(-1) = 21
    temp.values(0) = 22
    temp.values(1) = 23

    functionResult = temp

    ' The return object must own an independent descriptor/payload clone.
    temp.values(-2) = 999
    ReDim temp.values(7 To 8)
    MakeDynamicLocal = functionResult
End Function
