$Console:Only
$Unstable:TypeFields
Type BoxType
    scalarVal As Long
    values(0 To 2) _Static As Long
End Type
Dim dstBox As BoxType
Dim srcBox As BoxType
dstBox.values() = srcBox.scalarVal
