$CONSOLE:ONLY

PRINT HexString$(_DEFLATE$("this is a test"))
SYSTEM

Function HexString$(s As String)
Dim ret As String

For i = 1 To Len(s)
    Dim h As String
    h = Hex$(Asc(s, i))
    If Len(h) = 1 Then h = "0" + h
    ret = ret + h
Next

HexString$ = ret
End Function
