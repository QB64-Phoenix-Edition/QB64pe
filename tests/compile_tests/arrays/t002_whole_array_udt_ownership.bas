$CONSOLE:ONLY
$UNSTABLE:TYPEFIELDS
DEFLNG A-Z

TYPE StaticOnly
    recordId AS LONG
    numbers(0 To 3) _STATIC AS LONG
    labels(1 To 2) _STATIC AS STRING * 8
END TYPE

TYPE DynamicOnly
    numbers(0 To 1) _DYNAMIC AS LONG
    texts(0 To 1) _DYNAMIC AS STRING
END TYPE

TYPE MixedFields
    recordId AS LONG
    fixedNumbers(0 To 2) _STATIC AS LONG
    fixedNames(0 To 1) _STATIC AS STRING * 10
    dynNumbers(0 To 1) _DYNAMIC AS SINGLE
    dynTexts(0 To 1) _DYNAMIC AS STRING
END TYPE

TYPE NestedLeaf
    leafId AS LONG
    fixedData(0 To 1) _STATIC AS INTEGER
    dynData(0 To 1) _DYNAMIC AS LONG
    noteText AS STRING
END TYPE

TYPE NestedRoot
    rootId AS LONG
    leaf AS NestedLeaf
    weight AS DOUBLE
END TYPE

TYPE FixedLeaf
    leafId AS LONG
    samples(0 To 2) _STATIC AS LONG
    tagText AS STRING * 8
END TYPE

TYPE FixedBranch
    branchId AS LONG
    leafSet(0 To 2) _STATIC AS FixedLeaf
END TYPE

TYPE DynamicLeaf
    leafId AS LONG
    samples(0 To 1) _DYNAMIC AS LONG
    noteText AS STRING
END TYPE

TYPE DynamicBranch
    branchId AS LONG
    leafSet(0 To 1) _DYNAMIC AS DynamicLeaf
END TYPE

Run008
Run009
Run010
Run011
Run012
Run013

PRINT "ALL PASS"
SYSTEM 0

SUB Run008
    DIM srcData(0 TO 2) AS StaticOnly
    DIM dstData(0 TO 2) AS StaticOnly
    DIM slotIndex AS LONG
    DIM itemIndex AS LONG

    FOR slotIndex = 0 TO 2
        srcData(slotIndex).recordId = 1000 + slotIndex
        FOR itemIndex = 0 TO 3
            srcData(slotIndex).numbers(itemIndex) = slotIndex * 100 + itemIndex
        NEXT itemIndex
        srcData(slotIndex).labels(1) = "A" + LTRIM$(STR$(slotIndex))
        srcData(slotIndex).labels(2) = "B" + LTRIM$(STR$(slotIndex))
    NEXT slotIndex

    dstData() = srcData()

    srcData(1).recordId = -1
    srcData(1).numbers(2) = -2
    srcData(1).labels(2) = "changed"

    FOR slotIndex = 0 TO 2
        IF dstData(slotIndex).recordId <> 1000 + slotIndex THEN PRINT "FAIL: t008 scalar"; slotIndex: SYSTEM 1
        FOR itemIndex = 0 TO 3
            IF dstData(slotIndex).numbers(itemIndex) <> slotIndex * 100 + itemIndex THEN PRINT "FAIL: t008 numbers"; slotIndex; itemIndex: SYSTEM 1
        NEXT itemIndex
        IF RTRIM$(dstData(slotIndex).labels(1)) <> "A" + LTRIM$(STR$(slotIndex)) THEN PRINT "FAIL: t008 label 1"; slotIndex: SYSTEM 1
        IF RTRIM$(dstData(slotIndex).labels(2)) <> "B" + LTRIM$(STR$(slotIndex)) THEN PRINT "FAIL: t008 label 2"; slotIndex: SYSTEM 1
    NEXT slotIndex
END SUB

