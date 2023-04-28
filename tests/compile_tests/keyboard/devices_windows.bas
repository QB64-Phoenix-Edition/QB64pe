$Console
_Dest _Console

ChDir _StartDir$

$IF LNX THEN

'$include:'keyconsts.bi'
'$include:'sendinput.bi'

Dim Shared testCount As Long

deviceCount& = _Devices

AssertPress EscScanCode
AssertPress EnterScancode
AssertPress TabScancode
AssertPress OneScancode
AssertPress AScanCode
AssertPress WScanCode
AssertPress SpaceScanCode

AssertPress PgUpScanCode
AssertPress PgDownScanCode
AssertPress HomeScanCode
AssertPress InsertScanCode
AssertPress DeleteScanCode
AssertPress EndScanCode

AssertPress LCtrlScancode
AssertPress LShiftScancode
AssertPress LAltScancode

AssertPress RCtrlScancode
AssertPress RShiftScancode
AssertPress RAltScancode

AssertPress NumLockScancode
AssertPress ScrollLockScancode
AssertPress KeypadUpScancode

AssertPress F1Scancode
AssertPress F2Scancode
AssertPress F3Scancode
AssertPress F4Scancode
AssertPress F5Scancode
AssertPress F6Scancode
AssertPress F7Scancode
AssertPress F8Scancode
AssertPress F9Scancode
AssertPress F10Scancode
AssertPress F11Scancode
AssertPress F12Scancode

' Test multiple keys at the same time
' Modifiers in paticular are important to test
emulateScancode LShiftScancode, Down
emulateScancode LCtrlScancode, Down
emulateScancode LAltScancode, Down
emulateScancode AScanCode, Down

AssertDown LShiftScancode
AssertDown LCtrlScancode
AssertDown LAltScancode
AssertDown AScanCode

' These keys are released in same order they were pressed
emulateScancode LShiftScancode, Up
AssertUp LShiftScancode
emulateScancode LCtrlScancode, Up
AssertUp LCtrlScancode
emulateScancode LAltScancode, Up
AssertUp LAltScancode
emulateScancode AScanCode, Up
AssertUp AScanCode

' Same test, but release keys in opposite order they were pressed
emulateScancode LShiftScancode, Down
emulateScancode LCtrlScancode, Down
emulateScancode LAltScancode, Down
emulateScancode AScanCode, Down

AssertDown LShiftScancode
AssertDown LCtrlScancode
AssertDown LAltScancode
AssertDown AScanCode

emulateScancode AScanCode, Up
AssertUp AScanCode
emulateScancode LAltScancode, Up
AssertUp LAltScancode
emulateScancode LCtrlScancode, Up
AssertUp LCtrlScancode
emulateScancode LShiftScancode, Up
AssertUp LShiftScancode

' Test holding and releasing both left/right modifiers

' Shift
emulateScancode LShiftScancode, Down
AssertDown LShiftScancode
emulateScancode RShiftScancode, Down
AssertDown RShiftScancode

emulateScancode LShiftScancode, Up
AssertUp LShiftScancode
emulateScancode RShiftScancode, Up
AssertUp RShiftScancode

' Control
emulateScancode LCtrlScancode, Down
AssertDown LCtrlScancode
emulateScancode RCtrlScancode, Down
AssertDown RCtrlScancode

emulateScancode LCtrlScancode, Up
AssertUp LCtrlScancode
emulateScancode RCtrlScancode, Up
AssertUp RCtrlScancode

' Alt
emulateScancode LAltScancode, Down
AssertDown LAltScancode
emulateScancode RAltScancode, Down
AssertDown RAltScancode

emulateScancode LAltScancode, Up
AssertUp LAltScancode
emulateScancode RAltScancode, Up
AssertUp RAltScancode

System

'$include:'sendinput.bm'

' Convert scan code into the device button number
Function GetDeviceCode&(scan As Long)
    Dim deviceCode As Long

    ' "Extended" keys will have the 9th bit set, so add 256
    deviceCode = (scan And &H7FFF) + 1
    If scan And &H8000 Then deviceCode = deviceCode + 256

    GetDeviceCode& = deviceCode
End Function

Sub AssertPress(scan As Long)
    testCount = testCount + 1

    emulateScancode scan, Down
    AssertDown scan

    emulateScancode scan, Up
    AssertUp scan
End Sub

Sub AssertDown(scan As Long)
    testCount = testCount + 1
    deviceCode = GetDeviceCode&(scan)

    While _DeviceInput(1): Wend
    Print "Test button down:"; testCount; ": ";
    If _Button(deviceCode) Then Print "PASS!" Else Print "FAIL! Key="; scan And &H7FFF; ", state="; _Button(deviceCode)
End Sub

Sub AssertUp(scan As Long)
    testCount = testCount + 1
    deviceCode = GetDeviceCode&(scan)

    While _DeviceInput(1): Wend
    Print "Test button up  :"; testCount; ": ";
    If Not _Button(deviceCode) Then Print "PASS!" Else Print "FAIL! Key="; scan And &H7FFF; ", state="; _Button(deviceCode)
End Sub

$Else

' Not supported for other platforms at the moment, output is simulated
Open "devices_windows.output" For Input As #1

While Not Eof(1)
    Line Input #1, l$
    Print l$
Wend

System

$End If
