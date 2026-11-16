$Console:Only
$Unstable:TypeFields
Option _Explicit

Type FileValues
    inlineText(1 To 3) As String
    fixedText(-1 To 0) _Static As String * 4
    liveText(0 To 0) _Dynamic As String
    staticNums(7 To 8) _Static As Integer
    liveNums(0 To 0) _Dynamic As Long
    marker As Long
End Type

Dim blocks(3 To 4) As FileValues
Dim txt(2 To 3) As String
Dim packedNums(1 To 3) As _Unsigned _Bit * 3
Dim largeNums(1 To 2) As _Unsigned _Integer64
Dim span(1 To 2, 5 To 6) As Long
Dim fh As Long, pathName As String, firstValue As Long, nextValue As Long
Dim Shared picks As Long

ReDim blocks(3).liveText(-7 To -6) As String
ReDim blocks(3).liveNums(-2 To -1, 6 To 7) As Long
blocks(3).marker = 2468
blocks(4).marker = 1357
blocks(3).inlineText(1) = "111"
blocks(3).inlineText(2) = "two, words"
blocks(3).inlineText(3) = " 333 "
blocks(3).fixedText(-1) = "x"
blocks(3).fixedText(0) = "z,!"
blocks(3).liveText(-7) = ""
blocks(3).liveText(-6) = "dynamic"
blocks(3).staticNums(7) = -33
blocks(3).staticNums(8) = 67
blocks(3).liveNums(-2, 6) = 11
blocks(3).liveNums(-1, 6) = 22
blocks(3).liveNums(-2, 7) = 33
blocks(3).liveNums(-1, 7) = 44
txt(2) = "first": txt(3) = "second"
packedNums(1) = 7: packedNums(2) = 3: packedNums(3) = 2
largeNums(1) = 9223372036854775813~&&: largeNums(2) = 18446744073709551615~&&

pathName = Command$(1) + "/" + Command$(2) + ".input-text"
If Command$(1) = "" Then pathName = "member_input_roundtrip.tmp"
fh = FreeFile
Open pathName For Output As #fh
Write #fh, 71, blocks(3).inlineText(), blocks(3).fixedText(), blocks(3).liveText(), 72
Write #fh, blocks(3).staticNums(), blocks(3).liveNums(), txt$(), packedNums(), largeNums()
Write #fh, 14, 15, 16, 17, 18, 19
Close #fh

Erase blocks(3).inlineText, blocks(3).fixedText
Erase blocks(3).liveText, blocks(3).staticNums, blocks(3).liveNums
Erase txt, packedNums, largeNums
ReDim blocks(3).liveText(-7 To -6) As String
ReDim blocks(3).liveNums(-2 To -1, 6 To 7) As Long

Open pathName For Input As #fh
Input #fh, firstValue, (blocks(PickBlock).inlineText()), blocks(3).fixedText(), blocks(3).liveText$(), nextValue
If firstValue <> 71 Or nextValue <> 72 Or picks <> 1 Then System 1
If blocks(3).inlineText(1) <> "111" Or blocks(3).inlineText(2) <> "two, words" Or blocks(3).inlineText(3) <> " 333 " Then System 2
If blocks(3).fixedText(-1) <> "x   " Or blocks(3).fixedText(0) <> "z,! " Then System 3
If blocks(3).liveText(-7) <> "" Or blocks(3).liveText(-6) <> "dynamic" Then System 4
Input #fh, blocks(3).staticNums(), blocks(3).liveNums(), txt$(), packedNums(), largeNums()
If blocks(3).staticNums(7) <> -33 Or blocks(3).staticNums(8) <> 67 Then System 5
If blocks(3).liveNums(-2, 6) <> 11 Or blocks(3).liveNums(-1, 6) <> 22 Then System 6
If blocks(3).liveNums(-2, 7) <> 33 Or blocks(3).liveNums(-1, 7) <> 44 Then System 7
If txt(2) <> "first" Or txt(3) <> "second" Then System 8
If packedNums(1) <> 7 Or packedNums(2) <> 3 Or packedNums(3) <> 2 Then System 9
If largeNums(1) <> 9223372036854775813~&& Or largeNums(2) <> 18446744073709551615~&& Then System 10
ReadSpan fh, span()
Input #fh, blocks(3).liveNums(-1, 7), nextValue
If span(1, 5) <> 14 Or span(2, 5) <> 15 Or span(1, 6) <> 16 Or span(2, 6) <> 17 Then System 11
If blocks(3).liveNums(-1, 7) <> 18 Or nextValue <> 19 Or blocks(3).liveNums(-2, 7) <> 33 Then System 12
If blocks(3).marker <> 2468 Or blocks(4).marker <> 1357 Then System 13
Close #fh
Kill pathName
Print "PASS"
System 0

Function PickBlock As Long
    picks = picks + 1
    PickBlock = 3
End Function

Sub ReadSpan (fileNum As Long, targetVals(0, 0) As Long)
    Input #fileNum, targetVals()
End Sub
