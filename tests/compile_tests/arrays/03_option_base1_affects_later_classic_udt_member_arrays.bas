Option _Explicit

Type BeforeType
    Values(3) As Long
End Type

Option Base 1

Type AfterType
    Values(3) As Long
End Type

ReDim beforeItems(0) As BeforeType
ReDim afterItems(0) As AfterType

If LBound(beforeItems(0).Values) <> 0 Or UBound(beforeItems(0).Values) <> 3 Then
    Print "FAIL 03_option_base1_affects_later_classic_udt_member_arrays.bas: beforeItems bounds wrong"
    System
End If

If LBound(afterItems(0).Values) <> 1 Or UBound(afterItems(0).Values) <> 3 Then
    Print "FAIL 03_option_base1_affects_later_classic_udt_member_arrays.bas: afterItems bounds wrong"
    System
End If

Print "PASS 03_option_base1_affects_later_classic_udt_member_arrays.bas"
System
