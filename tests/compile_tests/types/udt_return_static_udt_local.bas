OPTION _EXPLICIT
$CONSOLE:ONLY

TYPE StateData
    count AS LONG
    total AS LONG
END TYPE

DIM firstResult AS StateData
DIM secondResult AS StateData
DIM thirdResult AS StateData

firstResult = AccumulateValue(5)
secondResult = AccumulateValue(7)
thirdResult = AccumulateValue(-2)

IF firstResult.count <> 1 THEN
    PRINT "FAIL: first count"
    SYSTEM 1
END IF

IF firstResult.total <> 5 THEN
    PRINT "FAIL: first total"
    SYSTEM 1
END IF

IF secondResult.count <> 2 THEN
    PRINT "FAIL: second count"
    SYSTEM 1
END IF

IF secondResult.total <> 12 THEN
    PRINT "FAIL: second total"
    SYSTEM 1
END IF

IF thirdResult.count <> 3 THEN
    PRINT "FAIL: third count"
    SYSTEM 1
END IF

IF thirdResult.total <> 10 THEN
    PRINT "FAIL: third total"
    SYSTEM 1
END IF

PRINT "PASS"
SYSTEM 0

FUNCTION AccumulateValue (inputValue AS LONG) AS StateData
    STATIC state AS StateData

    state.count = state.count + 1
    state.total = state.total + inputValue

    AccumulateValue = state
END FUNCTION
