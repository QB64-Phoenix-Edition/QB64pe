$CONSOLE:ONLY
OPTION BASE 1
DEFLNG A-Z

TYPE ParseNum
    typ AS LONG
END TYPE

TYPE PairType
    a AS LONG
    b AS SINGLE
END TYPE

TYPE TextItem
    number AS LONG
    textValue AS STRING
END TYPE

TYPE PairData
    code AS LONG
    amount AS SINGLE
END TYPE

ON ERROR GOTO ErrorHandler

Run001
Run002
Run003
Run004
Run005
Run006
Run007

ErrorHandler:
IF ERR <> 9 THEN PRINT "FAIL: t007 unexpected error"; ERR: SYSTEM 1
Run015


' Parser/name compatibility cases formerly t037-t039.
REDIM SHARED T(1 TO 8) AS INTEGER
DIM tempNum AS ParseNum
T(1) = 77
tempNum.typ = 1234
t = tempNum.typ
IF t <> 1234 THEN PRINT "FAIL: t037 scalar t": SYSTEM 1
IF T(1) <> 77 THEN PRINT "FAIL: t037 array T": SYSTEM 1

t = T(1)
IF t <> 77 THEN PRINT "FAIL: t038 shared array index to scalar": SYSTEM 1

DIM valueSet(0 TO 2) AS LONG
DIM valueSet AS LONG
valueSet = 19
valueSet(0) = 83
IF valueSet <> 19 THEN PRINT "FAIL: t039 scalar": SYSTEM 1
IF valueSet(0) <> 83 THEN PRINT "FAIL: t039 array": SYSTEM 1

PRINT "ALL PASS"
SYSTEM 0


SUB Run001
    DIM AS PairType srcPair(0 TO 5), dstPair(0 TO 5)
    DIM srcNum(0 TO 5) AS LONG
    DIM dstNum(0 TO 5) AS LONG
    DIM idx AS LONG

    srcPair(0).a = 1024
    srcPair(1).b = 3.1415

    FOR idx = 0 TO 5
        srcNum(idx) = idx + 1
    NEXT idx

    dstPair() = srcPair()
    dstNum() = srcNum()

    IF dstPair(0).a <> 1024 THEN PRINT "FAIL: t001 fixed UDT index 0": SYSTEM 1
    IF ABS(dstPair(1).b - 3.1415) > .0001 THEN PRINT "FAIL: t001 fixed UDT index 1": SYSTEM 1

    FOR idx = 0 TO 5
        IF dstNum(idx) <> idx + 1 THEN PRINT "FAIL: t001 numeric index"; idx: SYSTEM 1
    NEXT idx
END SUB

SUB Run002
    DIM srcText(0 TO 2) AS STRING
    DIM dstText(0 TO 2) AS STRING
    DIM srcFixed(0 TO 2) AS STRING * 8
    DIM dstFixed(0 TO 2) AS STRING * 8

    srcText(0) = "zero"
    srcText(1) = "one"
    srcText(2) = "two"

    srcFixed(0) = "A"
    srcFixed(1) = "BBBB"
    srcFixed(2) = "CCCCCCCC"

    dstText() = srcText()
    dstFixed() = srcFixed()

    srcText(1) = "changed"
    srcFixed(1) = "changed"

    IF dstText(0) <> "zero" THEN PRINT "FAIL: t002 variable STRING index 0": SYSTEM 1
    IF dstText(1) <> "one" THEN PRINT "FAIL: t002 variable STRING deep copy": SYSTEM 1
    IF dstText(2) <> "two" THEN PRINT "FAIL: t002 variable STRING index 2": SYSTEM 1
    IF dstFixed(0) <> "A" + SPACE$(7) THEN PRINT "FAIL: t002 fixed STRING index 0": SYSTEM 1
    IF dstFixed(1) <> "BBBB" + SPACE$(4) THEN PRINT "FAIL: t002 fixed STRING index 1": SYSTEM 1
    IF dstFixed(2) <> "CCCCCCCC" THEN PRINT "FAIL: t002 fixed STRING index 2": SYSTEM 1
END SUB

