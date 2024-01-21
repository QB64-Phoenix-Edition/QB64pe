Option _Explicit
DEFLNG A-Z
$Console:Only

Dim Debug As Long

'$include:'../../../source/global/constants.bas'
'$include:'../../../source/utilities/type.bi'
sp = "@" ' Makes the output readable

Type TestCase
    format As String
    providedArgs As String
    result As Long
    firstOptional As Long
End Type

Dim tests(18) As TestCase

tests(1).format = "?[?]"
tests(1).providedArgs = MKL$(-1)
tests(1).result = -1
tests(1).firstOptional = 2

tests(2).format = "?[?]"
tests(2).providedArgs = MKL$(-1) + MKL$(-1)
tests(2).result = -1
tests(2).firstOptional = 2

tests(3).format = "?[?]"
tests(3).providedArgs = MKL$(0) + MKL$(-1)
tests(3).result = 0
tests(3).firstOptional = 2

tests(4).format = "?[??]"
tests(4).providedArgs = MKL$(0) + MKL$(-1)
tests(4).result = 0
tests(4).firstOptional = 2

tests(5).format = "?[??]"
tests(5).providedArgs = MKL$(0) + MKL$(-1) + MKL$(-1)
tests(5).result = 0
tests(5).firstOptional = 2

tests(6).format = "?[??]"
tests(6).providedArgs = MKL$(-1) + MKL$(-1) + MKL$(-1)
tests(6).result = -1
tests(6).firstOptional = 2

tests(7).format = "?[??]"
tests(7).providedArgs = MKL$(-1) + MKL$(0) + MKL$(0)
tests(7).result = -1
tests(7).firstOptional = 2

tests(8).format = "?[??]"
tests(8).providedArgs = MKL$(-1)
tests(8).result = -1
tests(8).firstOptional = 2

tests(9).format = "?[?[?]]"
tests(9).providedArgs = MKL$(-1)
tests(9).result = -1
tests(9).firstOptional = 2

tests(10).format = "?[?[?]]"
tests(10).providedArgs = MKL$(-1) + MKL$(-1) + MKL$(0)
tests(10).result = -1
tests(10).firstOptional = 2

tests(11).format = "?[?[?]]"
tests(11).providedArgs = MKL$(-1) + MKL$(-1) + MKL$(-1)
tests(11).result = -1
tests(11).firstOptional = 2

tests(12).format = "?[?[?]]"
tests(12).providedArgs = MKL$(-1) + MKL$(0) + MKL$(-1)
tests(12).result = 0
tests(12).firstOptional = 2

tests(13).format = "?[[?][?]]"
tests(13).providedArgs = MKL$(-1) + MKL$(0) + MKL$(-1)
tests(13).result = -1
tests(13).firstOptional = 2

tests(14).format = "[?][??]"
tests(14).providedArgs = MKL$(0) + MKL$(-1) + MKL$(-1)
tests(14).result = -1
tests(14).firstOptional = 1

tests(15).format = "?"
tests(15).providedArgs = MKL$(0) + MKL$(-1) + MKL$(-1)
tests(15).result = 0
tests(15).firstOptional = 0

tests(16).format = "?"
tests(16).providedArgs = ""
tests(16).result = 0
tests(16).firstOptional = 0

tests(17).format = "?[?]"
tests(17).providedArgs = ""
tests(17).result = 0
tests(17).firstOptional = 2

tests(18).format = "???[?]"
tests(18).providedArgs = MKL$(-1) + MKL$(-1) + MKL$(-1) + MKL$(-1)
tests(18).result = -1
tests(18).firstOptional = 4

ReDim provided(10) As Long
Dim i As Long

For i = 1 To UBOUND(tests)
    Dim firstOpt As Long, result As Long

    firstOpt& = 0

    argStringToArray tests(i).providedArgs, provided()
    result& = isValidArgSet(tests(i).format, provided(), firstOpt&)

    Print "Test"; i; ", Format: "; tests(i).format; ", Args: "; argStringPrint$(tests(i).providedArgs)
    Print "    Expected:"; tests(i).result; ", Actual"; result&
    Print "    firstOpt:"; tests(i).firstOptional; ", Actual"; firstOpt&
    If tests(i).result = result& And tests(i).firstOptional = firstOpt& Then
        Print "      PASS!"
    Else
        Print "      FAIL!"
    End If
    Print
Next

System

'$include:'../../../source/utilities/elements.bas'

SUB argStringToArray(argString As String, provided() As Long)
    ReDim provided(LEN(argString) / 4) As Long, i As Long

    for i = 1 to UBOUND(provided)
        provided(i) = CVL(MID$(argString, (i - 1) * 4 + 1, 4))
    next
END SUB

FUNCTION argStringPrint$(argString As String)
    Dim res As String, i As Long

    res$ = ""

    If argString = "" Then argStringPrint$ = "": Exit Function

    res$ = STR$(CVL(MID$(argString, 1, 4)))
    For i = 2 To LEN(argString) / 4
        res$ = res$ + ", " + STR$(CVL(MID$(argString, (i - 1) * 4 + 1, 4)))
    Next I

    argStringPrint$ = res$
END FUNCTION
