
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

' Used to iterate over all the elements in a$
'
' index returns the current element number. index = -1 after last element
' strIndex is string index of the start of the next element
'
' index and strIndex should start initialized to zero
FUNCTION getnextelement$ (a$, index AS LONG, strIndex AS LONG)
    DIM i AS LONG

    IF strIndex = 0 THEN strIndex = 1

    i = INSTR(strIndex, a$, sp)

    IF i THEN
        getnextelement$ = MID$(a$, strIndex, i - strIndex)
        strIndex = i + 1
        index = index + 1
    ELSEIF strIndex <> LEN(a$) + 1 THEN
        getnextelement$ = MID$(a$, strIndex)
        strIndex = LEN(a$) + 1
        index = index + 1
    ELSE
        index = -1
    END IF
END FUNCTION

FUNCTION peeknextelement$ (a$, index AS LONG, strIndex AS LONG)
    peeknextelement$ = getnextelement$(a$, (index), (strIndex))
END FUNCTION

SUB pushelement (a$, b$)
    IF a$ <> "" THEN a$ = a$ + sp + b$ ELSE a$ = b$
END SUB

' Used to iterate over all the elements in a$ start with the last
'
' index returns the current element number. index = -1 after last element
' strIndex is string index of the start of the next element in the iteration
'
' index and strIndex should start initialized to zero, iteration will start at last element
FUNCTION getprevelement$ (a$, index AS LONG, strIndex AS LONG)
    DIM i AS LONG

    IF strIndex = 0 THEN strIndex = LEN(a$): Index = numelements(a$) + 1
    IF strIndex = -1 THEN Index = -1: EXIT FUNCTION

    IF strIndex > 0 THEN i = _INSTRREV(strIndex, a$, sp)

    IF i THEN
        getprevelement$ = MID$(a$, i + 1, strIndex - i)
        strIndex = i - 1
        index = index - 1

        ' Handle the case of an empty first element, strIndex would be zero
        ' which would trigger the starting logic again
        IF strIndex = 0 THEN strIndex = -2
    ELSEIF strIndex <> -1 THEN
        getprevelement$ = MID$(a$, 1, strIndex)
        strIndex = -1
        index = index - 1
    ELSE
        index = -1
    END IF
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

FUNCTION getelementsbefore$ (a$, i1)
    getelementsbefore$ = getelements$(a$, 1, i1)
END FUNCTION

FUNCTION getelementsafter$ (a$, i1)
    DIM p AS LONG, n AS LONG, i AS LONG

    n = 1
    p = 1

    getelementsnext:
    i = INSTR(p, a$, sp)

    IF n = i1 THEN
        getelementsafter$ = RIGHT$(a$, LEN(a$) - p + 1)
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

SUB removeelement (a$, i)
    removeelements a$, i, i, 0
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
                        ' We have an element here if there's any elements
                        ' in-between the previous comma and this one
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

FUNCTION eleucase$ (a$)
    DIM i AS LONG, i2 AS LONG, a2$, sp34$, i3 AS LONG

    'this function upper-cases all elements except for quoted strings
    'check first element
    IF LEN(a$) = 0 THEN EXIT FUNCTION
    i = 1
    IF ASC(a$) = 34 THEN
        i2 = INSTR(a$, sp)
        IF i2 = 0 THEN eleucase$ = a$: EXIT FUNCTION
        a2$ = LEFT$(a$, i2 - 1)
        i = i2
    END IF
    'check other elements
    sp34$ = sp + CHR$(34)
    IF i < LEN(a$) THEN
        DO WHILE INSTR(i, a$, sp34$)
            i2 = INSTR(i, a$, sp34$)
            a2$ = a2$ + UCASE$(MID$(a$, i, i2 - i + 1)) 'everything prior including spacer
            i3 = INSTR(i2 + 1, a$, sp): IF i3 = 0 THEN i3 = LEN(a$) ELSE i3 = i3 - 1
            a2$ = a2$ + MID$(a$, i2 + 1, i3 - (i2 + 1) + 1) 'everything from " to before next spacer or end
            i = i3 + 1
            IF i > LEN(a$) THEN EXIT DO
        LOOP
    END IF
    a2$ = a2$ + UCASE$(MID$(a$, i, LEN(a$) - i + 1))
    eleucase$ = a2$