SUB Run009
    REDIM srcData(0 TO 2) AS DynamicOnly
    REDIM dstData(0 TO 2) AS DynamicOnly

    REDIM srcData(0).numbers(-1 To 1)
    REDIM srcData(0).texts(2 To 3)
    srcData(0).numbers(-1) = 11
    srcData(0).numbers(0) = 12
    srcData(0).numbers(1) = 13
    srcData(0).texts(2) = "zero-two"
    srcData(0).texts(3) = "zero-three"

    REDIM srcData(1).numbers(5 To 7)
    REDIM srcData(1).texts(-2 To 0)
    srcData(1).numbers(5) = 21
    srcData(1).numbers(6) = 22
    srcData(1).numbers(7) = 23
    srcData(1).texts(-2) = "one-minus-two"
    srcData(1).texts(-1) = "one-minus-one"
    srcData(1).texts(0) = "one-zero"

    REDIM srcData(2).numbers(10 To 11)
    REDIM srcData(2).texts(4 To 4)
    srcData(2).numbers(10) = 31
    srcData(2).numbers(11) = 32
    srcData(2).texts(4) = "two-four"

    REDIM dstData(0).numbers(20 To 22)
    REDIM dstData(0).texts(20 To 21)
    dstData(0).texts(20) = "old destination"
    REDIM dstData(1).numbers(30 To 30)
    REDIM dstData(1).texts(30 To 30)
    REDIM dstData(2).numbers(40 To 43)
    REDIM dstData(2).texts(40 To 42)

    dstData() = srcData()

    IF LBOUND(dstData(0).numbers) <> -1 OR UBOUND(dstData(0).numbers) <> 1 THEN PRINT "FAIL: t009 bounds numbers 0": SYSTEM 1
    IF LBOUND(dstData(0).texts) <> 2 OR UBOUND(dstData(0).texts) <> 3 THEN PRINT "FAIL: t009 bounds texts 0": SYSTEM 1
    IF LBOUND(dstData(1).numbers) <> 5 OR UBOUND(dstData(1).numbers) <> 7 THEN PRINT "FAIL: t009 bounds numbers 1": SYSTEM 1
    IF LBOUND(dstData(1).texts) <> -2 OR UBOUND(dstData(1).texts) <> 0 THEN PRINT "FAIL: t009 bounds texts 1": SYSTEM 1
    IF LBOUND(dstData(2).numbers) <> 10 OR UBOUND(dstData(2).numbers) <> 11 THEN PRINT "FAIL: t009 bounds numbers 2": SYSTEM 1
    IF LBOUND(dstData(2).texts) <> 4 OR UBOUND(dstData(2).texts) <> 4 THEN PRINT "FAIL: t009 bounds texts 2": SYSTEM 1

    IF dstData(0).numbers(-1) <> 11 OR dstData(0).numbers(1) <> 13 THEN PRINT "FAIL: t009 data 0": SYSTEM 1
    IF dstData(1).numbers(5) <> 21 OR dstData(1).numbers(7) <> 23 THEN PRINT "FAIL: t009 data 1": SYSTEM 1
    IF dstData(2).numbers(10) <> 31 OR dstData(2).numbers(11) <> 32 THEN PRINT "FAIL: t009 data 2": SYSTEM 1
    IF dstData(0).texts(2) <> "zero-two" OR dstData(0).texts(3) <> "zero-three" THEN PRINT "FAIL: t009 text 0": SYSTEM 1
    IF dstData(1).texts(-2) <> "one-minus-two" OR dstData(1).texts(0) <> "one-zero" THEN PRINT "FAIL: t009 text 1": SYSTEM 1
    IF dstData(2).texts(4) <> "two-four" THEN PRINT "FAIL: t009 text 2": SYSTEM 1

    srcData(0).numbers(-1) = 999
    srcData(1).texts(0) = "changed"
    REDIM srcData(2).numbers(0 To 0)
    srcData(2).numbers(0) = 777

    IF dstData(0).numbers(-1) <> 11 THEN PRINT "FAIL: t009 numeric deep copy": SYSTEM 1
    IF dstData(1).texts(0) <> "one-zero" THEN PRINT "FAIL: t009 string deep copy": SYSTEM 1
    IF LBOUND(dstData(2).numbers) <> 10 OR UBOUND(dstData(2).numbers) <> 11 THEN PRINT "FAIL: t009 descriptor deep copy": SYSTEM 1
    IF dstData(2).numbers(10) <> 31 OR dstData(2).numbers(11) <> 32 THEN PRINT "FAIL: t009 descriptor data": SYSTEM 1
END SUB

