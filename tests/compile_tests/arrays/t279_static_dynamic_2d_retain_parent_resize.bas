$Console:Only
OPTION _EXPLICIT

TYPE T279
    fixedGrid(0 TO 1, 0 TO 1) _Static AS LONG
    dynGrid(0 TO 1, 0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 0) AS T279

work(0).fixedGrid(0, 0) = 11
work(0).fixedGrid(0, 1) = 12
work(0).fixedGrid(1, 0) = 21
work(0).fixedGrid(1, 1) = 22
work(0).dynGrid(0, 0) = 100
work(0).dynGrid(0, 1) = 101
work(0).dynGrid(1, 0) = 110
work(0).dynGrid(1, 1) = 111

REDIM _RETAIN work(0 TO 1) AS T279
work(1).fixedGrid(0, 0) = 31
work(1).fixedGrid(1, 1) = 44
work(1).dynGrid(0, 0) = 300
work(1).dynGrid(1, 1) = 311

REDIM _RETAIN work(0).dynGrid(0 TO 2, 0 TO 2)
work(0).dynGrid(2, 0) = 120
work(0).dynGrid(2, 2) = 122

ok = (LBOUND(work) = 0 AND UBOUND(work) = 1)
ok = ok AND (work(0).fixedGrid(0, 0) = 11 AND work(0).fixedGrid(1, 1) = 22)
ok = ok AND (LBOUND(work(0).dynGrid, 1) = 0 AND UBOUND(work(0).dynGrid, 1) = 2)
ok = ok AND (LBOUND(work(0).dynGrid, 2) = 0 AND UBOUND(work(0).dynGrid, 2) = 2)
ok = ok AND (work(0).dynGrid(0, 0) = 100 AND work(0).dynGrid(1, 1) = 111)
ok = ok AND (work(0).dynGrid(2, 0) = 120 AND work(0).dynGrid(2, 2) = 122)
ok = ok AND (work(1).fixedGrid(0, 0) = 31 AND work(1).fixedGrid(1, 1) = 44)
ok = ok AND (LBOUND(work(1).dynGrid, 1) = 0 AND UBOUND(work(1).dynGrid, 1) = 1)
ok = ok AND (LBOUND(work(1).dynGrid, 2) = 0 AND UBOUND(work(1).dynGrid, 2) = 1)
ok = ok AND (work(1).dynGrid(0, 0) = 300 AND work(1).dynGrid(1, 1) = 311)

IF ok THEN PRINT "PASS t279_static_dynamic_2d_retain_parent_resize" ELSE PRINT "FAIL t279_static_dynamic_2d_retain_parent_resize"
SYSTEM
