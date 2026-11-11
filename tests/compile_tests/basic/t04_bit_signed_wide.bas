$CONSOLE:ONLY
Dim sampleValue As _Bit * 33
sampleValue = 123456789
AdvanceWide sampleValue
If sampleValue <> 123456790 Then Print "FAIL signed _BIT * 33": End
If ReadWide&(sampleValue) <> 1 Then Print "FAIL FUNCTION _BIT * 33": End
Print "PASS"
SYSTEM

Sub AdvanceWide (value As _Bit * 33)
    value = value + 1
End Sub

Function ReadWide& (value As _Bit * 33)
    If value = 123456790 Then ReadWide& = 1 Else ReadWide& = 0
End Function
