
FUNCTION typevalue2symbol$ (t)

    IF t AND ISSTRING THEN
        IF t AND ISFIXEDLENGTH THEN Give_Error "Cannot convert expression type to symbol": EXIT FUNCTION
        typevalue2symbol$ = "$"
        EXIT FUNCTION
    END IF

    s$ = ""

    IF t AND ISUNSIGNED THEN s$ = "~"

    b = t AND 511

    IF t AND ISOFFSETINBITS THEN
        IF b > 1 THEN s$ = s$ + "`" + str2$(b) ELSE s$ = s$ + "`"
        typevalue2symbol$ = s$
        EXIT FUNCTION
    END IF

    IF t AND ISFLOAT THEN
        IF b = 32 THEN s$ = "!"
        IF b = 64 THEN s$ = "#"
        IF b = 256 THEN s$ = "##"
        typevalue2symbol$ = s$
        EXIT FUNCTION
    END IF

    IF b = 8 THEN s$ = s$ + "%%"
    IF b = 16 THEN s$ = s$ + "%"
    IF b = 32 THEN s$ = s$ + "&"
    IF b = 64 THEN s$ = s$ + "&&"
    typevalue2symbol$ = s$

END FUNCTION

FUNCTION id2fulltypename$
    t = id.t
    IF t = 0 THEN t = id.arraytype
    size = id.tsize
    bits = t AND 511
    IF t AND ISUDT THEN
        a$ = RTRIM$(udtxcname(t AND 511))
        id2fulltypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISSTRING THEN
        IF t AND ISFIXEDLENGTH THEN a$ = "STRING * " + str2(size) ELSE a$ = "STRING"
        id2fulltypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISOFFSETINBITS THEN
        IF bits > 1 THEN a$ = qb64prefix$ + "BIT * " + str2(bits) ELSE a$ = qb64prefix$ + "BIT"
        IF t AND ISUNSIGNED THEN a$ = qb64prefix$ + "UNSIGNED " + a$
        id2fulltypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISFLOAT THEN
        IF bits = 32 THEN a$ = "SINGLE"
        IF bits = 64 THEN a$ = "DOUBLE"
        IF bits = 256 THEN a$ = qb64prefix$ + "FLOAT"
    ELSE 'integer-based
        IF bits = 8 THEN a$ = qb64prefix$ + "BYTE"
        IF bits = 16 THEN a$ = "INTEGER"
        IF bits = 32 THEN a$ = "LONG"
        IF bits = 64 THEN a$ = qb64prefix$ + "INTEGER64"
        IF t AND ISUNSIGNED THEN a$ = qb64prefix$ + "UNSIGNED " + a$
    END IF
    IF t AND ISOFFSET THEN
        a$ = qb64prefix$ + "OFFSET"
        IF t AND ISUNSIGNED THEN a$ = qb64prefix$ + "UNSIGNED " + a$
    END IF
    id2fulltypename$ = a$
END FUNCTION

FUNCTION id2shorttypename$
    t = id.t
    IF t = 0 THEN t = id.arraytype
    size = id.tsize
    bits = t AND 511
    IF t AND ISUDT THEN
        a$ = RTRIM$(udtxcname(t AND 511))
        id2shorttypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISSTRING THEN
        IF t AND ISFIXEDLENGTH THEN a$ = "STRING" + str2(size) ELSE a$ = "STRING"
        id2shorttypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISOFFSETINBITS THEN
        IF t AND ISUNSIGNED THEN a$ = "_U" ELSE a$ = "_"
        IF bits > 1 THEN a$ = a$ + "BIT" + str2(bits) ELSE a$ = a$ + "BIT1"
        id2shorttypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISFLOAT THEN
        IF bits = 32 THEN a$ = "SINGLE"
        IF bits = 64 THEN a$ = "DOUBLE"
        IF bits = 256 THEN a$ = "_FLOAT"
    ELSE 'integer-based
        IF bits = 8 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "_UBYTE" ELSE a$ = "_BYTE"
        END IF
        IF bits = 16 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "UINTEGER" ELSE a$ = "INTEGER"
        END IF
        IF bits = 32 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "ULONG" ELSE a$ = "LONG"
        END IF
        IF bits = 64 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "_UINTEGER64" ELSE a$ = "_INTEGER64"
        END IF
    END IF
    id2shorttypename$ = a$
