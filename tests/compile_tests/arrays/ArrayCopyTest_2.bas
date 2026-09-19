$Console:Only
$Unstable:TypeFields

Type InnerData
    _Dynamic values(1 To 4, 1 To 5) As Long
End Type

Type OuterData
    inner As InnerData
End Type

Type GroupData
    items(1 To 3) As InnerData
    nothing As String
End Type

Dim testOk As _Byte
Dim rowIndex As Long
Dim colIndex As Long

testOk = -1

' Test 1:
' Multidimensional top-level arrays.
'
' Verify that a selected 2D source range is copied to the same
' coordinates in the destination and that surrounding elements
' remain untouched.

Dim sourceArray(1 To 4, 1 To 5) As Long
Dim targetArray(1 To 4, 1 To 5) As Long

For rowIndex = 1 To 4
    For colIndex = 1 To 5
        sourceArray(rowIndex, colIndex) = rowIndex * 100 + colIndex
        targetArray(rowIndex, colIndex) = -1
    Next
Next

_ArrayCopy sourceArray(2 To 3, 2 To 4) To targetArray()

For rowIndex = 1 To 4
    For colIndex = 1 To 5
        If rowIndex >= 2 And rowIndex <= 3 And colIndex >= 2 And colIndex <= 4 Then
            If targetArray(rowIndex, colIndex) <> rowIndex * 100 + colIndex Then
                testOk = 0
            End If
        Else
            If targetArray(rowIndex, colIndex) <> -1 Then
                testOk = 0
            End If
        End If
    Next
Next


' Test 2:
' Multidimensional _Dynamic array nested inside scalar UDTs.
'
' This verifies member-chain resolution together with the
' multidimensional range-to-whole copy path.

Dim sourceData As OuterData
Dim targetData As OuterData

For rowIndex = 1 To 4
    For colIndex = 1 To 5
        sourceData.inner.values(rowIndex, colIndex) = rowIndex * 1000 + colIndex
        targetData.inner.values(rowIndex, colIndex) = -2
    Next
Next

_ArrayCopy sourceData.inner.values(2 To 4, 1 To 3) To targetData.inner.values()

For rowIndex = 1 To 4
    For colIndex = 1 To 5
        If rowIndex >= 2 And rowIndex <= 4 And colIndex >= 1 And colIndex <= 3 Then
            If targetData.inner.values(rowIndex, colIndex) <> rowIndex * 1000 + colIndex Then
                testOk = 0
            End If
        Else
            If targetData.inner.values(rowIndex, colIndex) <> -2 Then
                testOk = 0
            End If
        End If
    Next
Next

' Test 3:
' Parent UDT array element -> inline UDT member array element
' -> multidimensional _Dynamic leaf array.
'
' This exercises the complete nested operand chain:
'
'   parentArray(index).items(index).values(...)
'
' and verifies that source and destination may use different
' parent/member indices without affecting the copied array
' coordinates.
Dim sourceGroups(1 To 2) As GroupData
Dim targetGroups(1 To 2) As GroupData

For rowIndex = 1 To 4
    For colIndex = 1 To 5
        sourceGroups(2).items(3).values(rowIndex, colIndex) = rowIndex * 10000 + colIndex
        targetGroups(1).items(2).values(rowIndex, colIndex) = -3
    Next
Next

_ArrayCopy sourceGroups(2).items(3).values(2 To 4, 2 To 5) To targetGroups(1).items(2).values()

For rowIndex = 1 To 4
    For colIndex = 1 To 5
        If rowIndex >= 2 And rowIndex <= 4 And colIndex >= 2 And colIndex <= 5 Then
            If targetGroups(1).items(2).values(rowIndex, colIndex) <> _
                rowIndex * 10000 + colIndex Then
                testOk = 0
            End If
        Else
            If targetGroups(1).items(2).values(rowIndex, colIndex) <> -3 Then
                testOk = 0
            End If
        End If
    Next
Next


If testOk Then
    Print "PASS"
Else
    Print "FAIL"
End If

System