END FUNCTION

'
' The natural type of the value is returned.
'
' The actual value is given back as floating point, integer, and unsigned integer.
'
FUNCTION elementGetNumericValue&(ele$, floating AS _FLOAT, integral AS _INTEGER64, uintegral AS _UNSIGNED _INTEGER64)
    Dim num$, typ&, e$, x As Long
    num$ = ele$
    typ& = 0

    ' Cut off the hex/oct/bin representation if present
    IF INSTR(num$, ",") THEN num$ = MID$(num$, 1, INSTR(num$, ",") - 1)

    ' integer suffixes
    e$ = RIGHT$(num$, 3)
    IF e$ = "~&&" THEN elementGetNumericValue& = UINTEGER64TYPE - ISPOINTER: GOTO handleInteger
    IF e$ = "~%%" THEN elementGetNumericValue& = UBYTETYPE - ISPOINTER: GOTO handleInteger
    e$ = RIGHT$(num$, 2)
    IF e$ = "&&" THEN elementGetNumericValue& = INTEGER64TYPE - ISPOINTER: GOTO handleInteger
    IF e$ = "%%" THEN elementGetNumericValue& = BYTETYPE - ISPOINTER: GOTO handleInteger
    IF e$ = "~%" THEN elementGetNumericValue& = UINTEGERTYPE - ISPOINTER: GOTO handleInteger
    IF e$ = "~&" THEN elementGetNumericValue& = ULONGTYPE - ISPOINTER: GOTO handleInteger
    e$ = RIGHT$(num$, 1)
    IF e$ = "%" THEN elementGetNumericValue& = INTEGERTYPE - ISPOINTER: GOTO handleInteger
    IF e$ = "&" THEN elementGetNumericValue& = LONGTYPE - ISPOINTER: GOTO handleInteger

    'ubit-type?
    IF INSTR(num$, "~`") THEN
        x = INSTR(num$, "~`")
        elementGetNumericValue& = UBITTYPE - ISPOINTER - 1 + VAL(RIGHT$(num$, LEN(num$) - x - 1))
        integral = VAL(LEFT$(num$, x - 1))
        uintegral = integral
        floating = integral
        EXIT FUNCTION
    END IF

    'bit-type?
    IF INSTR(num$, "`") THEN
        x = INSTR(num$, "`")
        elementGetNumericValue& = BITTYPE - ISPOINTER - 1 + VAL(RIGHT$(num$, LEN(num$) - x))
        integral = VAL(LEFT$(num$, x - 1))
        uintegral = integral
        floating = integral
        EXIT FUNCTION
    END IF

    'floats
    IF INSTR(num$, "F") OR RIGHT$(num$, 2) = "##" THEN
        floating = VAL(num$)
        integral = floating
        uintegral = floating

        elementGetNumericValue& = FLOATTYPE - ISPOINTER
        EXIT FUNCTION
    END IF
    IF INSTR(num$, "E") OR RIGHT$(num$, 1) = "!" THEN
        floating = VAL(num$)
        integral = floating
        uintegral = floating

        elementGetNumericValue& = SINGLETYPE - ISPOINTER
        EXIT FUNCTION
    END IF
    IF INSTR(num$, "D") OR RIGHT$(num$, 1) = "#" OR INSTR(num$, ".") THEN
        floating = VAL(num$)
        integral = floating
        uintegral = floating

        elementGetNumericValue& = DOUBLETYPE - ISPOINTER
        EXIT FUNCTION
    END IF

    ' No mentioned type, assume int64
    elementGetNumericValue& = INTEGER64TYPE - ISPOINTER
    e$ = ""