END FUNCTION

FUNCTION symbol2fulltypename$ (s2$)
    'note: accepts both symbols and type names
    s$ = s2$

    IF LEFT$(s$, 1) = "~" THEN
        u = 1
        IF LEN(typ$) = 1 THEN Give_Error "Expected ~...": EXIT FUNCTION
        s$ = RIGHT$(s$, LEN(s$) - 1)
        u$ = qb64prefix$ + "UNSIGNED "
    END IF

    IF s$ = "%%" THEN t$ = u$ + qb64prefix$ + "BYTE": GOTO gotsym2typ
    IF s$ = "%" THEN t$ = u$ + "INTEGER": GOTO gotsym2typ
    IF s$ = "&" THEN t$ = u$ + "LONG": GOTO gotsym2typ
    IF s$ = "&&" THEN t$ = u$ + qb64prefix$ + "INTEGER64": GOTO gotsym2typ
    IF s$ = "%&" THEN t$ = u$ + qb64prefix$ + "OFFSET": GOTO gotsym2typ

    IF LEFT$(s$, 1) = "`" THEN
        IF LEN(s$) = 1 THEN
            t$ = u$ + qb64prefix$ + "BIT * 1"
            GOTO gotsym2typ
        END IF
        n$ = RIGHT$(s$, LEN(s$) - 1)
        IF isuinteger(n$) = 0 THEN Give_Error "Expected number after symbol `": EXIT FUNCTION
        t$ = u$ + qb64prefix$ + "BIT * " + n$
        GOTO gotsym2typ
    END IF

    IF u = 1 THEN Give_Error "Expected type symbol after ~": EXIT FUNCTION

    IF s$ = "!" THEN t$ = "SINGLE": GOTO gotsym2typ
    IF s$ = "#" THEN t$ = "DOUBLE": GOTO gotsym2typ
    IF s$ = "##" THEN t$ = qb64prefix$ + "FLOAT": GOTO gotsym2typ
    IF s$ = "$" THEN t$ = "STRING": GOTO gotsym2typ

    IF LEFT$(s$, 1) = "$" THEN
        n$ = RIGHT$(s$, LEN(s$) - 1)
        IF isuinteger(n$) = 0 THEN Give_Error "Expected number after symbol $": EXIT FUNCTION
        t$ = "STRING * " + n$
        GOTO gotsym2typ
    END IF

    t$ = s$

    gotsym2typ:

    IF RIGHT$(" " + t$, 5) = " _BIT" THEN t$ = t$ + " * 1" 'clarify (_UNSIGNED) _BIT as (_UNSIGNED) _BIT * 1

    FOR i = 1 TO LEN(t$)
        IF ASC(t$, i) = ASC(sp) THEN ASC(t$, i) = 32
    NEXT

    symbol2fulltypename$ = t$

END FUNCTION

