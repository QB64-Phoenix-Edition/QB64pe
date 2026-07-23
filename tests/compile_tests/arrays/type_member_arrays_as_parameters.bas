$Console:Only
$Unstable:TypeFields

Type ParamLeaf
    fixedValues(0 To 2) _Static As Long
    dynamicValues(1) _Dynamic As Long
    labels(1) _Dynamic As String
End Type

Type ParamRoot
    leaves(1) _Dynamic As ParamLeaf
End Type

Dim work As ParamRoot
ReDim work.leaves(0 To 1)
ReDim work.leaves(0).dynamicValues(1 To 3)
ReDim work.leaves(0).labels(1 To 2)

work.leaves(0).fixedValues(0) = 1
work.leaves(0).fixedValues(1) = 2
work.leaves(0).fixedValues(2) = 3
work.leaves(0).dynamicValues(1) = 10
work.leaves(0).dynamicValues(2) = 20
work.leaves(0).dynamicValues(3) = 30

AddOne work.leaves(0).fixedValues()
AddOne work.leaves(0).dynamicValues()
SetLabels work.leaves(0).labels()
TouchLeaves work.leaves()

Dim fixedTotal As Long
Dim dynamicTotal As Long
fixedTotal = SumValues&(work.leaves(0).fixedValues())
dynamicTotal = SumValues&(work.leaves(0).dynamicValues())

If fixedTotal <> 9 Then Print "FAIL fixed numeric parameter": System 1
If dynamicTotal <> 63 Then Print "FAIL dynamic numeric parameter": System 1
If work.leaves(0).labels(1) <> "red" Then Print "FAIL dynamic string parameter 1": System 1
If work.leaves(0).labels(2) <> "blue" Then Print "FAIL dynamic string parameter 2": System 1
If work.leaves(1).fixedValues(2) <> 77 Then Print "FAIL UDT member array parameter": System 1

Print "PASS type_member_arrays_as_parameters"
System

Sub AddOne (items() As Long)
    Dim i As Long
    For i = LBound(items) To UBound(items)
        items(i) = items(i) + 1
    Next
End Sub

Sub SetLabels (items() As String)
    items(LBound(items)) = "red"
    items(UBound(items)) = "blue"
End Sub

Sub TouchLeaves (items() As ParamLeaf)
    items(UBound(items)).fixedValues(2) = 77
End Sub

Function SumValues& (items() As Long)
    Dim i As Long
    Dim totalValue As Long
    For i = LBound(items) To UBound(items)
        totalValue = totalValue + items(i)
    Next
    SumValues& = totalValue
End Function
