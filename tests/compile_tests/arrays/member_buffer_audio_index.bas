$Console:Only
$Unstable:TypeFields
Option _Explicit

Type AudioBlock
    inlinePcm(-4 To 3) As Single
    staticPcm(5 To 12) _Static As Single
    livePcm(0 To 0) _Dynamic As Single
    staticWave(4 To 9) _Static As _Byte
    liveWave(0 To 0) _Dynamic As _Byte
End Type

Dim blocks(3 To 4) As AudioBlock
Dim plainPcm(-4 To 3) As Single
Dim plainWave(4 To 9) As _Byte
Dim gotError As Long
Dim Shared parentCalls As Long, indexCalls As Long

' Explicit invalid raw handle keeps this test independent of a sound device.
_SndRawBatch plainPcm(), 2, -1
_SndRawBatch plainPcm(-2), 2, -1
_SndRawBatch blocks(3).inlinePcm(), 2, -1
_SndRawBatch blocks(3).inlinePcm(-2), 2, -1
_SndRawBatch blocks(3).staticPcm(), 2, -1
_SndRawBatch blocks(3).staticPcm(7), 2, -1

ReDim blocks(3).livePcm(3 To 10) As Single
_SndRawBatch blocks(3).livePcm(), 2, -1
_SndRawBatch (blocks(PickAudioBlock).livePcm!(PickAudioIndex)), 2, -1
If parentCalls <> 1 Or indexCalls <> 1 Then System 1

' REDIM changes the active rank and bounds. The indexed start is still in
' element units, not stereo frames.
ReDim blocks(3).livePcm(-2 To 1, 5 To 6) As Single
_SndRawBatch blocks(3).livePcm(), 2, -1
_SndRawBatch blocks(3).livePcm(-1, 6), 2, -1, 2

' _WAVE requires at least two bytes. An indexed operand must retain the
' remaining two bytes, not be truncated to a scalar or extend into a sibling.
ReDim blocks(3).liveWave(-5 To 0) As _Byte
On Error GoTo CaptureError
_Wave 0, plainWave(8)
If gotError <> 0 Then System 2
_Wave 0, blocks(3).staticWave(8)
If gotError <> 0 Then System 3
_Wave 0, blocks(3).liveWave()
If gotError <> 0 Then System 4
_Wave 0, blocks(3).liveWave(-1)
If gotError <> 0 Then System 5
_Wave 0, blocks(3).liveWave(0)
If gotError <> 5 Then System 6
gotError = 0
_SndRawBatch blocks(3).livePcm(2, 5), 2, -1
If gotError <> 9 Then System 7
gotError = 0
_SndRawBatch blocks(3).livePcm(5), 2, -1
If gotError <> 9 Then System 8
gotError = 0
_SndRawBatch blocks(3).staticPcm(13), 2, -1
If gotError <> 9 Then System 9
gotError = 0
Erase blocks(3).livePcm
_SndRawBatch blocks(3).livePcm(-1, 6), 2, -1
If gotError <> 9 Then System 10
On Error GoTo 0
Print "PASS"
System 0

CaptureError:
gotError = Err
Resume Next

Function PickAudioBlock As Long
    parentCalls = parentCalls + 1
    PickAudioBlock = 3
End Function

Function PickAudioIndex As Long
    indexCalls = indexCalls + 1
    PickAudioIndex = 5
End Function
