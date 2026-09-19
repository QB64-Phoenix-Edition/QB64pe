$Console:Only
$Unstable:TypeFields

Type FourDData
    _Dynamic values(1 To 2, 1 To 3, 1 To 4, 1 To 5) As Long
End Type


' 4D top-level array -> 4D _Dynamic UDT field -> REDIM _RETAIN
' -> top-level 4D array.
'
' The dynamic UDT field is resized in all four dimensions and
' its lower bounds are also changed. The original coordinate
' intersection must survive _RETAIN and must then be copied
' back correctly through _ArrayCopy.

Dim source4D(1 To 2, 1 To 3, 1 To 4, 1 To 5) As Long
Dim result4D(1 To 2, 1 To 3, 1 To 4, 1 To 5) As Long
Dim holder4D As FourDData

Dim indexA As Long
Dim indexB As Long
Dim indexC As Long
Dim indexD As Long
Dim TestOk As _Byte
TestOk = 1

For indexA = 1 To 2
    For indexB = 1 To 3
        For indexC = 1 To 4
            For indexD = 1 To 5
                source4D(indexA, indexB, indexC, indexD) = _
                    indexA * 1000000 + _
                    indexB * 10000 + _
                    indexC * 100 + _
                    indexD

                result4D(indexA, indexB, indexC, indexD) = -4
            Next
        Next
    Next
Next

' Copy the complete ordinary 4D array into the dynamic UDT field.
_ArrayCopy source4D() To holder4D.values()

' Change both the size and bounds of every dimension.
' The complete original 1..2, 1..3, 1..4, 1..5 coordinate
' range remains inside the new bounds and therefore must survive.
ReDim _Retain holder4D.values(0 To 3, 0 To 4, 0 To 5, 0 To 6)

' Verify the retained payload before performing the second copy.
For indexA = 1 To 2
    For indexB = 1 To 3
        For indexC = 1 To 4
            For indexD = 1 To 5
                If holder4D.values(indexA, indexB, indexC, indexD) <> _
                    indexA * 1000000 + _
                    indexB * 10000 + _
                    indexC * 100 + _
                    indexD Then
                    TestOk = 0
                End If
            Next
        Next
    Next
Next

' Copy only the original retained coordinate range back into
' an ordinary 4D array.
_ArrayCopy holder4D.values(1 To 2, 1 To 3, 1 To 4, 1 To 5) To result4D()

For indexA = 1 To 2
    For indexB = 1 To 3
        For indexC = 1 To 4
            For indexD = 1 To 5
                If result4D(indexA, indexB, indexC, indexD) <> _
                    indexA * 1000000 + _
                    indexB * 10000 + _
                    indexC * 100 + _
                    indexD Then
                    TestOk = 0
                End If
            Next
        Next
    Next
Next
If TestOk Then Print "PASS" Else Print "FAIL"
System
