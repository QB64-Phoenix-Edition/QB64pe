OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE ResultData
    callNumber AS LONG
    value AS LONG
END TYPE

DIM firstResult AS ResultData
DIM secondResult AS ResultData
DIM thirdResult AS ResultData

firstResult = MakeStaticValue(10)
secondResult = MakeStaticValue(20)
thirdResult = MakeStaticValue(30)

IF firstResult.callNumber <> 1 THEN
    PRINT "FAIL: first callNumber"
    SYSTEM 1
END IF

IF firstResult.value <> 10 THEN
    PRINT "FAIL: first value"
    SYSTEM 1
END IF

IF secondResult.callNumber <> 2 THEN
    PRINT "FAIL: second callNumber"
    SYSTEM 1
END IF

IF secondResult.value <> 40 THEN
    PRINT "FAIL: second value"
    SYSTEM 1
END IF

IF thirdResult.callNumber <> 3 THEN
    PRINT "FAIL: third callNumber"
    SYSTEM 1
END IF

IF thirdResult.value <> 90 THEN
    PRINT "FAIL: third value"
    SYSTEM 1
END IF

PRINT "PASS"
SYSTEM 0

FUNCTION MakeStaticValue (inputValue AS LONG) AS ResultData
    STATIC callNumber AS LONG

    callNumber = callNumber + 1

    MakeStaticValue.callNumber = callNumber
    MakeStaticValue.value = inputValue * callNumber
END FUNCTION
