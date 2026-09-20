$CONSOLE:ONLY
$UNSTABLE:TYPEFIELDS
OPTION _EXPLICIT

TYPE DynamicOwnerLeaf
    Name AS STRING
    Samples(0 TO 1) _DYNAMIC AS LONG
END TYPE

TYPE DynamicOwnerRoot
    Items(0 TO 1) _DYNAMIC AS DynamicOwnerLeaf
    Note AS STRING
END TYPE

REDIM src(0 TO 0) AS DynamicOwnerRoot
REDIM dst(0 TO 0) AS DynamicOwnerRoot
DIM i AS LONG

REDIM src(0).Items(0 TO 31)
FOR i = 0 TO 31
    src(0).Items(i).Name = "item-" + LTRIM$(STR$(i))
    REDIM src(0).Items(i).Samples(-1 TO 3)
    src(0).Items(i).Samples(-1) = i * 10 + 1
    src(0).Items(i).Samples(3) = i * 10 + 3
NEXT
src(0).Note = "root-note"

' Clone a _DYNAMIC UDT array whose elements own both STRING and another descriptor.
dst() = src()

IF LBOUND(dst(0).Items) <> 0 THEN GOTO failed
IF UBOUND(dst(0).Items) <> 31 THEN GOTO failed
IF dst(0).Items(17).Name <> "item-17" THEN GOTO failed
IF LBOUND(dst(0).Items(17).Samples) <> -1 THEN GOTO failed
IF UBOUND(dst(0).Items(17).Samples) <> 3 THEN GOTO failed
IF dst(0).Items(17).Samples(-1) <> 171 THEN GOTO failed
IF dst(0).Items(17).Samples(3) <> 173 THEN GOTO failed
IF dst(0).Note <> "root-note" THEN GOTO failed

src(0).Items(17).Name = "changed"
src(0).Items(17).Samples(3) = -999
IF dst(0).Items(17).Name <> "item-17" THEN GOTO failed
IF dst(0).Items(17).Samples(3) <> 173 THEN GOTO failed

' Grow the owner array itself. Existing nested owners must survive _RETAIN.
REDIM _RETAIN src(0).Items(0 TO 47)
IF src(0).Items(17).Name <> "changed" THEN GOTO failed
IF src(0).Items(17).Samples(3) <> -999 THEN GOTO failed
src(0).Items(47).Name = "item-47"
REDIM src(0).Items(47).Samples(0 TO 2)
src(0).Items(47).Samples(2) = 4702
IF src(0).Items(47).Samples(2) <> 4702 THEN GOTO failed

ERASE src(0).Items
IF dst(0).Items(17).Name <> "item-17" THEN GOTO failed
IF dst(0).Items(17).Samples(3) <> 173 THEN GOTO failed

ERASE src
ERASE dst
PRINT "PASS 93_type_codegen_dynamic_owner_udt.bas"
SYSTEM

failed:
PRINT "FAIL 93_type_codegen_dynamic_owner_udt.bas"
SYSTEM
