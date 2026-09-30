$CONSOLE:ONLY

Option _Explicit

Dim sourceText As String
Dim singleValue As Single
Dim doubleValue As Double
Dim floatValue As _Float

sourceText = "1111111111111111111111111111111"

singleValue = Val(sourceText, Single)
doubleValue = Val(sourceText, Double)
floatValue = Val(sourceText, _Float)

If singleValue <= 1E30 Or singleValue >= 1.2E30 Then
    Print "FAIL val_typed_large_integer_decimal.bas: SINGLE used the integer overflow path"
    System
End If

If doubleValue <= 1D30 Or doubleValue >= 1.2D30 Then
    Print "FAIL val_typed_large_integer_decimal.bas: DOUBLE used the integer overflow path"
    System
End If

If floatValue <= 1E30 Or floatValue >= 1.2E30 Then
    Print "FAIL val_typed_large_integer_decimal.bas: _FLOAT used the integer overflow path"
    System
End If

Print "PASS val_typed_large_integer_decimal.bas"
System