SUB Run010
    DIM srcData(0 TO 1) AS MixedFields
    DIM dstData(0 TO 1) AS MixedFields
    DIM slotIndex AS LONG
    DIM itemIndex AS LONG

    FOR slotIndex = 0 TO 1
        srcData(slotIndex).recordId = 500 + slotIndex
        FOR itemIndex = 0 TO 2
            srcData(slotIndex).fixedNumbers(itemIndex) = slotIndex * 10 + itemIndex
        NEXT itemIndex
        srcData(slotIndex).fixedNames(0) = "fixed-a" + LTRIM$(STR$(slotIndex))
        srcData(slotIndex).fixedNames(1) = "fixed-b" + LTRIM$(STR$(slotIndex))
    NEXT slotIndex

    REDIM srcData(0).dynNumbers(-2 To 0)
    REDIM srcData(0).dynTexts(3 To 4)
    srcData(0).dynNumbers(-2) = 1.25
    srcData(0).dynNumbers(-1) = 2.5
    srcData(0).dynNumbers(0) = 3.75
    srcData(0).dynTexts(3) = "dynamic zero a"
    srcData(0).dynTexts(4) = "dynamic zero b"

    REDIM srcData(1).dynNumbers(7 To 8)
    REDIM srcData(1).dynTexts(-1 To 1)
    srcData(1).dynNumbers(7) = 7.5
    srcData(1).dynNumbers(8) = 8.5
    srcData(1).dynTexts(-1) = "dynamic one a"
    srcData(1).dynTexts(0) = "dynamic one b"
    srcData(1).dynTexts(1) = "dynamic one c"

    REDIM dstData(0).dynNumbers(20 To 20)
    REDIM dstData(0).dynTexts(20 To 20)
    REDIM dstData(1).dynNumbers(30 To 32)
    REDIM dstData(1).dynTexts(30 To 31)

    dstData() = srcData()

    FOR slotIndex = 0 TO 1
        IF dstData(slotIndex).recordId <> 500 + slotIndex THEN PRINT "FAIL: t010 scalar"; slotIndex: SYSTEM 1
        FOR itemIndex = 0 TO 2
            IF dstData(slotIndex).fixedNumbers(itemIndex) <> slotIndex * 10 + itemIndex THEN PRINT "FAIL: t010 static numbers"; slotIndex; itemIndex: SYSTEM 1
        NEXT itemIndex
        IF RTRIM$(dstData(slotIndex).fixedNames(0)) <> "fixed-a" + LTRIM$(STR$(slotIndex)) THEN PRINT "FAIL: t010 static name 0"; slotIndex: SYSTEM 1
        IF RTRIM$(dstData(slotIndex).fixedNames(1)) <> "fixed-b" + LTRIM$(STR$(slotIndex)) THEN PRINT "FAIL: t010 static name 1"; slotIndex: SYSTEM 1
    NEXT slotIndex

    IF LBOUND(dstData(0).dynNumbers) <> -2 OR UBOUND(dstData(0).dynNumbers) <> 0 THEN PRINT "FAIL: t010 dynamic bounds 0": SYSTEM 1
    IF LBOUND(dstData(1).dynNumbers) <> 7 OR UBOUND(dstData(1).dynNumbers) <> 8 THEN PRINT "FAIL: t010 dynamic bounds 1": SYSTEM 1
    IF dstData(0).dynNumbers(-1) <> 2.5 THEN PRINT "FAIL: t010 dynamic number 0": SYSTEM 1
    IF dstData(1).dynNumbers(8) <> 8.5 THEN PRINT "FAIL: t010 dynamic number 1": SYSTEM 1
    IF dstData(0).dynTexts(4) <> "dynamic zero b" THEN PRINT "FAIL: t010 dynamic text 0": SYSTEM 1
    IF dstData(1).dynTexts(1) <> "dynamic one c" THEN PRINT "FAIL: t010 dynamic text 1": SYSTEM 1

    srcData(0).fixedNumbers(1) = -100
    srcData(0).dynNumbers(-1) = -200
    srcData(1).dynTexts(1) = "changed"
    REDIM srcData(1).dynNumbers(0 To 0)

    IF dstData(0).fixedNumbers(1) <> 1 THEN PRINT "FAIL: t010 static independence": SYSTEM 1
    IF dstData(0).dynNumbers(-1) <> 2.5 THEN PRINT "FAIL: t010 dynamic independence": SYSTEM 1
    IF dstData(1).dynTexts(1) <> "dynamic one c" THEN PRINT "FAIL: t010 string independence": SYSTEM 1
    IF LBOUND(dstData(1).dynNumbers) <> 7 OR UBOUND(dstData(1).dynNumbers) <> 8 THEN PRINT "FAIL: t010 descriptor independence": SYSTEM 1
