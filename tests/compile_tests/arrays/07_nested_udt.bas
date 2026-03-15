OPTION _EXPLICIT


DIM SHARED PassCount AS LONG
DIM SHARED FailCount AS LONG


TYPE Cell
    Value AS LONG
    Note AS STRING
END TYPE

TYPE Board
    Grid( 0 To 1 , 0 To 2) AS INTEGER
    Cells( 0 To 1) AS Cell
    Name AS STRING * 6
END TYPE

TYPE World
    Boards( 0 To 1) AS Board
    Labels( 0 To 1) AS STRING
END TYPE

DIM a(0) AS World
DIM b(0) AS World

a(0).Boards(1).Grid(1, 2) = 123
a(0).Boards(0).Cells(1).Value = 456
a(0).Boards(0).Cells(1).Note = "ok"
a(0).Boards(1).Name = "BOARD2"
a(0).Labels(0) = "earth"
a(0).Labels(1) = "mars"

CheckTrue "nested grid value", a(0).Boards(1).Grid(1, 2) = 123
CheckTrue "nested cell value", a(0).Boards(0).Cells(1).Value = 456
CheckText "nested cell note", a(0).Boards(0).Cells(1).Note, "ok"
CheckText "board name", a(0).Boards(1).Name, "BOARD2"
CheckText "label 1", a(0).Labels(1), "mars"

b() = a()
CheckTrue "copy nested grid", b(0).Boards(1).Grid(1, 2) = 123
CheckText "copy nested note", b(0).Boards(0).Cells(1).Note, "ok"
CheckText "copy label", b(0).Labels(0), "earth"

ERASE a ( 0 ) . Boards ( 1 ) . Grid
CheckTrue "ERASE nested grid resets value", a(0).Boards(1).Grid(1, 2) = 0
CheckText "ERASE nested grid keeps board name", a(0).Boards(1).Name, "BOARD2"
CheckText "ERASE nested grid keeps world label", a(0).Labels(1), "mars"

ERASE a
CheckText "ERASE parent nested string", a(0).Boards(0).Cells(1).Note, ""
CheckTrue "ERASE parent nested numeric", a(0).Boards(0).Cells(1).Value = 0
CheckText "ERASE parent outer label", a(0).Labels(0), ""

FinishTest
SYSTEM

SUB CheckTrue (label AS STRING, condition AS LONG)
    IF condition THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label
    END IF
END SUB

SUB CheckText (label AS STRING, actual AS STRING, expected AS STRING)
    IF actual = expected THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label; " expected=["; expected; "] actual=["; actual; "]"
    END IF
END SUB

SUB CheckNear (label AS STRING, actual AS DOUBLE, expected AS DOUBLE, tolerance AS DOUBLE)
    IF ABS(actual - expected) <= tolerance THEN
        PassCount = PassCount + 1
    ELSE
        FailCount = FailCount + 1
        PRINT "FAIL:"; label; " expected="; expected; " actual="; actual
    END IF
END SUB

SUB FinishTest
    IF FailCount = 0 THEN
        PRINT "RESULT: PASS"
    ELSE
        PRINT "RESULT: FAIL"; FailCount; "failure(s)"
    END IF
END SUB
