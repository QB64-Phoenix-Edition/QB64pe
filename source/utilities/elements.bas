
FUNCTION getelement$ (a$, elenum)
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
    Dim count As Long, p As Long, lvl As Long
    p = 1
    lvl = 1
    i = 0

    if Len(a$) = 0 Then
        countFunctionElements = 0
        Exit Function
    End If

    Do
        Select Case Asc(a$, i + 1)
            Case Asc("("):
                lvl = lvl + 1

            Case Asc(")"):
                lvl = lvl - 1

            Case Asc(","):
                If lvl = 1 Then
                    count = count + 1
                End If

        End Select

        i = INSTR(p, a$, sp)
        if i = 0 Then
            Exit Do
        End If

        p = i + 1
    Loop

    ' Make sure to count the argument after the last comma
    countFunctionElements = count + 1
END FUNCTION

' a$ should be a function argument list
' Returns true if the argument was provided in the list
FUNCTION hasFunctionElement(a$, element)
    Dim count As Long, p As Long, lvl As Long
    start = 0
    p = 1
    lvl = 1
    i = 1

    if Len(a$) = 0 Then
        hasFunctionElement = 0
        Exit Function
    End If

    ' Special case for a single provided argument
    If INSTR(a$, sp) = 0 And Len(a$) <> 0 Then
        hasFunctionElement = element = 1
        Exit Function
    End If

    Do
        If i > Len(a$) Then
            Exit Do
        End If

        Select Case Asc(a$, i)
            Case Asc("("):
                lvl = lvl + 1

            Case Asc(")"):
                lvl = lvl - 1

            Case Asc(","):
                If lvl = 1 Then
                    count = count + 1

                    If element = count Then
                        ' We have a element here if there's any elements
                        ' inbetween the previous comma and this one
                        hasFunctionElement = (i <> 1) And (i - 2 <> start)
                        Exit Function
                    End If

                    start = i
                End If
        End Select

        p = i
        i = INSTR(i, a$, sp)

        if i = 0 Then
            Exit Do
        End If

        i = i + 1
    Loop

    If element > count + 1 Then
        hasFunctionElement = 0
        Exit Function
    End If

    ' Check if last argument was provided.
    '
    ' Syntax '2,3' has two arguments, the '3' argument is what gets compared here
    ' Syntax '2,' has one argument, the comma is the last element so it fails this check.
    If p > 0 Then
        If Asc(a$, p) <> Asc(",") Then
            hasFunctionElement = -1
            Exit Function
        End If
    End If

    hasFunctionElement = 0
END FUNCTION

' Returns true if the provided arguments are a valid set for the given function format
' firstOptionalArgument returns the index of the first argument that is optional
FUNCTION isValidArgSet(format As String, providedArgs() As Long, firstOptionalArgument As Long)
    Dim maxArgument As Long, i As Long
    Dim currentArg As Long, optionLvl As Long
    Dim wasProvided(0 To 10) As Long
    Dim As Long ArgProvided , ArgNotProvided, ArgIgnored

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

    For i = 1 to Len(format)
        Select Case Asc(format, i)
            Case Asc("["):
                optionLvl = optionLvl + 1
                wasProvided(optionLvl) = ArgIgnored

            Case Asc("]"):
                optionLvl = optionLvl - 1

                If wasProvided(optionLvl) = ArgIgnored Then
                    ' If not provided, then we stay in the ignored state
                    ' because whether this arg set was provided does not matter
                    ' for the rest of the parsing
                    If wasProvided(optionLvl + 1) = ArgProvided Then
                        wasProvided(optionLvl) = ArgProvided
                    End If
                Else
                    ' If an arg at this level was already not provided, then
                    ' this optional set can't be provided either
                    if wasProvided(optionLvl) = ArgNotProvided And wasProvided(optionLvl + 1) = ArgProvided Then
                        isValidArgSet = 0
                        EXIT FUNCTION
                    End If
                End If

            Case Asc("?"):
                currentArg = currentArg + 1
                if optionLvl >= 1 And firstOptionalArgument = 0 Then firstOptionalArgument = currentArg

                if wasProvided(optionLvl) = ArgIgnored Then
                    If maxArgument >= currentArg Then
                        wasProvided(optionLvl) = providedArgs(currentArg)
                    else
                        wasProvided(optionLvl) = 0
                    End If
                else
                    if maxArgument < currentArg Then
                        If wasProvided(optionLvl) <> ArgNotProvided Then
                            isValidArgSet = 0
                            Exit Function
                        End If
                    Elseif wasProvided(optionLvl) <> providedArgs(currentArg) Then
                        isValidArgSet = 0
                        EXIT FUNCTION
                    End If
                End If
        End Select
    Next

    ' The base level of arguments are required. They can be in the
    ' 'ignored' state though if all arguments are within brackets
    if currentArg < maxArgument Or wasProvided(0) = ArgNotProvided then
        isValidArgSet = 0
        Exit Function
    End If

    isValidArgSet = -1
END FUNCTION
