$Console:Only
$Unstable:TypeFields
Option _Explicit

Type ClassicRegs
    axVal As Integer
    bxVal As Integer
    cxVal As Integer
    dxVal As Integer
    bpVal As Integer
    siVal As Integer
    diVal As Integer
    flagsVal As Integer
    dsVal As Integer
    esVal As Integer
End Type

Type RegisterSet
    inlineRegs(-3 To 6) As Integer
    staticRegs(5 To 14) _Static As Integer
    liveRegs(0 To 0) _Dynamic As Integer
End Type

Dim hosts(2 To 3) As RegisterSet
Dim stdIn(0 To 9) As Integer, stdOut(0 To 9) As Integer
Dim classicIn As ClassicRegs, classicOut As ClassicRegs
Dim caught As Long
ReDim hosts(2).liveRegs(-8 To 1) As Integer
ReDim hosts(3).liveRegs(9 To 18) As Integer

' INT 33h, AX=0 has deterministic results and does not require mouse input.
stdIn(2) = 123
Call Interrupt(&H33, stdIn(), stdOut())
If stdOut(0) <> -1 Or stdOut(1) <> 3 Or stdOut(2) <> 123 Then System 1
classicIn.dsVal = 347
Call InterruptX(&H33, classicIn, classicOut)
If classicOut.axVal <> -1 Or classicOut.bxVal <> 3 Or classicOut.dsVal <> 347 Then System 2

hosts(2).liveRegs(-6) = 321
hosts(3).staticRegs(12) = 864
Call Interrupt(&H33, hosts(2).liveRegs(), hosts(3).staticRegs())
If hosts(3).staticRegs(5) <> -1 Or hosts(3).staticRegs(6) <> 3 Then System 3
If hosts(3).staticRegs(7) <> 321 Or hosts(3).staticRegs(12) <> 864 Then System 4

hosts(3).inlineRegs(-1) = 425
Call Interrupt(&H33, hosts(3).inlineRegs(), hosts(3).liveRegs())
If hosts(3).liveRegs(9) <> -1 Or hosts(3).liveRegs(10) <> 3 Then System 5
If hosts(3).liveRegs(11) <> 425 Then System 6

hosts(2).liveRegs(0) = 613
hosts(2).liveRegs(1) = 719
' The statement form without CALL also accepts bare member arrays, as before.
InterruptX &H33, hosts(2).liveRegs, hosts(3).liveRegs
If hosts(3).liveRegs(9) <> -1 Or hosts(3).liveRegs(10) <> 3 Then System 7
If hosts(3).liveRegs(17) <> 613 Or hosts(3).liveRegs(18) <> 719 Then System 8

' Use the live size, not the declaration's single-element placeholder.
ReDim hosts(2).liveRegs(-10 To -3) As Integer
Call Interrupt(&H33, hosts(2).liveRegs(), hosts(3).liveRegs())
On Error GoTo captureFailure
Call InterruptX(&H33, hosts(2).liveRegs(), hosts(3).liveRegs())
On Error GoTo 0
If caught <> 5 Or hosts(3).liveRegs(17) <> 613 Then System 9

Erase hosts(2).liveRegs
caught = 0
On Error GoTo captureFailure
Call Interrupt(&H33, hosts(2).liveRegs(), hosts(3).liveRegs())
On Error GoTo 0
If caught <> 9 Or hosts(3).liveRegs(9) <> -1 Then System 10

Print "PASS"
System 0

captureFailure:
caught = Err
Resume Next