FUNCTION symboltype (s$) 'returns type or 0(not a valid symbol)
    'note: sets symboltype_size for fixed length strings
    'created: 2011 (fast & comprehensive)
    IF LEN(s$) = 0 THEN EXIT FUNCTION
    'treat common cases first
    a = ASC(s$)
    l = LEN(s$)
    IF a = 37 THEN '%
        IF l = 1 THEN symboltype = 16: EXIT FUNCTION
        IF l > 2 THEN EXIT FUNCTION
        IF ASC(s$, 2) = 37 THEN symboltype = 8: EXIT FUNCTION
        IF ASC(s$, 2) = 38 THEN symboltype = OFFSETTYPE - ISPOINTER: EXIT FUNCTION '%&
        EXIT FUNCTION
    END IF
    IF a = 38 THEN '&
        IF l = 1 THEN symboltype = 32: EXIT FUNCTION
        IF l > 2 THEN EXIT FUNCTION
        IF ASC(s$, 2) = 38 THEN symboltype = 64: EXIT FUNCTION
        EXIT FUNCTION
    END IF
    IF a = 33 THEN '!
        IF l = 1 THEN symboltype = 32 + ISFLOAT: EXIT FUNCTION
        EXIT FUNCTION
    END IF
    IF a = 35 THEN '#
        IF l = 1 THEN symboltype = 64 + ISFLOAT: EXIT FUNCTION
        IF l > 2 THEN EXIT FUNCTION
        IF ASC(s$, 2) = 35 THEN symboltype = 64 + ISFLOAT: EXIT FUNCTION
        EXIT FUNCTION
    END IF
    IF a = 36 THEN '$
        IF l = 1 THEN symboltype = ISSTRING: EXIT FUNCTION
        IF isuinteger(RIGHT$(s$, l - 1)) THEN
            IF l >= (1 + 10) THEN
                IF l > (1 + 10) THEN EXIT FUNCTION
                IF s$ > "$2147483647" THEN EXIT FUNCTION
            END IF
            symboltype_size = VAL(RIGHT$(s$, l - 1))
            symboltype = ISSTRING + ISFIXEDLENGTH
            EXIT FUNCTION
        END IF
        EXIT FUNCTION
    END IF
    IF a = 96 THEN '`
        IF l = 1 THEN symboltype = 1 + ISOFFSETINBITS: EXIT FUNCTION
        IF isuinteger(RIGHT$(s$, l - 1)) THEN
            IF l > 3 THEN EXIT FUNCTION
            n = VAL(RIGHT$(s$, l - 1))
            IF n > 64 THEN EXIT FUNCTION
            symboltype = n + ISOFFSETINBITS: EXIT FUNCTION
        END IF
        EXIT FUNCTION
    END IF
    IF a = 126 THEN '~
        IF l = 1 THEN EXIT FUNCTION
        a = ASC(s$, 2)
        IF a = 37 THEN '%
            IF l = 2 THEN symboltype = 16 + ISUNSIGNED: EXIT FUNCTION
            IF l > 3 THEN EXIT FUNCTION
            IF ASC(s$, 3) = 37 THEN symboltype = 8 + ISUNSIGNED: EXIT FUNCTION
            IF ASC(s$, 3) = 38 THEN symboltype = UOFFSETTYPE - ISPOINTER: EXIT FUNCTION '~%&
            EXIT FUNCTION
        END IF
        IF a = 38 THEN '&
            IF l = 2 THEN symboltype = 32 + ISUNSIGNED: EXIT FUNCTION
            IF l > 3 THEN EXIT FUNCTION
            IF ASC(s$, 3) = 38 THEN symboltype = 64 + ISUNSIGNED: EXIT FUNCTION
            EXIT FUNCTION
        END IF
        IF a = 96 THEN '`
            IF l = 2 THEN symboltype = 1 + ISOFFSETINBITS + ISUNSIGNED: EXIT FUNCTION
            IF isuinteger(RIGHT$(s$, l - 2)) THEN
                IF l > 4 THEN EXIT FUNCTION
                n = VAL(RIGHT$(s$, l - 2))
                IF n > 64 THEN EXIT FUNCTION
                symboltype = n + ISOFFSETINBITS + ISUNSIGNED: EXIT FUNCTION
            END IF
            EXIT FUNCTION
        END IF
    END IF '~
END FUNCTION

