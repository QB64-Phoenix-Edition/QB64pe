
FUNCTION getelement$ (a$, elenum)
    DIM p AS LONG, n AS LONG, i AS LONG

    IF a$ = "" THEN EXIT FUNCTION 'no elements!

    n = 1
    p = 1
    getelementnext:
    i = INSTR(p, a$, sp)

    IF elenum = n THEN
        IF i THEN
            getelement$ = MID$(a$, p, i - p)
        ELSE
            getelement$ = RIGHT$(a$, LEN(a$) - p + 1)
        END IF
        EXIT FUNCTION
    END IF

    IF i = 0 THEN EXIT FUNCTION 'no more elements!
    n = n + 1
    p = i + 1
    GOTO getelementnext
END FUNCTION

FUNCTION getelements$ (a$, i1, i2)
    DIM p AS LONG, n AS LONG, i AS LONG, i1pos AS LONG

    IF i2 < i1 THEN getelements$ = "": EXIT FUNCTION
    n = 1
    p = 1
    getelementsnext:
    i = INSTR(p, a$, sp)
    IF n = i1 THEN
        i1pos = p
    END IF
    IF n = i2 THEN
        IF i THEN
            getelements$ = MID$(a$, i1pos, i - i1pos)
        ELSE
            getelements$ = RIGHT$(a$, LEN(a$) - i1pos + 1)
        END IF
        EXIT FUNCTION
    END IF
    n = n + 1
    p = i + 1
    GOTO getelementsnext
END FUNCTION

SUB insertelements (a$, i, elements$)
    DIM a2 AS STRING, n AS LONG, i2 AS LONG

    IF i = 0 THEN
        IF a$ = "" THEN
            a$ = elements$
            EXIT SUB
        END IF
        a$ = elements$ + sp + a$
        EXIT SUB
    END IF

    a2$ = ""
    n = numelements(a$)

    FOR i2 = 1 TO n
        IF i2 > 1 THEN a2$ = a2$ + sp
        a2$ = a2$ + getelement$(a$, i2)
        IF i = i2 THEN a2$ = a2$ + sp + elements$
    NEXT

    a$ = a2$

END SUB

FUNCTION numelements (a$)
    DIM p AS LONG, n AS LONG, i AS LONG

    IF a$ = "" THEN EXIT FUNCTION
    n = 1
    p = 1
    numelementsnext:
    i = INSTR(p, a$, sp)
    IF i = 0 THEN numelements = n: EXIT FUNCTION
    n = n + 1
    p = i + 1
    GOTO numelementsnext
END FUNCTION

SUB removeelements (a$, first, last, keepindexing)
    DIM n AS LONG, i AS LONG, a2 AS STRING

    a2$ = ""
    'note: first and last MUST be valid
    '      keepindexing means the number of elements will stay the same
    '       but some elements will be equal to ""

    n = numelements(a$)
    FOR i = 1 TO n
        IF i < first OR i > last THEN
            a2$ = a2$ + sp + getelement(a$, i)
        ELSE
            IF keepindexing THEN a2$ = a2$ + sp
        END IF
    NEXT
    IF LEFT$(a2$, 1) = sp THEN a2$ = RIGHT$(a2$, LEN(a2$) - 1)

    a$ = a2$

END SUB

' a$ should be a function argument list
' Returns number of function arguments (including empty ones) in the provided list
FUNCTION countFunctionElements (a$)
    DIM count AS LONG, p AS LONG, lvl AS LONG, i AS LONG
    p = 1
    lvl = 1
    i = 0

    IF LEN(a$) = 0 THEN
        countFunctionElements = 0
        EXIT FUNCTION
    END IF

    DO
        SELECT CASE ASC(a$, i + 1)
            CASE ASC("("):
                lvl = lvl + 1

            CASE ASC(")"):
                lvl = lvl - 1

            CASE ASC(","):
                IF lvl = 1 THEN
                    count = count + 1
                END IF

        END SELECT

        i = INSTR(p, a$, sp)
        IF i = 0 THEN
            EXIT DO
        END IF

        p = i + 1
    LOOP

    ' Make sure to count the argument after the last comma
    countFunctionElements = count + 1
END FUNCTION

