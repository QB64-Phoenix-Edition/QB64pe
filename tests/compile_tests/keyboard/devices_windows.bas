$IF LNX THEN

    $CONSOLE
    _DEST _CONSOLE

    CHDIR _STARTDIR$

    '$INCLUDE:'keyconsts.bi'
    '$INCLUDE:'sendinput.bi'

    DIM SHARED testCount AS LONG

    deviceCount& = _DEVICES

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

    SYSTEM

    '$INCLUDE:'sendinput.bm'

    ' Convert scan code into the device button number
    FUNCTION GetDeviceCode& (scan AS LONG)
        DIM deviceCode AS LONG

        ' "Extended" keys will have the 9th bit set, so add 256
        deviceCode = (scan AND &H7FFF) + 1
        IF scan AND &H8000 THEN deviceCode = deviceCode + 256

        GetDeviceCode& = deviceCode
    END FUNCTION

    SUB AssertPress (scan AS LONG)
        testCount = testCount + 1

        emulateScancode scan, Down
        AssertDown scan

        emulateScancode scan, Up
        AssertUp scan
    END SUB

    SUB AssertDown (scan AS LONG)
        testCount = testCount + 1
        deviceCode = GetDeviceCode&(scan)

        WHILE _DEVICEINPUT(1): WEND
        PRINT "Test button down:"; testCount; ": ";
        IF _BUTTON(deviceCode) THEN PRINT "PASS!" ELSE PRINT "FAIL! Key="; scan AND &H7FFF; ", state="; _BUTTON(deviceCode)
    END SUB

    SUB AssertUp (scan AS LONG)
        testCount = testCount + 1
        deviceCode = GetDeviceCode&(scan)

        WHILE _DEVICEINPUT(1): WEND
        PRINT "Test button up  :"; testCount; ": ";
        IF NOT _BUTTON(deviceCode) THEN PRINT "PASS!" ELSE PRINT "FAIL! Key="; scan AND &H7FFF; ", state="; _BUTTON(deviceCode)
    END SUB

$ELSE

    $CONSOLE:ONLY

    CHDIR _STARTDIR$

    ' Not supported for other platforms at the moment, output is simulated
    OPEN "devices_windows.output" FOR INPUT AS #1

    WHILE NOT EOF(1)
        LINE INPUT #1, l$
        PRINT l$
    WEND

    SYSTEM

$END IF
