$Console:Only
$Unstable:TypeFields
Option _Explicit

Type BufferValue
    words(0 To 0) _Dynamic As Long
End Type

Dim owner As BufferValue
Dim fh As Long, gotError As Long
Dim outPath As String
outPath = Command$(1) + "/" + Command$(2) + ".bin"
If Command$(1) = "" Then outPath = "member_buffer_unallocated_temp.bin"
If _FileExists(outPath) Then Kill outPath
fh = FreeFile
Open outPath For Binary As #fh

ReDim owner.words(-1 To 2) As Long
Erase owner.words
On Error GoTo FailedAsExpected
Put #fh, , owner.words()
On Error GoTo 0
If gotError <> 9 Or Lof(fh) <> 0 Then System 1
Close #fh
Kill outPath
Print "PASS"
System 0

FailedAsExpected:
gotError = Err
Resume Next
