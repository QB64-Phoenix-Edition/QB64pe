TYPE test
    variable AS SINGLE
    array(10) AS LONG
END TYPE

REDIM foo(10) AS test
foo(1).array(2) = 55
ERASE foo(1).arrayB


