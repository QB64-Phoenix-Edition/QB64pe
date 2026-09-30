$CONSOLE:ONLY

Option _Explicit

' 2^24 + 1 is not exactly representable by SINGLE.
If Val("16777217", Single) <> 16777216&& Then
    Print "FAIL val_typed_floating_precision.bas: SINGLE return precision is wrong"
    System
End If

' The same value is exactly representable by DOUBLE.
If Val("16777217", Double) <> 16777217&& Then
    Print "FAIL val_typed_floating_precision.bas: DOUBLE return precision is wrong"
    System
End If

' 2^53 + 1 is not exactly representable by DOUBLE.
If Val("9007199254740993", Double) <> 9007199254740992&& Then
    Print "FAIL val_typed_floating_precision.bas: DOUBLE rounding above 2^53 is wrong"
    System
End If

Print "PASS val_typed_floating_precision.bas"
System
