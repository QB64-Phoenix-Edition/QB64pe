$CONSOLE:ONLY
$ERRORLOCATION:ON
OPTION _EXPLICIT

' PR #779: the built-in _MEM and user FUNCTION ... AS _MEM must coexist.

TYPE LeafRec
    secondValue AS LONG
END TYPE

TYPE PairRec
    firstValue AS LONG
    leaf AS LeafRec
END TYPE

DIM SHARED testStage AS STRING
DIM mem AS _MEM, otherBlock AS _MEM, freshBlock AS _MEM
DIM directBlock AS _MEM, wholeView AS _MEM, elementView AS _MEM
DIM cells(0 TO 2) AS LONG
DIM roundIndex AS LONG, readValue AS LONG, exprValue AS LONG
DIM blockType AS _OFFSET, savedAddress AS _OFFSET

testStage = "startup"
ON ERROR GOTO RuntimeFailure

' 01. Legacy _MEMNEW and ordinary dotted fields are the control path.
testStage = "01 direct _MEMNEW"
directBlock = _MEMNEW(16)
CheckCase _MEMEXISTS(directBlock) <> 0, "allocation did not produce a live block"
CheckCase directBlock.SIZE = 16, "SIZE should be 16"
CheckCase directBlock.ELEMENTSIZE = 1, "ELEMENTSIZE should be 1"
CheckCase directBlock.OFFSET <> 0, "OFFSET should be nonzero"
CheckCase directBlock.IMAGE = -1, "IMAGE should be -1 for a raw allocation"
blockType = directBlock.TYPE
_MEMPUT directBlock, directBlock.OFFSET + 4, 123456 AS LONG
readValue = _MEMGET(directBlock, directBlock.OFFSET + 4, LONG)
CheckCase readValue = 123456, "typed write/read did not round-trip"
_MEMFREE directBlock
CheckCase _MEMEXISTS(directBlock) = 0, "_MEMFREE did not invalidate the block"

' 02. Two owned blocks from one syntactic call site. The first must remain
' valid after the hidden result slot is reused for the second.
testStage = "02 AS _MEM, dotted wrapper and call-site reuse"
FOR roundIndex = 1 TO 2
    freshBlock = MemProbe.AllocBlock(16 * roundIndex)
    CheckCase _MEMEXISTS(freshBlock) <> 0, "returned block is not live"
    CheckCase freshBlock.SIZE = 16 * roundIndex, "wrong SIZE after return"
    CheckCase freshBlock.ELEMENTSIZE = 1, "wrong ELEMENTSIZE after return"
    CheckCase freshBlock.TYPE = blockType, "TYPE differs from direct _MEMNEW"
    _MEMPUT freshBlock, freshBlock.OFFSET, roundIndex AS LONG
    IF roundIndex = 1 THEN
        mem = freshBlock
        savedAddress = mem.OFFSET
    ELSE
        otherBlock = freshBlock
    END IF
NEXT
CheckCase mem.OFFSET = savedAddress AND mem.SIZE = 16, "first descriptor changed on reuse"
CheckCase otherBlock.OFFSET <> mem.OFFSET AND otherBlock.SIZE = 32, "two live allocations alias"
_MEMGET mem, mem.OFFSET, readValue
CheckCase readValue = 1, "first payload changed on reuse"
_MEMGET otherBlock, otherBlock.OFFSET, readValue
CheckCase readValue = 2, "second payload is wrong"

' 03. Direct member access on an AS _MEM result. EchoBlock returns an alias
' of an already-live block, so reading the temporary does not lose ownership.
testStage = "03 dotted _MEM result members and nested arguments"
exprValue = MemProbe.EchoBlock(mem).SIZE + MemProbe.EchoBlock(otherBlock).ELEMENTSIZE
CheckCase exprValue = 17, "returned SIZE + ELEMENTSIZE should be 17"
CheckCase MemProbe.EchoBlock(mem).OFFSET = savedAddress, "returned OFFSET changed"
CheckCase MemProbe.EchoBlock(mem).TYPE = blockType, "returned TYPE changed"
CheckCase MemProbe.EchoBlock(mem).IMAGE = -1, "returned IMAGE changed"
exprValue = MemProbe.EchoBlock(MemProbe.EchoBlock(mem)).SIZE + mem.ELEMENTSIZE
CheckCase exprValue = 17, "nested AS _MEM argument lost its descriptor"
CheckCase MemProbe.EchoBlock(mem).SIZE + mem.ELEMENTSIZE = 17, "mixed result and variable members failed"
exprValue = MemProbe.EchoBlock(mem).SIZE + DotProbe.Whole(2).leaf.secondValue + mem.ELEMENTSIZE
CheckCase exprValue = 21, "_MEM and ordinary UDT members interfered in one expression"
_MEMFREE mem
CheckCase _MEMEXISTS(mem) = 0 AND _MEMEXISTS(otherBlock) <> 0, "freeing one block affected the other"
_MEMFREE otherBlock
CheckCase _MEMEXISTS(otherBlock) = 0, "second block was not invalidated"

' 04. _MEM views a whole LONG array, _MEMELEMENT views exactly one element.
testStage = "04 _MEM array view and _MEMELEMENT"
cells(0) = 11
cells(1) = 22
cells(2) = 33
wholeView = _MEM(cells())
elementView = _MEMELEMENT(cells(1))
CheckCase _MEMEXISTS(wholeView) <> 0 AND _MEMEXISTS(elementView) <> 0, "array views are not live"
CheckCase wholeView.SIZE = 12 AND wholeView.ELEMENTSIZE = 4, "whole LONG array metadata is wrong"
CheckCase elementView.SIZE = 4 AND elementView.ELEMENTSIZE = 4, "element view should span one LONG"
CheckCase elementView.OFFSET = wholeView.OFFSET + wholeView.ELEMENTSIZE, "element offset is wrong"
CheckCase MemProbe.EchoBlock(elementView).SIZE = elementView.SIZE, "element metadata changed across AS _MEM return"
_MEMGET elementView, elementView.OFFSET, readValue
CheckCase readValue = 22, "element view does not point to cells(1)"
_MEMPUT elementView, elementView.OFFSET, 77 AS LONG
CheckCase cells(0) = 11 AND cells(1) = 77 AND cells(2) = 33, "write did not stay in the selected element"
_MEMFREE elementView
_MEMFREE wholeView
CheckCase cells(1) = 77, "freeing array views changed the source array"

PRINT "PASS"
SYSTEM 0

RuntimeFailure:
PRINT "FAIL ["; testStage; "]: runtime error"; ERR; " at BASIC line"; _ERRORLINE; " - "; _ERRORMESSAGE$
SYSTEM 1

SUB CheckCase (passed AS LONG, detailText AS STRING)
    IF passed = 0 THEN
        PRINT "FAIL ["; testStage; "]: "; detailText
        SYSTEM 1
    END IF
END SUB

FUNCTION MakeMemory (byteCount AS LONG) AS _MEM
    MakeMemory = _MEMNEW(byteCount)
END FUNCTION

FUNCTION MemProbe.AllocBlock (byteCount AS LONG) AS _MEM
    MemProbe.AllocBlock = MakeMemory(byteCount)
END FUNCTION

FUNCTION MemProbe.EchoBlock (sourceBlock AS _MEM) AS _MEM
    MemProbe.EchoBlock = sourceBlock
END FUNCTION

FUNCTION DotProbe.Whole (seedValue AS LONG) AS PairRec
    DIM localPair AS PairRec
    localPair.firstValue = seedValue
    localPair.leaf.secondValue = seedValue * 2
    DotProbe.Whole = localPair
END FUNCTION
