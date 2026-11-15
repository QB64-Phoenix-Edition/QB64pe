$CONSOLE:ONLY
$ERRORLOCATION:ON
$UNSTABLE:TYPEFIELDS
OPTION _EXPLICIT


TYPE LeafRec
    secondValue AS LONG
END TYPE

TYPE PairRec
    firstValue AS LONG
    leaf AS LeafRec
END TYPE

TYPE OwnedRec
    tagText AS STRING
    values(0) _DYNAMIC AS LONG
END TYPE

DIM SHARED testStage AS STRING
DIM SHARED pairCalls AS LONG
DIM SHARED ownedCalls AS LONG
DIM roundIndex AS LONG, previousValue AS LONG
DIM exprValue AS LONG, staticValue AS LONG
DIM firstPair AS PairRec, secondPair AS PairRec
DIM currentOwned AS OwnedRec, savedOwned AS OwnedRec
DIM legacy.total AS LONG

testStage = "startup"
ON ERROR GOTO RuntimeFailure

' 01. A dotted procedure name, a dotted legacy scalar, and UDT member dots
' must remain distinct. OPTION _EXPLICIT prevents accidental implicit names.
testStage = "01 dotted names and nested UDT result members"
firstPair = DotProbe.Make(4)
secondPair = DotProbe.Whole(7)
CheckCase firstPair.firstValue = 4 AND firstPair.leaf.secondValue = 8, "dotted result member assignment failed"
CheckCase secondPair.firstValue = 7 AND secondPair.leaf.secondValue = 14, "dotted whole-result assignment failed"
legacy.total = 10
previousValue = pairCalls
exprValue = legacy.total + DotProbe.Make(3).leaf.secondValue * 2 - DotProbe.Whole(5).firstValue
CheckCase exprValue = 17, "mixed dotted expression produced the wrong result"
CheckCase pairCalls = previousValue + 2, "a direct member expression evaluated its call more than once"

' 02. A returned UDT can be passed to FUNCTION and SUB by the same TYPE.
testStage = "02 nested UDT FUNCTION and SUB arguments"
secondPair = FlipPair(FlipPair(firstPair))
CheckCase secondPair.firstValue = 4 AND secondPair.leaf.secondValue = 8, "nested FlipPair changed the original pair"
previousValue = pairCalls
ReadPair DotProbe.Make(9), exprValue
CheckCase exprValue = 27, "SUB did not receive the temporary UDT correctly"
CheckCase pairCalls = previousValue + 1, "SUB argument call did not execute exactly once"

' 03. Resize the function's own result member and reuse its owning slot.
' The saved copy must survive subsequent calls with different bounds and text.
testStage = "03 dotted REDIM, descriptor reuse and deep copy"
FOR roundIndex = 1 TO 4
    currentOwned = DotProbe.Owned(roundIndex)
    CheckCase LBOUND(currentOwned.values) = 1, "wrong lower bound"
    CheckCase UBOUND(currentOwned.values) = roundIndex, "wrong upper bound after REDIM"
    CheckCase currentOwned.values(roundIndex) = roundIndex * 10, "wrong value in the last element"
    IF roundIndex = 1 THEN
        savedOwned = currentOwned
    ELSE
        CheckCase currentOwned.values(1) = 0, "old array contents leaked into the new result"
    END IF
    IF roundIndex MOD 2 THEN
        CheckCase currentOwned.tagText = "odd", "wrong text on odd call"
    ELSE
        CheckCase currentOwned.tagText = "", "old text leaked into an even call"
    END IF
NEXT
CheckCase UBOUND(savedOwned.values) = 1 AND savedOwned.values(1) = 10, "saved descriptor copy changed"
CheckCase savedOwned.tagText = "odd", "saved string copy changed"
CheckCase ownedCalls = 4, "owned call did not execute exactly once per iteration"
exprValue = DotProbe.Owned(3).values(3) + savedOwned.values(1)
CheckCase exprValue = 40 AND ownedCalls = 5, "direct dynamic member read failed or called twice"

' 04. Both alternate STATIC positions must preserve local state.
testStage = "04 leading and trailing STATIC"
exprValue = TrailingTick
CheckCase exprValue = 1, "trailing STATIC first call is wrong"
exprValue = TrailingTick
CheckCase exprValue = 2, "trailing STATIC did not retain state"
LeadingTick staticValue
CheckCase staticValue = 1, "leading STATIC first call is wrong"
LeadingTick staticValue
CheckCase staticValue = 2, "leading STATIC did not retain state"

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

FUNCTION DotProbe.Make (seedValue AS LONG) AS PairRec
    pairCalls = pairCalls + 1
    DotProbe.Make.firstValue = seedValue
    DotProbe.Make.leaf.secondValue = seedValue * 2
END FUNCTION

FUNCTION DotProbe.Whole (seedValue AS LONG) AS PairRec
    DIM localPair AS PairRec
    pairCalls = pairCalls + 1
    localPair.firstValue = seedValue
    localPair.leaf.secondValue = seedValue * 2
    DotProbe.Whole = localPair
END FUNCTION

FUNCTION FlipPair (sourcePair AS PairRec) AS PairRec
    FlipPair.firstValue = sourcePair.leaf.secondValue
    FlipPair.leaf.secondValue = sourcePair.firstValue
END FUNCTION

SUB ReadPair (sourcePair AS PairRec, resultValue AS LONG)
    resultValue = sourcePair.firstValue + sourcePair.leaf.secondValue
END SUB

FUNCTION DotProbe.Owned (stepNumber AS LONG) AS OwnedRec
    ownedCalls = ownedCalls + 1
    REDIM DotProbe.Owned.values(1 TO stepNumber)
    DotProbe.Owned.values(stepNumber) = stepNumber * 10
    IF stepNumber MOD 2 THEN DotProbe.Owned.tagText = "odd"
END FUNCTION

FUNCTION TrailingTick AS LONG STATIC
    DIM ticks AS LONG
    ticks = ticks + 1
    TrailingTick = ticks
END FUNCTION

STATIC SUB LeadingTick (resultValue AS LONG)
    DIM ticks AS LONG
    ticks = ticks + 1
    resultValue = ticks
END SUB