SUB Run003
    DIM srcGrid(2, 3) AS LONG
    DIM dstGrid(2, 3) AS LONG
    DIM rowIndex AS LONG
    DIM colIndex AS LONG

    FOR rowIndex = 1 TO 2
        FOR colIndex = 1 TO 3
            srcGrid(rowIndex, colIndex) = rowIndex * 100 + colIndex
        NEXT colIndex
    NEXT rowIndex

    dstGrid() = srcGrid()

    FOR rowIndex = 1 TO 2
        FOR colIndex = 1 TO 3
            IF dstGrid(rowIndex, colIndex) <> rowIndex * 100 + colIndex THEN
                PRINT "FAIL: t003 2D at"; rowIndex; colIndex
                SYSTEM 1
            END IF
        NEXT colIndex
    NEXT rowIndex
END SUB

SUB Run004
    DIM srcItem(0 TO 2) AS TextItem
    DIM dstItem(0 TO 2) AS TextItem

    srcItem(0).number = 10
    srcItem(0).textValue = "first"
    srcItem(1).number = 20
    srcItem(1).textValue = "second"
    srcItem(2).number = 30
    srcItem(2).textValue = "third"

    dstItem() = srcItem()

    srcItem(1).number = 999
    srcItem(1).textValue = "changed"

    IF dstItem(0).number <> 10 OR dstItem(0).textValue <> "first" THEN PRINT "FAIL: t004 UDT varstring index 0": SYSTEM 1
    IF dstItem(1).number <> 20 OR dstItem(1).textValue <> "second" THEN PRINT "FAIL: t004 UDT varstring deep copy": SYSTEM 1
    IF dstItem(2).number <> 30 OR dstItem(2).textValue <> "third" THEN PRINT "FAIL: t004 UDT varstring index 2": SYSTEM 1
END SUB

SUB Run005
    DIM srcValue(0 TO 5) AS LONG
    REDIM dstValue(0 TO 5) AS LONG
    DIM idx AS LONG

    FOR idx = 0 TO 5
        srcValue(idx) = 1000 + idx
    NEXT idx

    dstValue() = srcValue()

    FOR idx = 0 TO 5
        IF dstValue(idx) <> 1000 + idx THEN PRINT "FAIL: t005 DIM to REDIM index"; idx: SYSTEM 1
    NEXT idx
END SUB

SUB Run006
    DIM values(0 TO 3) AS LONG
    DIM idx AS LONG

    FOR idx = 0 TO 3
        values(idx) = idx * 11
    NEXT idx

    values() = values()

    FOR idx = 0 TO 3
        IF values(idx) <> idx * 11 THEN PRINT "FAIL: t006 self assignment index"; idx: SYSTEM 1
    NEXT idx
END SUB

SUB Run007


    DIM srcValue(0 TO 5) AS LONG
    DIM dstValue(1 TO 6) AS LONG

    dstValue() = srcValue()
    PRINT "FAIL: t007 bounds mismatch was accepted"
    SYSTEM 1

END SUB

SUB Run015
    DIM srcNum(0 TO 5) AS LONG
    DIM dstNum(0 TO 5) AS LONG
    DIM srcPair(0 TO 2) AS PairData
    DIM dstPair(0 TO 2) AS PairData
    DIM idx AS LONG

    FOR idx = 0 TO 5
        srcNum(idx) = 100 + idx
    NEXT idx

    dstNum() = srcNum()
    FOR idx = 0 TO 5
        IF dstNum(idx) <> 100 + idx THEN PRINT "FAIL: t015 numeric"; idx: SYSTEM 1
    NEXT idx

    dstNum(2) = srcNum(4)
    IF dstNum(2) <> 104 THEN PRINT "FAIL: t015 indexed": SYSTEM 1

    FOR idx = 0 TO 2
        srcPair(idx).code = 1000 + idx
        srcPair(idx).amount = idx + .25
    NEXT idx

    dstPair() = srcPair()
    FOR idx = 0 TO 2
        IF dstPair(idx).code <> 1000 + idx THEN PRINT "FAIL: t015 UDT code"; idx: SYSTEM 1
        IF ABS(dstPair(idx).amount - (idx + .25)) > .0001 THEN PRINT "FAIL: t015 UDT amount"; idx: SYSTEM 1
    NEXT idx
END SUB
