$CONSOLE:ONLY
$UNSTABLE:TYPEFIELDS
DEFLNG A-Z

TYPE NegLeaf
    valueId AS LONG
    samples(0 To 1) _DYNAMIC AS LONG
END TYPE

TYPE NegDynBranch
    leaves(0 To 1) _DYNAMIC AS NegLeaf
END TYPE

TYPE NegStaticBranch
    leaves(-2 To -1) _STATIC AS NegLeaf
END TYPE

TYPE StaticBox
    values(0 To 2) _STATIC AS LONG
    tags(0 To 1) _STATIC AS STRING * 8
END TYPE

TYPE DynamicBox
    values(0 To 1) _DYNAMIC AS LONG
    words(0 To 1) _DYNAMIC AS STRING
END TYPE

TYPE PlainLeaf
    leafId AS LONG
    score AS SINGLE
END TYPE

TYPE OwnerLeaf
    leafId AS LONG
    noteText AS STRING
    samples(0 To 1) _DYNAMIC AS LONG
END TYPE

TYPE BranchData
    fixedLeaf(0 To 1) _STATIC AS PlainLeaf
    ownedLeaf(0 To 1) _DYNAMIC AS OwnerLeaf
END TYPE

Run014
Run016
Run017
Run018

PRINT "ALL PASS"
SYSTEM 0

SUB Run014
    REDIM dynRoot(-1 TO 0) AS NegDynBranch
    DIM staticRoot AS NegStaticBranch

    ' Negative index in the top-level parent array.
    REDIM dynRoot(-1).leaves(-2 To -1)

    ' Negative index in an intermediate descriptor-backed UDT member array.
    REDIM dynRoot(-1).leaves(-2).samples(8 To 9)
    dynRoot(-1).leaves(-2).samples(8) = 81
    dynRoot(-1).leaves(-2).samples(9) = 91

    ' Negative index in an intermediate inline/_Static UDT member array.
    REDIM staticRoot.leaves(-2).samples(3 To 4)
    staticRoot.leaves(-2).samples(3) = 31
    staticRoot.leaves(-2).samples(4) = 41

    IF LBOUND(dynRoot(-1).leaves) <> -2 OR UBOUND(dynRoot(-1).leaves) <> -1 THEN PRINT "FAIL: t014 dynamic leaf bounds": SYSTEM 1
    IF LBOUND(dynRoot(-1).leaves(-2).samples) <> 8 OR UBOUND(dynRoot(-1).leaves(-2).samples) <> 9 THEN PRINT "FAIL: t014 dynamic sample bounds": SYSTEM 1
    IF dynRoot(-1).leaves(-2).samples(8) <> 81 OR dynRoot(-1).leaves(-2).samples(9) <> 91 THEN PRINT "FAIL: t014 dynamic values": SYSTEM 1
    IF LBOUND(staticRoot.leaves(-2).samples) <> 3 OR UBOUND(staticRoot.leaves(-2).samples) <> 4 THEN PRINT "FAIL: t014 static sample bounds": SYSTEM 1
    IF staticRoot.leaves(-2).samples(3) <> 31 OR staticRoot.leaves(-2).samples(4) <> 41 THEN PRINT "FAIL: t014 static values": SYSTEM 1
END SUB

SUB Run016
    DIM srcBox AS StaticBox
    DIM dstBox AS StaticBox
    DIM normalVals(0 TO 2) AS LONG

    srcBox.values(0) = 11
    srcBox.values(1) = 22
    srcBox.values(2) = 33
    srcBox.tags(0) = "alpha"
    srcBox.tags(1) = "beta"

    dstBox.values() = srcBox.values()
    dstBox.tags() = srcBox.tags()

    IF dstBox.values(0) <> 11 OR dstBox.values(1) <> 22 OR dstBox.values(2) <> 33 THEN PRINT "FAIL: t016 member copy": SYSTEM 1
    IF RTRIM$(dstBox.tags(0)) <> "alpha" OR RTRIM$(dstBox.tags(1)) <> "beta" THEN PRINT "FAIL: t016 fixed strings": SYSTEM 1

    normalVals() = srcBox.values()
    IF normalVals(0) <> 11 OR normalVals(1) <> 22 OR normalVals(2) <> 33 THEN PRINT "FAIL: t016 member to normal": SYSTEM 1

    normalVals(0) = 71
    normalVals(1) = 72
    normalVals(2) = 73
    dstBox.values() = normalVals()
    IF dstBox.values(0) <> 71 OR dstBox.values(1) <> 72 OR dstBox.values(2) <> 73 THEN PRINT "FAIL: t016 normal to member": SYSTEM 1

    srcBox.values(0) = -1
    IF dstBox.values(0) <> 71 THEN PRINT "FAIL: t016 independence": SYSTEM 1
END SUB

