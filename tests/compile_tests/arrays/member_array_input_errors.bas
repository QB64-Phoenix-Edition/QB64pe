$Console:Only
$Unstable:TypeFields
Option _Explicit

Type FileValues
    liveNums(0 To 0) _Dynamic As Integer
    liveText(0 To 0) _Dynamic As String
End Type

Dim box As FileValues
ReDim removed(0 To 1) As Integer
Dim pathName As String, fh As Long, seen As Long, afterValue As Long
pathName = Command$(1) + "/" + Command$(2) + ".input-errors"
If Command$(1) = "" Then pathName = "member_input_errors.tmp"
fh = FreeFile
Open pathName For Output As #fh
Write #fh, 10, 40000, 99, "first"
Close #fh

ReDim box.liveNums(5 To 7) As Integer
Erase box.liveNums
Erase removed
Open pathName For Input As #fh
On Error GoTo CatchError
Input #fh, box.liveNums()
If seen <> 9 Then System 1
seen = 0
Input #fh, removed()
If seen <> 9 Then System 2

ReDim box.liveNums(5 To 7) As Integer
box.liveNums(6) = -6: box.liveNums(7) = -7
seen = 0
Input #fh, box.liveNums(), afterValue
If seen <> 6 Or box.liveNums(5) <> 10 Or box.liveNums(6) <> -6 Or box.liveNums(7) <> -7 Then System 3
Input #fh, afterValue
If afterValue <> 99 Then System 4

ReDim box.liveText(2 To 4) As String
box.liveText(3) = "keep3": box.liveText(4) = "keep4"
seen = 0
Input #fh, box.liveText()
If seen <> 62 Or box.liveText(2) <> "first" Or box.liveText(3) <> "keep3" Or box.liveText(4) <> "keep4" Then System 5
On Error GoTo 0
Close #fh
Kill pathName
Print "PASS"
System 0

CatchError:
seen = Err
Resume Next
