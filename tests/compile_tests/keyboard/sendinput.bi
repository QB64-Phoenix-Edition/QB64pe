
Const INPUT_TYPE_KEYBOARD = 1

Const KBD_FLAG_EXTENDED = 1
Const KBD_FLAG_UP = 2
Const KBD_FLAG_UNICODE = 4
Const KBD_FLAG_SCANCODE = 8

Const Down = 1
Const Up = 0

Type KbdInput
    typ As Long
    pad As Long
    vk As Integer
    scancode As Integer
    flags As _Unsigned Long
    tim As _Unsigned Long
    pad2 As Long
    extraInfo As _Offset
    padding As String * 8
End Type

Declare CustomType Library
    Function SendInput~&(ByVal inputCount As _Unsigned Long, ByVal inputArray As _Offset, ByVal inputTypeSize As _Unsigned Long)
    Function GetLastError&()
End Declare