SUB Run017
    DIM srcBox AS DynamicBox
    DIM dstBox AS DynamicBox
    DIM normalWords(2 TO 3) AS STRING

    REDIM srcBox.values(-2 To 0)
    REDIM dstBox.values(-2 To 0)
    REDIM srcBox.words(2 To 3)
    REDIM dstBox.words(2 To 3)

    srcBox.values(-2) = 12
    srcBox.values(-1) = 13
    srcBox.values(0) = 14
    srcBox.words(2) = "dynamic two"
    srcBox.words(3) = "dynamic three"

    dstBox.values() = srcBox.values()
    dstBox.words() = srcBox.words()

    IF LBOUND(dstBox.values) <> -2 OR UBOUND(dstBox.values) <> 0 THEN PRINT "FAIL: t017 numeric bounds": SYSTEM 1
    IF dstBox.values(-2) <> 12 OR dstBox.values(-1) <> 13 OR dstBox.values(0) <> 14 THEN PRINT "FAIL: t017 numeric payload": SYSTEM 1
    IF dstBox.words(2) <> "dynamic two" OR dstBox.words(3) <> "dynamic three" THEN PRINT "FAIL: t017 string payload": SYSTEM 1

    normalWords() = srcBox.words()
    IF normalWords(2) <> "dynamic two" OR normalWords(3) <> "dynamic three" THEN PRINT "FAIL: t017 member to normal": SYSTEM 1

    normalWords(2) = "normal two"
    normalWords(3) = "normal three"
    dstBox.words() = normalWords()
    IF dstBox.words(2) <> "normal two" OR dstBox.words(3) <> "normal three" THEN PRINT "FAIL: t017 normal to member": SYSTEM 1

    srcBox.words(2) = "changed source"
    IF dstBox.words(2) <> "normal two" THEN PRINT "FAIL: t017 deep copy": SYSTEM 1
END SUB

SUB Run018
    DIM srcBranch AS BranchData
    DIM dstBranch AS BranchData

    REDIM srcBranch.ownedLeaf(0 To 1)
    REDIM dstBranch.ownedLeaf(0 To 1)
    REDIM srcBranch.ownedLeaf(0).samples(4 To 5)
    REDIM srcBranch.ownedLeaf(1).samples(7 To 8)
    REDIM dstBranch.ownedLeaf(0).samples(4 To 5)
    REDIM dstBranch.ownedLeaf(1).samples(7 To 8)

    srcBranch.fixedLeaf(0).leafId = 10
    srcBranch.fixedLeaf(0).score = 1.5
    srcBranch.fixedLeaf(1).leafId = 20
    srcBranch.fixedLeaf(1).score = 2.5

    srcBranch.ownedLeaf(0).leafId = 101
    srcBranch.ownedLeaf(0).noteText = "owner zero"
    srcBranch.ownedLeaf(0).samples(4) = 41
    srcBranch.ownedLeaf(0).samples(5) = 42
    srcBranch.ownedLeaf(1).leafId = 102
    srcBranch.ownedLeaf(1).noteText = "owner one"
    srcBranch.ownedLeaf(1).samples(7) = 71
    srcBranch.ownedLeaf(1).samples(8) = 72

    dstBranch.fixedLeaf() = srcBranch.fixedLeaf()
    dstBranch.ownedLeaf() = srcBranch.ownedLeaf()

    IF dstBranch.fixedLeaf(0).leafId <> 10 OR dstBranch.fixedLeaf(1).leafId <> 20 THEN PRINT "FAIL: t018 static UDT array": SYSTEM 1
    IF dstBranch.ownedLeaf(0).leafId <> 101 OR dstBranch.ownedLeaf(1).leafId <> 102 THEN PRINT "FAIL: t018 dynamic UDT ids": SYSTEM 1
    IF dstBranch.ownedLeaf(0).noteText <> "owner zero" OR dstBranch.ownedLeaf(1).noteText <> "owner one" THEN PRINT "FAIL: t018 dynamic UDT strings": SYSTEM 1
    IF dstBranch.ownedLeaf(0).samples(4) <> 41 OR dstBranch.ownedLeaf(0).samples(5) <> 42 THEN PRINT "FAIL: t018 nested samples 0": SYSTEM 1
    IF dstBranch.ownedLeaf(1).samples(7) <> 71 OR dstBranch.ownedLeaf(1).samples(8) <> 72 THEN PRINT "FAIL: t018 nested samples 1": SYSTEM 1

    srcBranch.ownedLeaf(0).noteText = "changed source"
    REDIM _RETAIN srcBranch.ownedLeaf(0).samples(4 To 6)
    srcBranch.ownedLeaf(0).samples(4) = -1
    IF dstBranch.ownedLeaf(0).noteText <> "owner zero" THEN PRINT "FAIL: t018 string independence": SYSTEM 1
    IF UBOUND(dstBranch.ownedLeaf(0).samples) <> 5 OR dstBranch.ownedLeaf(0).samples(4) <> 41 THEN PRINT "FAIL: t018 descriptor independence": SYSTEM 1
END SUB
