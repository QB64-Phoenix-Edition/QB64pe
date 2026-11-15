$Console:Only
$Unstable:TypeFields
Option _Explicit

Type TextBuffer
    liveNums(0 To 0) _Dynamic As Long
End Type
Dim buffer As TextBuffer
ReDim liveTop(-1 To 0, 2 To 3) As Long
Dim pathText As String, gotText As String, wanted As String
Dim fh As Long, gotError As Long
pathText = Command$(1) + "/" + Command$(2) + ".text-runtime"
If Command$(1) = "" Then pathText = "member_array_text_runtime.tmp"
fh = FreeFile
Open pathText For Output As #fh

liveTop(-1, 2) = 71: liveTop(0, 2) = 72: liveTop(-1, 3) = 73: liveTop(0, 3) = 74
WriteValues fh, liveTop()
ReDim _Retain liveTop(-1 To 0, 2 To 4) As Long
WriteValues fh, liveTop()

ReDim buffer.liveNums(4 To 5) As Long
buffer.liveNums(4) = 85: buffer.liveNums(5) = 86
Write #fh, buffer.liveNums()
ReDim buffer.liveNums(-2 To 0) As Long
buffer.liveNums(0) = 97
Write #fh, buffer.liveNums()

Erase liveTop
Erase buffer.liveNums
On Error GoTo catchError
Write #fh, liveTop()
If gotError <> 9 Then System 1
gotError = 0
Print #fh, buffer.liveNums()
If gotError <> 9 Then System 2
On Error GoTo 0
Close #fh

Open pathText For Binary As #fh
gotText = Space$(Lof(fh))
Get #fh, , gotText
Close #fh
Kill pathText
Dim eol As String
eol = Chr$(13) + Chr$(10)
wanted = "71,72,73,74" + eol + "71,72,73,74,0,0" + eol + "85,86" + eol + "0,0,97" + eol
If gotText <> wanted Then
    Print gotText
    System 3
End If
Print "PASS"
System 0

catchError:
gotError = Err
Resume Next

Sub WriteValues (fileNum As Long, nums(0) As Long)
    Write #fileNum, nums()
End Sub
