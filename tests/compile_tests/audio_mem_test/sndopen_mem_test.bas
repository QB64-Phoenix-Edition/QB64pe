$Console:Only
Option _Explicit
Option _ExplicitArray

Dim f As Long: f = FreeFile
Open "test.mp3" For Binary Access Read As f

Dim buffer As String: buffer = Input$(LOF(f), f)
Print "Size ="; Len(buffer)

Close f

Dim h As Long: h = _SndOpen(buffer, "memory")
Print "Handle ="; h

System

