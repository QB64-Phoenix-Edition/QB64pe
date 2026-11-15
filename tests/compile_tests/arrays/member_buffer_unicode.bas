$Console:Only
$Unstable:TypeFields
Option _Explicit

Type TextBuffers
    inlinePos(3 To 8) As Long
    dynPos(0 To 0) _Dynamic As Long
End Type

Dim buffers As TextBuffers
Dim topPositions(1 To 6) As Long
Dim textCount As Long, itemIndex As Long
ReDim buffers.dynPos(-5 To -4, 7 To 9) As Long

textCount = _UCharPos("Hello", topPositions())
If textCount <> 5 Then System 1
textCount = _UCharPos("Hello", buffers.inlinePos())
If textCount <> 5 Then System 2
textCount = _UCharPos("Hello", buffers.dynPos())
If textCount <> 5 Then System 3
For itemIndex = 1 To 6
    If buffers.inlinePos(itemIndex + 2) <> topPositions(itemIndex) Then System 4
    If buffers.dynPos(-5 + ((itemIndex - 1) Mod 2), 7 + ((itemIndex - 1) \ 2)) <> topPositions(itemIndex) Then System 5
Next
If topPositions(6) <> 40 Then System 6
Print "PASS"
System 0