FUNCTION typ2ctyp$ (t AS LONG, tstr AS STRING)
    ctyp$ = ""
    'typ can be passed as either: (the unused value is ignored)
    'i. as a typ value in t
    'ii. as a typ symbol (eg. "~%") in tstr
    'iii. as a typ name (eg. _UNSIGNED INTEGER) in tstr
    IF tstr$ = "" THEN
        IF (t AND ISARRAY) THEN EXIT FUNCTION 'cannot return array types
        IF (t AND ISSTRING) THEN typ2ctyp$ = "qbs": EXIT FUNCTION
        b = t AND 511
        IF (t AND ISUDT) THEN typ2ctyp$ = "void": EXIT FUNCTION
        IF (t AND ISOFFSETINBITS) THEN
            IF b <= 32 THEN ctyp$ = "int32" ELSE ctyp$ = "int64"
            IF (t AND ISUNSIGNED) THEN ctyp$ = "u" + ctyp$
            typ2ctyp$ = ctyp$: EXIT FUNCTION
        END IF
        IF (t AND ISFLOAT) THEN
            IF b = 32 THEN ctyp$ = "float"
            IF b = 64 THEN ctyp$ = "double"
            IF b = 256 THEN ctyp$ = "long double"
        ELSE
            IF b = 8 THEN ctyp$ = "int8"
            IF b = 16 THEN ctyp$ = "int16"
            IF b = 32 THEN ctyp$ = "int32"
            IF b = 64 THEN ctyp$ = "int64"
            IF t AND ISOFFSET THEN ctyp$ = "ptrszint"
            IF (t AND ISUNSIGNED) THEN ctyp$ = "u" + ctyp$
        END IF
        IF t AND ISOFFSET THEN
            ctyp$ = "ptrszint": IF (t AND ISUNSIGNED) THEN ctyp$ = "uptrszint"
        END IF
        typ2ctyp$ = ctyp$: EXIT FUNCTION
    END IF

    ts$ = tstr$
    'is ts$ a symbol?
    IF ts$ = "$" THEN ctyp$ = "qbs"
    IF ts$ = "!" THEN ctyp$ = "float"
    IF ts$ = "#" THEN ctyp$ = "double"
    IF ts$ = "##" THEN ctyp$ = "long double"
    IF LEFT$(ts$, 1) = "~" THEN unsgn = 1: ts$ = RIGHT$(ts$, LEN(ts$) - 1)
    IF LEFT$(ts$, 1) = "`" THEN
        n$ = RIGHT$(ts$, LEN(ts$) - 1)
        b = 1
        IF n$ <> "" THEN
            IF isuinteger(n$) = 0 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
            b = VAL(n$)
            IF b > 64 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
        END IF
        IF b <= 32 THEN ctyp$ = "int32" ELSE ctyp$ = "int64"
        IF unsgn THEN ctyp$ = "u" + ctyp$
        typ2ctyp$ = ctyp$: EXIT FUNCTION
    END IF
    IF ts$ = "%&" THEN
        typ2ctyp$ = "ptrszint": IF (t AND ISUNSIGNED) THEN typ2ctyp$ = "uptrszint"
        EXIT FUNCTION
    END IF
    IF ts$ = "%%" THEN ctyp$ = "int8"
    IF ts$ = "%" THEN ctyp$ = "int16"
    IF ts$ = "&" THEN ctyp$ = "int32"
    IF ts$ = "&&" THEN ctyp$ = "int64"
    IF ctyp$ <> "" THEN
        IF unsgn THEN ctyp$ = "u" + ctyp$
        typ2ctyp$ = ctyp$: EXIT FUNCTION
    END IF
    'is tstr$ a named type? (eg. 'LONG')
    s$ = type2symbol$(tstr$)
    IF Error_Happened THEN EXIT FUNCTION
    IF LEN(s$) THEN
        typ2ctyp$ = typ2ctyp$(0, s$)
        IF Error_Happened THEN EXIT FUNCTION
        EXIT FUNCTION
    END IF

    Give_Error "Invalid type": EXIT FUNCTION

END FUNCTION

