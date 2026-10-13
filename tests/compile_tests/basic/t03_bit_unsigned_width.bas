$CONSOLE:ONLY
Dim sampleValue As _Unsigned _Bit * 5
sampleValue = 17
AdvanceFive sampleValue
If sampleValue <> 18 Then Print "FAIL unsigned _BIT * 5": End
Print "PASS"
System

Sub AdvanceFive (value As _Unsigned _Bit * 5)
    value = value + 1
End Sub
