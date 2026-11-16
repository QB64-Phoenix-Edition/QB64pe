$Console:Only
$Unstable:TypeFields
Option _Explicit

Type OutputValues
    oldNums(-2 To 0) As Long
    fixedNums(4 To 6) _Static As Integer
    liveNums(0 To 0, 0 To 0) _Dynamic As Long
    inlineText(0 To 1) _Static As String
    liveText(0 To 0) _Dynamic As String
    fixedText(2 To 3) _Static As String * 3
End Type

Dim boxes(3 To 4) As OutputValues
Dim topNums(-2 To 0) As Long
Dim topText(0 To 1) As String
Dim topFixed(0 To 1) As String * 3
Dim wideValues(0 To 1) As _Unsigned _Integer64
Dim bitValues(0 To 2) As _Unsigned _Bit * 3
Dim Shared callCount As Long
Dim fh As Long
Dim actualPath As String, expectedPath As String
Dim actualText As String, expectedText As String

ReDim boxes(3).liveNums(-1 To 0, 5 To 6) As Long
ReDim boxes(3).liveText(-5 To -4) As String
topNums(-2) = 11: topNums(-1) = -22: topNums(0) = 33
boxes(3).oldNums(-2) = 11: boxes(3).oldNums(-1) = -22: boxes(3).oldNums(0) = 33
boxes(3).fixedNums(4) = 11: boxes(3).fixedNums(5) = -22: boxes(3).fixedNums(6) = 33
boxes(3).liveNums(-1, 5) = 41: boxes(3).liveNums(0, 5) = 42
boxes(3).liveNums(-1, 6) = 43: boxes(3).liveNums(0, 6) = 44
topText(0) = "a," + Chr$(0): topText(1) = "b"
boxes(3).inlineText(0) = topText(0): boxes(3).inlineText(1) = topText(1)
boxes(3).liveText(-5) = topText(0): boxes(3).liveText(-4) = topText(1)
topFixed(0) = "X": topFixed(1) = "XYZ"
boxes(3).fixedText(2) = topFixed(0): boxes(3).fixedText(3) = topFixed(1)
wideValues(0) = 9223372036854775808~&&: wideValues(1) = 18446744073709551615~&&
bitValues(0) = 2: bitValues(1) = 7: bitValues(2) = 4

actualPath = Command$(1) + "/" + Command$(2) + ".text-actual"
expectedPath = Command$(1) + "/" + Command$(2) + ".text-expected"
If Command$(1) = "" Then actualPath = "member_array_text_actual.tmp": expectedPath = "member_array_text_expected.tmp"
fh = FreeFile
Open actualPath For Output As #fh
Write #fh, "tag", topNums(), (boxes(3).oldNums()), boxes(3).fixedNums(), 80
Write #fh, boxes(PickBox).liveNums(), boxes(3).liveNums(0, 6)
Write #fh, topText$(), boxes(3).inlineText(), boxes(3).liveText$(), boxes(3).fixedText(), topFixed$()
Write #fh, wideValues(), bitValues(), topNums(-1)
Print #fh, topNums(), boxes(3).fixedNums();
Print #fh, boxes(3).oldNums(); "tail"
Print #fh, boxes(3).liveText(); boxes(3).fixedText()
Print #fh, boxes(3).liveNums(0, 5); topText(1); boxes(3).inlineText(1)
Print #fh, Using "## "; boxes(3).liveNums(); topNums(-2)
Print #fh, Using "[&]"; boxes(3).inlineText()
Close #fh

Open expectedPath For Output As #fh
Write #fh, "tag", 11, -22, 33, 11, -22, 33, 11, -22, 33, 80
Write #fh, 41, 42, 43, 44, 44
Write #fh, topText(0), topText(1), topText(0), topText(1), topText(0), topText(1), topFixed(0), topFixed(1), topFixed(0), topFixed(1)
Write #fh, wideValues(0), wideValues(1), bitValues(0), bitValues(1), bitValues(2), -22
Print #fh, 11, -22, 33, 11; -22; 33;
Print #fh, 11; -22; 33; "tail"
Print #fh, topText(0); topText(1); topFixed(0); topFixed(1)
Print #fh, 42; "b"; "b"
Print #fh, Using "## "; 41; 42; 43; 44; 11
Print #fh, Using "[&]"; topText(0); topText(1)
Close #fh

Open actualPath For Binary As #fh
actualText = Space$(Lof(fh))
Get #fh, , actualText
Close #fh
Open expectedPath For Binary As #fh
expectedText = Space$(Lof(fh))
Get #fh, , expectedText
Close #fh
Kill actualPath
Kill expectedPath
If callCount <> 1 Then System 1
If actualText <> expectedText Then
    Print "actual: "; actualText
    Print "expected: "; expectedText
    System 2
End If
Print "PASS"
System 0

Function PickBox As Long
    callCount = callCount + 1
    PickBox = 3
End Function