FUNCTION type2symbol$ (typ$)
    t$ = typ$
    FOR i = 1 TO LEN(t$)
        IF MID$(t$, i, 1) = sp THEN MID$(t$, i, 1) = " "
    NEXT
    e$ = "Cannot convert type (" + typ$ + ") to symbol"

    t2$ = "INTEGER": s$ = "%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "LONG": s$ = "&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "SINGLE": s$ = "!": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "DOUBLE": s$ = "#": IF t$ = t2$ THEN GOTO t2sfound

    t2$ = "_BYTE": s$ = "%%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "BYTE": s$ = "%%": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED LONG": s$ = "~&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED LONG": s$ = "~&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED INTEGER": s$ = "~%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED INTEGER": s$ = "~%": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED _BYTE": s$ = "~%%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED BYTE": s$ = "~%%": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED _BYTE": s$ = "~%%": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED BYTE": s$ = "~%%": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED _OFFSET": s$ = "~%&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED OFFSET": s$ = "~%&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED _OFFSET": s$ = "~%&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED OFFSET": s$ = "~%&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED _INTEGER64": s$ = "~&&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED INTEGER64": s$ = "~&&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED _INTEGER64": s$ = "~&&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED INTEGER64": s$ = "~&&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_INTEGER64": s$ = "&&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "INTEGER64": s$ = "&&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_OFFSET": s$ = "%&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "OFFSET": s$ = "%&": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    t2$ = "_FLOAT": s$ = "##": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "FLOAT": s$ = "##": IF qb64prefix_set = 1 AND t$ = t2$ THEN GOTO t2sfound

    ' These can have a length after them, so LEFT$() is used
    t2$ = "STRING": s$ = "$": IF LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED _BIT": s$ = "~`1": IF LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED BIT": s$ = "~`1": IF qb64prefix_set = 1 AND LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED _BIT": s$ = "~`1": IF qb64prefix_set = 1 AND LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "UNSIGNED BIT": s$ = "~`1": IF qb64prefix_set = 1 AND LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "_BIT": s$ = "`1": IF LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "BIT": s$ = "`1": IF qb64prefix_set = 1 AND LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound

    Give_Error e$: EXIT FUNCTION
    t2sfound:
    type2symbol$ = s$
    IF LEN(t2$) <> LEN(t$) THEN
        IF s$ <> "$" AND s$ <> "~`1" AND s$ <> "`1" THEN Give_Error e$: EXIT FUNCTION
        t$ = RIGHT$(t$, LEN(t$) - LEN(t2$))
        IF LEFT$(t$, 3) <> " * " THEN Give_Error e$: EXIT FUNCTION
        t$ = RIGHT$(t$, LEN(t$) - 3)
        IF isuinteger(t$) = 0 THEN Give_Error e$: EXIT FUNCTION
        v = VAL(t$)
        IF v = 0 THEN Give_Error e$: EXIT FUNCTION
        IF s$ <> "$" AND v > 64 THEN Give_Error e$: EXIT FUNCTION
        IF s$ = "$" THEN
            s$ = s$ + str2$(v)
        ELSE
            s$ = LEFT$(s$, LEN(s$) - 1) + str2$(v)
        END IF
        type2symbol$ = s$
    END IF
END FUNCTION

'Strips away bits/indentifiers which make locating a variables source difficult
FUNCTION typecomp (typ)
    typ2 = typ
    IF (typ2 AND ISINCONVENTIONALMEMORY) THEN typ2 = typ2 - ISINCONVENTIONALMEMORY
    typecomp = typ2
END FUNCTION