END SUB

SUB Run011
    REDIM srcData(0 TO 1) AS NestedRoot
    REDIM dstData(0 TO 1) AS NestedRoot

    srcData(0).rootId = 100
    srcData(0).weight = 1.5
    srcData(0).leaf.leafId = 101
    srcData(0).leaf.fixedData(0) = 11
    srcData(0).leaf.fixedData(1) = 12
    srcData(0).leaf.noteText = "root zero leaf"
    REDIM srcData(0).leaf.dynData(-1 To 1)
    srcData(0).leaf.dynData(-1) = 13
    srcData(0).leaf.dynData(0) = 14
    srcData(0).leaf.dynData(1) = 15

    srcData(1).rootId = 200
    srcData(1).weight = 2.5
    srcData(1).leaf.leafId = 201
    srcData(1).leaf.fixedData(0) = 21
    srcData(1).leaf.fixedData(1) = 22
    srcData(1).leaf.noteText = "root one leaf"
    REDIM srcData(1).leaf.dynData(5 To 6)
    srcData(1).leaf.dynData(5) = 23
    srcData(1).leaf.dynData(6) = 24

    REDIM dstData(0).leaf.dynData(20 To 21)
    dstData(0).leaf.noteText = "old zero"
    REDIM dstData(1).leaf.dynData(30 To 30)
    dstData(1).leaf.noteText = "old one"

    dstData() = srcData()

    IF dstData(0).rootId <> 100 OR dstData(0).leaf.leafId <> 101 THEN PRINT "FAIL: t011 scalar 0": SYSTEM 1
    IF dstData(1).rootId <> 200 OR dstData(1).leaf.leafId <> 201 THEN PRINT "FAIL: t011 scalar 1": SYSTEM 1
    IF dstData(0).leaf.fixedData(0) <> 11 OR dstData(0).leaf.fixedData(1) <> 12 THEN PRINT "FAIL: t011 fixed 0": SYSTEM 1
    IF dstData(1).leaf.fixedData(0) <> 21 OR dstData(1).leaf.fixedData(1) <> 22 THEN PRINT "FAIL: t011 fixed 1": SYSTEM 1
    IF dstData(0).leaf.noteText <> "root zero leaf" THEN PRINT "FAIL: t011 text 0": SYSTEM 1
    IF dstData(1).leaf.noteText <> "root one leaf" THEN PRINT "FAIL: t011 text 1": SYSTEM 1
    IF LBOUND(dstData(0).leaf.dynData) <> -1 OR UBOUND(dstData(0).leaf.dynData) <> 1 THEN PRINT "FAIL: t011 bounds 0": SYSTEM 1
    IF LBOUND(dstData(1).leaf.dynData) <> 5 OR UBOUND(dstData(1).leaf.dynData) <> 6 THEN PRINT "FAIL: t011 bounds 1": SYSTEM 1
    IF dstData(0).leaf.dynData(0) <> 14 OR dstData(1).leaf.dynData(6) <> 24 THEN PRINT "FAIL: t011 dynamic data": SYSTEM 1

    srcData(0).leaf.fixedData(0) = -1
    srcData(0).leaf.noteText = "changed"
    srcData(0).leaf.dynData(0) = -2
    REDIM srcData(1).leaf.dynData(0 To 0)

    IF dstData(0).leaf.fixedData(0) <> 11 THEN PRINT "FAIL: t011 fixed independence": SYSTEM 1
    IF dstData(0).leaf.noteText <> "root zero leaf" THEN PRINT "FAIL: t011 text independence": SYSTEM 1
    IF dstData(0).leaf.dynData(0) <> 14 THEN PRINT "FAIL: t011 dynamic independence": SYSTEM 1
    IF LBOUND(dstData(1).leaf.dynData) <> 5 OR UBOUND(dstData(1).leaf.dynData) <> 6 THEN PRINT "FAIL: t011 nested descriptor independence": SYSTEM 1
