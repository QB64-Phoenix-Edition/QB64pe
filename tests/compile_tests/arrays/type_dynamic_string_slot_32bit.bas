$Console:Only
$Unstable:TypeFields
Type Leaf
    labels(1) _Dynamic As String
End Type
Dim work As Leaf
ReDim work.labels(1 To 2)
work.labels(1) = "red"
work.labels(2) = "blue"
If work.labels(1) <> "red" Then Print "FAIL 1": System 1
If work.labels(2) <> "blue" Then Print "FAIL 2": System 1
Print "PASS string_direct_min"
System 