' a$ should be a function argument list
' Returns true if the argument was provided in the list
FUNCTION hasFunctionElement (a$, element)
    DIM count AS LONG, p AS LONG, lvl AS LONG, i AS LONG, start AS LONG
    start = 0
    p = 1
    lvl = 1
    i = 1

    IF LEN(a$) = 0 THEN
        hasFunctionElement = 0
        EXIT FUNCTION
    END IF

    ' Special case for a single provided argument
    IF INSTR(a$, sp) = 0 AND LEN(a$) <> 0 THEN
        hasFunctionElement = element = 1
        EXIT FUNCTION
    END IF

    DO
        IF i > LEN(a$) THEN
            EXIT DO
        END IF

        SELECT CASE ASC(a$, i)
            CASE ASC("("):
                lvl = lvl + 1

            CASE ASC(")"):
                lvl = lvl - 1

            CASE ASC(","):
                IF lvl = 1 THEN
                    count = count + 1

                    IF element = count THEN
                        ' We have a element here if there's any elements
                        ' inbetween the previous comma and this one
                        hasFunctionElement = (i <> 1) AND (i - 2 <> start)
                        EXIT FUNCTION
                    END IF

                    start = i
                END IF
        END SELECT

        p = i
        i = INSTR(i, a$, sp)

        IF i = 0 THEN
            EXIT DO
        END IF

        i = i + 1
    LOOP

    IF element > count + 1 THEN
        hasFunctionElement = 0
        EXIT FUNCTION
    END IF

    ' Check if last argument was provided.
    '
    ' Syntax '2,3' has two arguments, the '3' argument is what gets compared here
    ' Syntax '2,' has one argument, the comma is the last element so it fails this check.
    IF p > 0 THEN
        IF ASC(a$, p) <> ASC(",") THEN
            hasFunctionElement = -1
            EXIT FUNCTION
        END IF
    END IF

    hasFunctionElement = 0
END FUNCTION

' Returns true if the provided arguments are a valid set for the given function format
' firstOptionalArgument returns the index of the first argument that is optional
FUNCTION isValidArgSet (format AS STRING, providedArgs() AS LONG, firstOptionalArgument AS LONG)
    DIM maxArgument AS LONG, i AS LONG
    DIM currentArg AS LONG, optionLvl AS LONG
    DIM wasProvided(0 TO 10) AS LONG
    DIM AS LONG ArgProvided, ArgNotProvided, ArgIgnored

    ArgProvided = -1
    ArgNotProvided = 0
    ArgIgnored = -2
    firstOptionalArgument = 0

    wasProvided(0) = ArgIgnored

    ' Inside of each set of brackets, all arguments must either be provide or not provided, with no mixing.
    ' For nested brackets, if the argument(s) inside the nested brackets are
    ' provided, then the arguments inside the outer brackets also have to be
    ' provided. Ex:
    '
    '     x[,y[,z]]
    '
    ' When x is provided, y and z are optional, but when y is provided x is required.
    ' When z is provided both y and x are required.

    maxArgument = UBOUND(providedArgs)

    FOR i = 1 TO LEN(format)
        SELECT CASE ASC(format, i)
            CASE ASC("["):
                optionLvl = optionLvl + 1
                wasProvided(optionLvl) = ArgIgnored

            CASE ASC("]"):
                optionLvl = optionLvl - 1

                IF wasProvided(optionLvl) = ArgIgnored THEN
                    ' If not provided, then we stay in the ignored state
                    ' because whether this arg set was provided does not matter
                    ' for the rest of the parsing
                    IF wasProvided(optionLvl + 1) = ArgProvided THEN
                        wasProvided(optionLvl) = ArgProvided
                    END IF
                ELSE
                    ' If an arg at this level was already not provided, then
                    ' this optional set can't be provided either
                    IF wasProvided(optionLvl) = ArgNotProvided AND wasProvided(optionLvl + 1) = ArgProvided THEN
                        isValidArgSet = 0
                        EXIT FUNCTION
                    END IF
                END IF

            CASE ASC("?"):
                currentArg = currentArg + 1
                IF optionLvl >= 1 AND firstOptionalArgument = 0 THEN firstOptionalArgument = currentArg

                IF wasProvided(optionLvl) = ArgIgnored THEN
                    IF maxArgument >= currentArg THEN
                        wasProvided(optionLvl) = providedArgs(currentArg)
                    ELSE
                        wasProvided(optionLvl) = 0
                    END IF
                ELSE
                    IF maxArgument < currentArg THEN
                        IF wasProvided(optionLvl) <> ArgNotProvided THEN
                            isValidArgSet = 0
                            EXIT FUNCTION
                        END IF
                    ELSEIF wasProvided(optionLvl) <> providedArgs(currentArg) THEN
                        isValidArgSet = 0
                        EXIT FUNCTION
                    END IF
                END IF
        END SELECT
    NEXT

    ' The base level of arguments are required. They can be in the
    ' 'ignored' state though if all arguments are within brackets
    IF currentArg < maxArgument OR wasProvided(0) = ArgNotProvided THEN
        isValidArgSet = 0
        EXIT FUNCTION
    END IF

    isValidArgSet = -1
END FUNCTION