FUNCTION typname2typ& (t2$)
    typname2typsize = 0 'the default

    t$ = t2$

    'symbol?
    ts$ = t$
    IF ts$ = "$" THEN typname2typ& = STRINGTYPE: EXIT FUNCTION
    IF ts$ = "!" THEN typname2typ& = SINGLETYPE: EXIT FUNCTION
    IF ts$ = "#" THEN typname2typ& = DOUBLETYPE: EXIT FUNCTION
    IF ts$ = "##" THEN typname2typ& = FLOATTYPE: EXIT FUNCTION

    'fixed length string?
    IF LEFT$(ts$, 1) = "$" THEN
        n$ = RIGHT$(ts$, LEN(ts$) - 1)
        IF isuinteger(n$) = 0 THEN Give_Error "Invalid index after STRING * type": EXIT FUNCTION
        b = VAL(n$)
        IF b = 0 THEN Give_Error "Invalid index after STRING * type": EXIT FUNCTION
        typname2typsize = b
        typname2typ& = STRINGTYPE + ISFIXEDLENGTH
        EXIT FUNCTION
    END IF

    'unsigned?
    IF LEFT$(ts$, 1) = "~" THEN unsgn = 1: ts$ = RIGHT$(ts$, LEN(ts$) - 1)

    'bit-type?
    IF LEFT$(ts$, 1) = "`" THEN
        n$ = RIGHT$(ts$, LEN(ts$) - 1)
        b = 1
        IF n$ <> "" THEN
            IF isuinteger(n$) = 0 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
            b = VAL(n$)
            IF b > 64 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
        END IF
        IF unsgn THEN typname2typ& = UBITTYPE + (b - 1) ELSE typname2typ& = BITTYPE + (b - 1)
        EXIT FUNCTION
    END IF

    t = 0
    IF ts$ = "%%" THEN t = BYTETYPE
    IF ts$ = "%" THEN t = INTEGERTYPE
    IF ts$ = "&" THEN t = LONGTYPE
    IF ts$ = "&&" THEN t = INTEGER64TYPE
    IF ts$ = "%&" THEN t = OFFSETTYPE

    IF t THEN
        IF unsgn THEN t = t + ISUNSIGNED
        typname2typ& = t: EXIT FUNCTION
    END IF
    'not a valid symbol

    'type name?
    FOR i = 1 TO LEN(t$)
        IF MID$(t$, i, 1) = sp THEN MID$(t$, i, 1) = " "
    NEXT
    IF t$ = "STRING" THEN typname2typ& = STRINGTYPE: EXIT FUNCTION

    IF LEFT$(t$, 9) = "STRING * " THEN

        n$ = RIGHT$(t$, LEN(t$) - 9)

        'constant check 2011
        hashfound = 0
        hashname$ = n$
        hashchkflags = HASHFLAG_CONSTANT
        hashres = HashFindRev(hashname$, hashchkflags, hashresflags, hashresref)
        DO WHILE hashres
            IF constsubfunc(hashresref) = subfuncn OR constsubfunc(hashresref) = 0 THEN
                IF constdefined(hashresref) THEN
                    hashfound = 1
                    EXIT DO
                END IF
            END IF
            IF hashres <> 1 THEN hashres = HashFindCont(hashresflags, hashresref) ELSE hashres = 0
        LOOP
        IF hashfound THEN
            i2 = hashresref
            t = consttype(i2)
            IF t AND ISSTRING THEN Give_Error "Expected STRING * numeric-constant": EXIT FUNCTION
            'convert value to general formats
            IF t AND ISFLOAT THEN
                v## = constfloat(i2)
                v&& = v##
                v~&& = v&&
            ELSE
                IF t AND ISUNSIGNED THEN
                    v~&& = constuinteger(i2)
                    v&& = v~&&
                    v## = v&&
                ELSE
                    v&& = constinteger(i2)
                    v## = v&&
                    v~&& = v&&
                END IF
            END IF
            IF v&& < 1 OR v&& > 9999999999 THEN Give_Error "STRING * out-of-range constant": EXIT FUNCTION
            b = v&&
            GOTO constantlenstr
        END IF

        IF isuinteger(n$) = 0 OR LEN(n$) > 10 THEN Give_Error "Invalid number/constant after STRING * type": EXIT FUNCTION
        b = VAL(n$)
        IF b = 0 OR LEN(n$) > 10 THEN Give_Error "Invalid number after STRING * type": EXIT FUNCTION
        constantlenstr:
        typname2typsize = b
        typname2typ& = STRINGTYPE + ISFIXEDLENGTH
        EXIT FUNCTION
    END IF

    IF t$ = "SINGLE" THEN typname2typ& = SINGLETYPE: EXIT FUNCTION
    IF t$ = "DOUBLE" THEN typname2typ& = DOUBLETYPE: EXIT FUNCTION
    IF t$ = "_FLOAT" OR (t$ = "FLOAT" AND qb64prefix_set = 1) THEN typname2typ& = FLOATTYPE: EXIT FUNCTION
    IF LEFT$(t$, 10) = "_UNSIGNED " OR (LEFT$(t$, 9) = "UNSIGNED " AND qb64prefix_set = 1) THEN
        u = 1
        t$ = MID$(t$, INSTR(t$, CHR$(32)) + 1)
    END IF
    IF LEFT$(t$, 4) = "_BIT" OR (LEFT$(t$, 3) = "BIT" AND qb64prefix_set = 1) THEN
        IF t$ = "_BIT" OR (t$ = "BIT" AND qb64prefix_set = 1) THEN
            IF u THEN typname2typ& = UBITTYPE ELSE typname2typ& = BITTYPE
            EXIT FUNCTION
        END IF

        IF LEFT$(t$, 7) <> "_BIT * " AND LEFT$(t$, 6) <> "BIT * " THEN Give_Error "Expected " + qb64prefix$ + "BIT * number": EXIT FUNCTION

        IF LEFT$(t$, 4) = "_BIT" THEN
            n$ = RIGHT$(t$, LEN(t$) - 7)
        ELSE
            n$ = RIGHT$(t$, LEN(t$) - 6)
        END IF

        IF isuinteger(n$) = 0 THEN Give_Error "Invalid size after " + qb64prefix$ + "BIT *": EXIT FUNCTION
        b = VAL(n$)
        IF b = 0 OR b > 64 THEN Give_Error "Invalid size after " + qb64prefix$ + "BIT *": EXIT FUNCTION
        t = BITTYPE - 1 + b: IF u THEN t = t + ISUNSIGNED
        typname2typ& = t
        EXIT FUNCTION
    END IF

    t = 0
    IF t$ = "_BYTE" OR (t$ = "BYTE" AND qb64prefix_set = 1) THEN t = BYTETYPE
    IF t$ = "INTEGER" THEN t = INTEGERTYPE
    IF t$ = "LONG" THEN t = LONGTYPE
    IF t$ = "_INTEGER64" OR (t$ = "INTEGER64" AND qb64prefix_set = 1) THEN t = INTEGER64TYPE
    IF t$ = "_OFFSET" OR (t$ = "OFFSET" AND qb64prefix_set = 1) THEN t = OFFSETTYPE
    IF t THEN
        IF u THEN t = t + ISUNSIGNED
        typname2typ& = t
        EXIT FUNCTION
    END IF
    IF u THEN EXIT FUNCTION '_UNSIGNED (nothing)

    'UDT?
    FOR i = 1 TO lasttype
        IF t$ = RTRIM$(udtxname(i)) THEN
            typname2typ& = ISUDT + ISPOINTER + i
            EXIT FUNCTION
        ELSEIF RTRIM$(udtxname(i)) = "_MEM" AND t$ = "MEM" AND qb64prefix_set = 1 THEN
            typname2typ& = ISUDT + ISPOINTER + i
            EXIT FUNCTION
        END IF
    NEXT

    'return 0 (failed)
