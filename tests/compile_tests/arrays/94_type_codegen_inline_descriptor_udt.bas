$CONSOLE:ONLY
$UNSTABLE:TYPEFIELDS
OPTION _EXPLICIT

TYPE DescriptorLeaf
    Values(0 TO 1) _DYNAMIC AS LONG
    Tag AS LONG
END TYPE

TYPE DescriptorRoot
    Leaves(0 TO 127) AS DescriptorLeaf
    Guard AS LONG
END TYPE

REDIM src(0 TO 0) AS DescriptorRoot
REDIM dst(0 TO 0) AS DescriptorRoot

' The parent contains a large fixed inline member array. Each element contains
' a descriptor-backed member, so descriptor init/free/clone must traverse Leaves().
REDIM src(0).Leaves(0).Values(-2 TO 4)
REDIM src(0).Leaves(63).Values(10 TO 20)
REDIM src(0).Leaves(127).Values(0 TO 5)
src(0).Leaves(0).Values(-2) = 102
src(0).Leaves(63).Values(20) = 6320
src(0).Leaves(127).Values(5) = 12705
src(0).Leaves(63).Tag = 6300
src(0).Guard = 777

dst() = src()

IF LBOUND(dst(0).Leaves(0).Values) <> -2 THEN GOTO failed
IF UBOUND(dst(0).Leaves(0).Values) <> 4 THEN GOTO failed
IF dst(0).Leaves(0).Values(-2) <> 102 THEN GOTO failed
IF dst(0).Leaves(63).Values(20) <> 6320 THEN GOTO failed
IF dst(0).Leaves(127).Values(5) <> 12705 THEN GOTO failed
IF dst(0).Leaves(63).Tag <> 6300 THEN GOTO failed
IF dst(0).Guard <> 777 THEN GOTO failed

src(0).Leaves(63).Values(20) = -1
src(0).Leaves(63).Tag = -2
IF dst(0).Leaves(63).Values(20) <> 6320 THEN GOTO failed
IF dst(0).Leaves(63).Tag <> 6300 THEN GOTO failed

ERASE src
IF dst(0).Leaves(127).Values(5) <> 12705 THEN GOTO failed
ERASE dst

PRINT "PASS 94_type_codegen_inline_descriptor_udt.bas"
SYSTEM

failed:
PRINT "FAIL 94_type_codegen_inline_descriptor_udt.bas"
SYSTEM
