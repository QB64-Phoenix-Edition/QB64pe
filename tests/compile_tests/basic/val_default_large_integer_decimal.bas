$CONSOLE:ONLY

Option _Explicit

Dim sourceText As String
Dim value As _Float

sourceText = "1111111111111111111111111111111"
value = Val(sourceText)

If value <= 1E30 Or value >= 1.2E30 Then
    Print "FAIL val_default_large_integer_decimal.bas: large integer-looking decimal was not parsed as floating point"
    System
End If

Print "PASS val_default_large_integer_decimal.bas"
System
