$Console:Only
$Unstable:TypeFields
Option _Explicit

Type AudioBuffers
    pcmInline(2 To 9) _Static As Single
    pcmDynamic(0 To 0) _Dynamic As Single
    waveInline(3 To 6) As _Byte
    waveDynamic(0 To 0) _Dynamic As _Byte
End Type

Dim buf As AudioBuffers
Dim rawHandle As Long
ReDim buf.pcmDynamic(-7 To 0) As Single
ReDim buf.waveDynamic(-3 To 0) As _Byte
buf.pcmInline(2) = 0.125
buf.pcmDynamic(-7) = 0.25
buf.waveInline(3) = -32
buf.waveInline(4) = 32
buf.waveDynamic(-3) = -64
buf.waveDynamic(-2) = 64

' This is a compile-only test in the suite. When run manually, use a valid
' raw handle to hear PCM or leave this invalid handle to skip playback.
rawHandle = -1
_SndRawBatch buf.pcmInline(), 2, rawHandle, 4
_SndRawBatch buf.pcmDynamic(), 2, rawHandle, 4

' A two-frame _WAVE is valid. If the dynamic byte view has length 1 from the
' declaration instead of 4 from REDIM, the runtime rejects it.
_Wave 0, buf.waveInline(), 2
_Wave 0, buf.waveDynamic(), 2

Print "PASS"
System 0
