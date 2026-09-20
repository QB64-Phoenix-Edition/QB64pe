$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE OwnerLeaf
    Name AS STRING
    Tags(0 TO 7) AS STRING
    Value AS LONG
END TYPE

TYPE OwnerTree
    Leaves(0 TO 127) AS OwnerLeaf
    Labels(0 TO 31) AS STRING
    Marker AS LONG
END TYPE

DIM src(0 TO 1) AS OwnerTree
DIM dst(0 TO 1) AS OwnerTree

src(0).Leaves(0).Name = "leaf-0"
src(0).Leaves(63).Name = "leaf-63"
src(0).Leaves(127).Name = "leaf-127"
src(0).Leaves(63).Tags(7) = "tag-63-7"
src(0).Leaves(127).Value = 12700
src(0).Labels(31) = "label-31"
src(0).Marker = 9876

' This traverses an inline fixed array of nested owner UDT elements.
dst() = src()

IF dst(0).Leaves(0).Name <> "leaf-0" THEN GOTO failed
IF dst(0).Leaves(63).Name <> "leaf-63" THEN GOTO failed
IF dst(0).Leaves(127).Name <> "leaf-127" THEN GOTO failed
IF dst(0).Leaves(63).Tags(7) <> "tag-63-7" THEN GOTO failed
IF dst(0).Leaves(127).Value <> 12700 THEN GOTO failed
IF dst(0).Labels(31) <> "label-31" THEN GOTO failed
IF dst(0).Marker <> 9876 THEN GOTO failed

src(0).Leaves(63).Name = "changed"
src(0).Leaves(63).Tags(7) = "changed-tag"
src(0).Labels(31) = "changed-label"
IF dst(0).Leaves(63).Name <> "leaf-63" THEN GOTO failed
IF dst(0).Leaves(63).Tags(7) <> "tag-63-7" THEN GOTO failed
IF dst(0).Labels(31) <> "label-31" THEN GOTO failed

ERASE src
IF dst(0).Leaves(127).Name <> "leaf-127" THEN GOTO failed
IF dst(0).Leaves(63).Tags(7) <> "tag-63-7" THEN GOTO failed

PRINT "PASS 91_type_codegen_nested_inline_owner.bas"
SYSTEM

failed:
PRINT "FAIL 91_type_codegen_nested_inline_owner.bas"
SYSTEM
