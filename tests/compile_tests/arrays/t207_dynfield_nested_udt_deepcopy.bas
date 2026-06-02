$Console:Only

Type NodeT
    labelText As String
    samples(1) _DynamicField As Long
End Type

Type HolderT
    nodes(1) _DynamicField As NodeT
End Type

Dim failText As String
ReDim src(0 To 0) As HolderT
ReDim dst(0 To 0) As HolderT

ReDim src(0).nodes(0 To 1)

src(0).nodes(0).labelText = "source-a"
ReDim src(0).nodes(0).samples(0 To 1)
src(0).nodes(0).samples(0) = 41
src(0).nodes(0).samples(1) = 42

src(0).nodes(1).labelText = "source-b"
ReDim src(0).nodes(1).samples(2 To 3)
src(0).nodes(1).samples(2) = 52
src(0).nodes(1).samples(3) = 53

dst() = src()

src(0).nodes(0).labelText = "changed-a"
src(0).nodes(0).samples(1) = 9001
src(0).nodes(1).labelText = "changed-b"
src(0).nodes(1).samples(3) = 9002

If dst(0).nodes(0).labelText <> "source-a" And failText = "" Then failText = "copied nested string zero"
If LBound(dst(0).nodes(0).samples) <> 0 And failText = "" Then failText = "copied nested samples zero lower"
If UBound(dst(0).nodes(0).samples) <> 1 And failText = "" Then failText = "copied nested samples zero upper"
If dst(0).nodes(0).samples(1) <> 42 And failText = "" Then failText = "deep copy nested samples zero"

If dst(0).nodes(1).labelText <> "source-b" And failText = "" Then failText = "copied nested string one"
If LBound(dst(0).nodes(1).samples) <> 2 And failText = "" Then failText = "copied nested samples one lower"
If UBound(dst(0).nodes(1).samples) <> 3 And failText = "" Then failText = "copied nested samples one upper"
If dst(0).nodes(1).samples(3) <> 53 And failText = "" Then failText = "deep copy nested samples one"

If failText = "" Then
    Print "PASS t207_dynfield_nested_udt_deepcopy"
Else
    Print "FAIL t207_dynfield_nested_udt_deepcopy: "; failText
End If
System
