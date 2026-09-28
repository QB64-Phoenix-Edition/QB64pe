$CONSOLE:ONLY

Option _Explicit

If Val("16777217") <> 16777217&& Then
    Print "FAIL val_default_regression.bas: decimal integer-looking input changed"
    System
End If

If Val("16777217.0") <> 16777217&& Then
    Print "FAIL val_default_regression.bas: decimal floating input changed"
    System
End If

If Val("&H1000001") <> 16777217&& Then
    Print "FAIL val_default_regression.bas: hexadecimal input changed"
    System
End If

If Val("&B1000000000000000000000001") <> 16777217&& Then
    Print "FAIL val_default_regression.bas: binary input changed"
    System
End If

Print "PASS val_default_regression.bas"
System