handleInteger:
    num$ = LEFT$(num$, LEN(num$) - LEN(e$))
    integral = VAL(num$)
    uintegral = integral
    floating = integral

END FUNCTION

' Returns whether the given element is a number
'
' Note that it allows numbers to have a negative sign.
FUNCTION elementIsNumber&(oele$)
    DIM ele$, res&
    IF oele$ = "" THEN EXIT FUNCTION
    ele$ = oele$

    ' Skip the negative if present
    IF ASC(ele$) = ASC("-") THEN ele$ = MID$(ele$, 2)

    ' Can start with a decimal point
    res& = (ASC(ele$) >= ASC("0") AND ASC(ele$) <= ASC("9"))
    IF NOT res& AND LEN(ele$) > 1 THEN res& = (ASC(ele$) = ASC(".") AND (ASC(ele$, 2) >= ASC("0") AND ASC(ele$, 2) <= ASC("9")))

    elementIsNumber& = res&
END FUNCTION

FUNCTION elementIsString&(ele$)
    ' String elements are always surrounded by quotes
    elementIsString& = INSTR(ele$, CHR$(34)) <> 0
END FUNCTION

FUNCTION elementGetStringValue&(ele$, value AS STRING)
    Dim rawString$, res$, i AS LONG
    ' We have to invert the escaping done by createElementString
    '
    ' Note this does not handle all possible C escaping, just the specific
    ' escaping done by createElementString
    '
    rawString$ = MID$(ele$, 2, INSTR(2, ele$, CHR$(34)) - 2)
    res$ = ""
    i = 1

    WHILE INSTR(i, rawString$, "\")
        res$ = res$ + MID$(rawString$, i, INSTR(i, rawString$, "\") - i)
        i = INSTR(i, rawString$, "\") + 1

        IF ASC(rawString$, i) = ASC("\") THEN
            res$ = res$ + "\"
            i = i + 1
        ELSE
            res$ = res$ + CHR$(VAL("&O" + MID$(rawString$, i, 3)))
            i = i + 3
        END IF
    WEND

    value = res$ + MID$(rawString$, i)
    elementGetStringValue& = STRINGTYPE
END FUNCTION

' s$ should be all the data making up the string, with no quotes around it
'
' The string data will have C escape sequences in it if necessary
FUNCTION createElementString$(s$)
    Dim ele$, o$, p1 As Long, c2 As Long, i As Long
    ele$ = CHR$(34)

    p1 = 1
    FOR i = 1 TO LEN(s$)
        c2 = ASC(s$, i)

        IF c2 = 92 THEN '\
            ele$ = ele$ + MID$(s$, p1, i - p1) + "\\"
            p1 = i + 1
        END IF

        IF c2 < 32 OR c2 = 34 OR c2 > 126 THEN
            o$ = OCT$(c2)
            IF LEN(o$) < 3 THEN
                o$ = "0" + o$
                IF LEN(o$) < 3 THEN o$ = "0" + o$
            END IF
            ele$ = ele$ + MID$(s$, p1, i - p1) + "\" + o$

            p1 = i + 1
        END IF
    NEXT

    ele$ = ele$ + MID$(s$, p1) + CHR$(34) + "," + _TRIM$(STR$(LEN(s$)))
    createElementString$ = ele$
END FUNCTION

FUNCTION elementStringConcat$(os1$, os2$)
    DIM s1$, s2$, s1size AS LONG, s2size AS LONG
    'concat strings
    s1$ = MID$(os1$, 2, _INSTRREV(os1$, CHR$(34)) - 2)
    s1size = VAL(RIGHT$(os1$, LEN(os1$) - LEN(s1$) - 3))

    s2$ = MID$(os2$, 2, _INSTRREV(os2$, CHR$(34)) - 2)
    s2size = VAL(RIGHT$(os2$, LEN(os2$) - LEN(s2$) - 3))

    elementStringConcat$ = CHR$(34) + s1$ + s2$ + CHR$(34) + "," + _TRIM$(STR$(s1size + s2size))
END FUNCTION
