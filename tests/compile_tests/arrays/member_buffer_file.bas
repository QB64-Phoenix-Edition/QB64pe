$Console:Only
$Unstable:TypeFields
Option _Explicit

Type PairValue
    loValue As Integer
    hiValue As Integer
End Type

Type BufferValue
    marker As Long
    ints(2 To 4) _Static As Integer
    words(0 To 0) _Dynamic As Long
    records(0 To 0) _Dynamic As PairValue
    fixedText(0 To 0) _Dynamic As String * 4
End Type

Dim owner As BufferValue
ReDim targets(-1 To 2) As BufferValue
Dim Shared posCalls As Long
Dim fh As Long, itemIndex As Long, fileStart As _Integer64
Dim tempFile As String
tempFile = Command$(1) + "/" + Command$(2) + ".bin"
If Command$(1) = "" Then tempFile = "member_buffer_file_temp.bin"
If _FileExists(tempFile) Then Kill tempFile
fh = FreeFile
Open tempFile For Binary As #fh

ReDim owner.words(-2 To 0, 3 To 4) As Long
ReDim targets(1).words(8 To 9, -1 To 1) As Long
ReDim owner.records(1 To 3) As PairValue
ReDim targets(1).records(4 To 6) As PairValue
ReDim owner.fixedText(2 To 3) As String * 4
ReDim targets(1).fixedText(-2 To -1) As String * 4
owner.marker = 12345
targets(1).marker = 54321

For itemIndex = 2 To 4
    owner.ints(itemIndex) = itemIndex * 11
Next
owner.words(-2, 3) = 111
owner.words(0, 3) = 222
owner.words(0, 4) = 333
owner.records(1).loValue = 12
owner.records(3).hiValue = 34
owner.fixedText(2) = "ab" + Chr$(0) + "c"
owner.fixedText(3) = "defg"

Put #fh, , owner.ints()
Put #fh, , owner.words&()
Put #fh, , owner.records()
Put #fh, , owner.fixedText()
If Lof(fh) <> 50 Then System 1

Seek #fh, 1
Get #fh, , targets(PickTarget).ints()
Get #fh, , (targets(PickTarget).words())
Get #fh, , targets(PickTarget).records()
Get #fh, , targets(PickTarget).fixedText()
If posCalls <> 4 Then System 2
If targets(1).marker <> 54321 Or owner.marker <> 12345 Then System 3
If targets(1).ints(2) <> 22 Or targets(1).ints(4) <> 44 Then System 4
If targets(1).words(8, -1) <> 111 Or targets(1).words(8, 0) <> 222 Or targets(1).words(9, 1) <> 333 Then System 5
If targets(1).records(4).loValue <> 12 Or targets(1).records(6).hiValue <> 34 Then System 6
If targets(1).fixedText(-2) <> "ab" + Chr$(0) + "c" Or targets(1).fixedText(-1) <> "defg" Then System 7

fileStart = Seek(fh)
Put #fh, , targets(PickTarget).words()
If posCalls <> 5 Or Seek(fh) <> fileStart + 24 Then System 8

Close #fh
Kill tempFile
Print "PASS"
System 0

Function PickTarget As Long
    posCalls = posCalls + 1
    PickTarget = 1
End Function
