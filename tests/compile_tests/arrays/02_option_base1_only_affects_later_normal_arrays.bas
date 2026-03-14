$CONSOLE:ONLY
Option _Explicit

ReDim beforeArray(3) As Long
Option Base 1
ReDim afterArray(3) As Long

If LBound(beforeArray) <> 0 Or UBound(beforeArray) <> 3 Then
    Print "FAIL 02_option_base1_only_affects_later_normal_arrays.bas: beforeArray bounds wrong"
    System
End If

If LBound(afterArray) <> 1 Or UBound(afterArray) <> 3 Then
    Print "FAIL 02_option_base1_only_affects_later_normal_arrays.bas: afterArray bounds wrong"
    System
End If

Print "PASS 02_option_base1_only_affects_later_normal_arrays.bas"
System