END SUB

SUB Run012
    DIM srcData(0 TO 1) AS FixedBranch
    DIM dstData(0 TO 1) AS FixedBranch
    DIM branchIndex AS LONG
    DIM leafIndex AS LONG
    DIM sampleIndex AS LONG

    FOR branchIndex = 0 TO 1
        srcData(branchIndex).branchId = 1000 + branchIndex
        FOR leafIndex = 0 TO 2
            srcData(branchIndex).leafSet(leafIndex).leafId = branchIndex * 100 + leafIndex
            srcData(branchIndex).leafSet(leafIndex).tagText = "T" + LTRIM$(STR$(branchIndex)) + LTRIM$(STR$(leafIndex))
            FOR sampleIndex = 0 TO 2
                srcData(branchIndex).leafSet(leafIndex).samples(sampleIndex) = branchIndex * 1000 + leafIndex * 10 + sampleIndex
            NEXT sampleIndex
        NEXT leafIndex
    NEXT branchIndex

    dstData() = srcData()

    srcData(1).leafSet(2).leafId = -1
    srcData(1).leafSet(2).samples(1) = -2
    srcData(1).leafSet(2).tagText = "changed"

    FOR branchIndex = 0 TO 1
        IF dstData(branchIndex).branchId <> 1000 + branchIndex THEN PRINT "FAIL: t012 branch"; branchIndex: SYSTEM 1
        FOR leafIndex = 0 TO 2
            IF dstData(branchIndex).leafSet(leafIndex).leafId <> branchIndex * 100 + leafIndex THEN PRINT "FAIL: t012 leaf"; branchIndex; leafIndex: SYSTEM 1
            IF RTRIM$(dstData(branchIndex).leafSet(leafIndex).tagText) <> "T" + LTRIM$(STR$(branchIndex)) + LTRIM$(STR$(leafIndex)) THEN PRINT "FAIL: t012 tag"; branchIndex; leafIndex: SYSTEM 1
            FOR sampleIndex = 0 TO 2
                IF dstData(branchIndex).leafSet(leafIndex).samples(sampleIndex) <> branchIndex * 1000 + leafIndex * 10 + sampleIndex THEN PRINT "FAIL: t012 sample"; branchIndex; leafIndex; sampleIndex: SYSTEM 1
            NEXT sampleIndex
        NEXT leafIndex
    NEXT branchIndex
END SUB