END FUNCTION

FUNCTION removesymbol$ (varname$)
    i = INSTR(varname$, "~"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "`"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "%"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "&"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "!"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "#"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "$"): IF i THEN GOTO foundsymbol
    EXIT FUNCTION
    foundsymbol:
    IF i = 1 THEN Give_Error "Expected variable name before symbol": EXIT FUNCTION
    symbol$ = RIGHT$(varname$, LEN(varname$) - i + 1)
    IF symboltype(symbol$) = 0 THEN Give_Error "Invalid symbol": EXIT FUNCTION
    removesymbol$ = symbol$
    varname$ = LEFT$(varname$, i - 1)
END FUNCTION

' 
' Does not report an error if the symbol is invalid or varname is blank
'
FUNCTION tryRemoveSymbol$ (varname$)
    i = INSTR(varname$, "~"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "`"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "%"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "&"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "!"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "#"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "$"): IF i THEN GOTO foundsymbol
    EXIT FUNCTION
    foundsymbol:
    symbol$ = RIGHT$(varname$, LEN(varname$) - i + 1)
    IF symboltype(symbol$) = 0 THEN EXIT FUNCTION
    tryRemoveSymbol$ = symbol$
    varname$ = LEFT$(varname$, i - 1)
END FUNCTION

SUB increaseUDTArrays
    x = UBOUND(udtxname)
    REDIM _PRESERVE udtxname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtxcname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtxsize(x + 1000) AS LONG
    REDIM _PRESERVE udtxbytealign(x + 1000) AS INTEGER 'first element MUST be on a byte alignment & size is a multiple of 8
    REDIM _PRESERVE udtxnext(x + 1000) AS LONG
    REDIM _PRESERVE udtxvariable(x + 1000) AS INTEGER 'true if the udt contains variable length elements
    'elements
    REDIM _PRESERVE udtename(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtecname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtebytealign(x + 1000) AS INTEGER
    REDIM _PRESERVE udtesize(x + 1000) AS LONG
    REDIM _PRESERVE udtetype(x + 1000) AS LONG
    REDIM _PRESERVE udtetypesize(x + 1000) AS LONG
    REDIM _PRESERVE udtearrayelements(x + 1000) AS LONG
    REDIM _PRESERVE udtenext(x + 1000) AS LONG
END SUB

SUB initialise_udt_varstrings (n$, udt, buf, base_offset)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    element = udtxnext(udt)
    offset = 0
    DO WHILE element
        IF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                WriteBufLine buf, "*(qbs**)(((char*)" + n$ + ")+" + STR$(base_offset + offset) + ") = qbs_new(0,0);"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            initialise_udt_varstrings n$, udtetype(element) AND 511, buf, offset
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB free_udt_varstrings (n$, udt, buf, base_offset)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    element = udtxnext(udt)
    offset = 0
    DO WHILE element
        IF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                WriteBufLine buf, "qbs_free(*((qbs**)(((char*)" + n$ + ")+" + STR$(base_offset + offset) + ")));"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            initialise_udt_varstrings n$, udtetype(element) AND 511, buf, offset
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB clear_udt_with_varstrings (n$, udt, buf, base_offset)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    element = udtxnext(udt)
    offset = 0
    DO WHILE element
        IF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                WriteBufLine buf, "(*(qbs**)(((char*)" + n$ + ")+" + STR$(base_offset + offset) + "))->len=0;"
            ELSE
                WriteBufLine buf, "memset((char*)" + n$ + "+" + STR$(base_offset + offset) + ",0," + STR$(udtesize(element) \ 8) + ");"
            END IF
        ELSE
            IF udtetype(element) AND ISUDT THEN
                clear_udt_with_varstrings n$, udtetype(element) AND 511, buf, base_offset + offset
            ELSE
                WriteBufLine buf, "memset((char*)" + n$ + "+" + STR$(base_offset + offset) + ",0," + STR$(udtesize(element) \ 8) + ");"
            END IF
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB initialise_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                acc$ = acc$ + CHR$(13) + CHR$(10) + "*(qbs**)(" + n$ + "[0]+(" + bytesperelement$ + "-1)*tmp_long+" + STR$(offset) + ")=qbs_new(0,0);"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            initialise_array_udt_varstrings n$, udtetype(element) AND 511, offset, bytesperelement$, acc$
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB free_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                acc$ = acc$ + CHR$(13) + CHR$(10) + "qbs_free(*(qbs**)(" + n$ + "[0]+(" + bytesperelement$ + "-1)*tmp_long+" + STR$(offset) + "));"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            free_array_udt_varstrings n$, udtetype(element) AND 511, offset, bytesperelement$, acc$
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB copy_full_udt (dst$, src$, buf, base_offset, udt)
    IF NOT udtxvariable(udt) THEN
        WriteBufLine buf, "memcpy(" + dst$ + "+" + STR$(base_offset) + "," + src$ + "+" + STR$(base_offset) + "," + STR$(udtxsize(udt) \ 8) + ");"
        EXIT SUB
    END IF
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF ((udtetype(element) AND ISSTRING) > 0) AND (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
            WriteBufLine buf, "qbs_set(*(qbs**)(" + dst$ + "+" + STR$(offset) + "), *(qbs**)(" + src$ + "+" + STR$(offset) + "));"
        ELSEIF ((udtetype(element) AND ISUDT) > 0) THEN
            copy_full_udt dst$, src$, MainTxtBuf, offset, udtetype(element) AND 511
        ELSE
            WriteBufLine buf, "memcpy((" + dst$ + "+" + STR$(offset) + "),(" + src$ + "+" + STR$(offset) + ")," + STR$(udtesize(element) \ 8) + ");"
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB dump_udts
    fh = FREEFILE
    OPEN "types.txt" FOR OUTPUT AS #fh
    PRINT #fh, "Name   Size   Align? Next   Var?"
    FOR i = 1 TO lasttype
        PRINT #fh, RTRIM$(udtxname(i)), udtxsize(i), udtxbytealign(i), udtxnext(i), udtxvariable(i)
    NEXT i
    PRINT #fh, "Name   Size   Align? Next   Type   Tsize  Arr"
    FOR i = 1 TO lasttypeelement
        PRINT #fh, RTRIM$(udtename(i)), udtesize(i), udtebytealign(i), udtenext(i), udtetype(i), udtetypesize(i), udtearrayelements(i)
    NEXT i
    CLOSE #fh
END SUB

FUNCTION isuinteger (i$)
    IF LEN(i$) = 0 THEN EXIT FUNCTION
    IF ASC(i$, 1) = 48 AND LEN(i$) > 1 THEN EXIT FUNCTION
    FOR c = 1 TO LEN(i$)
        v = ASC(i$, c)
        IF v < 48 OR v > 57 THEN EXIT FUNCTION
    NEXT
    isuinteger = -1
END FUNCTION
