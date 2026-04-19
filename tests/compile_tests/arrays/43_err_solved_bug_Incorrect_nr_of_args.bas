TYPE e
    a(1 to 50, 2 to 20) AS _BYTE
END TYPE

DIM f(15) AS e

PRINT LBOUND(f(10).a(5), 1)