SUB Run013
    REDIM srcData(0 TO 1) AS DynamicBranch
    REDIM dstData(0 TO 1) AS DynamicBranch

    srcData(0).branchId = 100
    REDIM srcData(0).leafSet(1 To 2)
    srcData(0).leafSet(1).leafId = 101
    srcData(0).leafSet(1).noteText = "zero leaf one"
    REDIM srcData(0).leafSet(1).samples(-1 To 0)
    srcData(0).leafSet(1).samples(-1) = 11
    srcData(0).leafSet(1).samples(0) = 12
    srcData(0).leafSet(2).leafId = 102
    srcData(0).leafSet(2).noteText = "zero leaf two"
    REDIM srcData(0).leafSet(2).samples(5 To 7)
    srcData(0).leafSet(2).samples(5) = 13
    srcData(0).leafSet(2).samples(6) = 14
    srcData(0).leafSet(2).samples(7) = 15

    srcData(1).branchId = 200
    REDIM srcData(1).leafSet(-1 To 0)
    srcData(1).leafSet(-1).leafId = 201
    srcData(1).leafSet(-1).noteText = "one leaf minus one"
    REDIM srcData(1).leafSet(-1).samples(8 To 8)
    srcData(1).leafSet(-1).samples(8) = 21
    srcData(1).leafSet(0).leafId = 202
    srcData(1).leafSet(0).noteText = "one leaf zero"
    REDIM srcData(1).leafSet(0).samples(9 To 10)
    srcData(1).leafSet(0).samples(9) = 22
    srcData(1).leafSet(0).samples(10) = 23

    REDIM dstData(0).leafSet(20 To 20)
    dstData(0).leafSet(20).noteText = "old destination zero"
    REDIM dstData(0).leafSet(20).samples(20 To 21)
    REDIM dstData(1).leafSet(30 To 31)
    dstData(1).leafSet(30).noteText = "old destination one"
    REDIM dstData(1).leafSet(30).samples(30 To 30)

    dstData() = srcData()

    IF dstData(0).branchId <> 100 OR dstData(1).branchId <> 200 THEN PRINT "FAIL: t013 branch scalar": SYSTEM 1
    IF LBOUND(dstData(0).leafSet) <> 1 OR UBOUND(dstData(0).leafSet) <> 2 THEN PRINT "FAIL: t013 leaf bounds 0": SYSTEM 1
    IF LBOUND(dstData(1).leafSet) <> -1 OR UBOUND(dstData(1).leafSet) <> 0 THEN PRINT "FAIL: t013 leaf bounds 1": SYSTEM 1
    IF dstData(0).leafSet(1).leafId <> 101 OR dstData(0).leafSet(2).leafId <> 102 THEN PRINT "FAIL: t013 leaf ids 0": SYSTEM 1
    IF dstData(1).leafSet(-1).leafId <> 201 OR dstData(1).leafSet(0).leafId <> 202 THEN PRINT "FAIL: t013 leaf ids 1": SYSTEM 1
    IF dstData(0).leafSet(1).noteText <> "zero leaf one" THEN PRINT "FAIL: t013 note 0-1": SYSTEM 1
    IF dstData(0).leafSet(2).noteText <> "zero leaf two" THEN PRINT "FAIL: t013 note 0-2": SYSTEM 1
    IF dstData(1).leafSet(-1).noteText <> "one leaf minus one" THEN PRINT "FAIL: t013 note 1--1": SYSTEM 1
    IF dstData(1).leafSet(0).noteText <> "one leaf zero" THEN PRINT "FAIL: t013 note 1-0": SYSTEM 1
    IF LBOUND(dstData(0).leafSet(1).samples) <> -1 OR UBOUND(dstData(0).leafSet(1).samples) <> 0 THEN PRINT "FAIL: t013 sample bounds 0-1": SYSTEM 1
    IF LBOUND(dstData(0).leafSet(2).samples) <> 5 OR UBOUND(dstData(0).leafSet(2).samples) <> 7 THEN PRINT "FAIL: t013 sample bounds 0-2": SYSTEM 1
    IF LBOUND(dstData(1).leafSet(-1).samples) <> 8 OR UBOUND(dstData(1).leafSet(-1).samples) <> 8 THEN PRINT "FAIL: t013 sample bounds 1--1": SYSTEM 1
    IF LBOUND(dstData(1).leafSet(0).samples) <> 9 OR UBOUND(dstData(1).leafSet(0).samples) <> 10 THEN PRINT "FAIL: t013 sample bounds 1-0": SYSTEM 1
    IF dstData(0).leafSet(1).samples(-1) <> 11 OR dstData(0).leafSet(2).samples(7) <> 15 THEN PRINT "FAIL: t013 samples 0": SYSTEM 1
    IF dstData(1).leafSet(-1).samples(8) <> 21 OR dstData(1).leafSet(0).samples(10) <> 23 THEN PRINT "FAIL: t013 samples 1": SYSTEM 1

    srcData(0).leafSet(1).noteText = "changed"
    srcData(0).leafSet(2).samples(7) = 999
    REDIM srcData(1).leafSet(0).samples(0 To 0)
    REDIM srcData(0).leafSet(0 To 0)

    IF dstData(0).leafSet(1).noteText <> "zero leaf one" THEN PRINT "FAIL: t013 nested string independence": SYSTEM 1
    IF dstData(0).leafSet(2).samples(7) <> 15 THEN PRINT "FAIL: t013 nested data independence": SYSTEM 1
    IF LBOUND(dstData(1).leafSet(0).samples) <> 9 OR UBOUND(dstData(1).leafSet(0).samples) <> 10 THEN PRINT "FAIL: t013 nested descriptor independence": SYSTEM 1
    IF LBOUND(dstData(0).leafSet) <> 1 OR UBOUND(dstData(0).leafSet) <> 2 THEN PRINT "FAIL: t013 outer descriptor independence": SYSTEM 1
END SUB
