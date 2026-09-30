$CONSOLE:ONLY

Option _Explicit

If Val("&H1000001", Single) <> 16777216&& Then
    Print "FAIL val_typed_nondecimal_precision.bas: hexadecimal SINGLE return precision is wrong"
    System
End If

If Val("&H1000001", Double) <> 16777217&& Then
    Print "FAIL val_typed_nondecimal_precision.bas: hexadecimal DOUBLE return precision is wrong"
    System
End If

If Val("&H1000001", _Float) <> 16777217&& Then
    Print "FAIL val_typed_nondecimal_precision.bas: hexadecimal _FLOAT return precision is wrong"
    System
End If

If Val("&B1000000000000000000000001", Single) <> 16777216&& Then
    Print "FAIL val_typed_nondecimal_precision.bas: binary SINGLE return precision is wrong"
    System
End If

If Val("&B1000000000000000000000001", Double) <> 16777217&& Then
    Print "FAIL val_typed_nondecimal_precision.bas: binary DOUBLE return precision is wrong"
    System
End If

If Val("&B1000000000000000000000001", _Float) <> 16777217&& Then
    Print "FAIL val_typed_nondecimal_precision.bas: binary _FLOAT return precision is wrong"
    System
End If

Print "PASS val_typed_nondecimal_precision.bas"
System
