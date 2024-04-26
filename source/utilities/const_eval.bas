
TYPE ParserState
    index AS LONG
    strIndex AS LONG
    num AS ParseNum
    errStr AS STRING
    result AS STRING
END TYPE

'Steve Subs/Functins for _MATH support with CONST
FUNCTION Evaluate_Expression$ (e$, num AS ParseNum)
    t$ = e$ 'So we preserve our original data, we parse a temp copy of it

    PreParse t$

    IF CONST_EVAL_DEBUG THEN _Echo "t$: " + t$
    IF LEFT$(t$, 5) = "ERROR" THEN Evaluate_Expression$ = t$: EXIT FUNCTION

    'Deal with brackets first
    exp$ = "(" + sp + t$ + sp + ")" 'Starting and finishing brackets for our parse routine.
    IF CONST_EVAL_DEBUG THEN  _Echo "exp$: " + exp$

    DIM Eval_E AS LONG, c AS LONG
    DO
        FindInnerParens exp$, C, Eval_E

        IF Eval_E > 0 THEN
            IF c = 0 THEN Evaluate_Expression$ = "ERROR - BAD () Count": EXIT FUNCTION
            eval$ = getelements$(exp$, c + 1, Eval_E - 1)

            ParseExpression2 eval$

            eval$ = LTRIM$(RTRIM$(eval$))
            IF LEFT$(eval$, 5) = "ERROR" THEN Evaluate_Expression$ = eval$: EXIT FUNCTION

            ' Check if element preceding the parens is a known function name
            ' If so, evaluate it now using the argument list we have
            funcOp& = IsFunctionIdentifier(getelement$(exp$, c - 1))
            IF funcOp& > 0 THEN
                eval$ = EvaluateFunction$(funcOp&, eval$)
                IF LEFT$(eval$, 5) = "ERROR" THEN Evaluate_Expression$ = eval$: EXIT FUNCTION

                c = c - 1
            END IF

            IF CONST_EVAL_DEBUG THEN _Echo "eval$: " + eval$
            leftele$ = getelements$(exp$, 1, c - 1)
            rightele$ = getelements$(exp$, Eval_E + 1, numelements(exp$))

            exp$ = leftele$
            IF exp$ <> "" THEN exp$ = exp$ + sp
            exp$ = exp$ + eval$
            IF rightele$ <> "" THEN exp$ = exp$ + sp + rightele$
        END IF
    LOOP UNTIL Eval_E = 0

    IF CONST_EVAL_DEBUG THEN _Echo "resulting exp$: " + exp$ + ", numelements: " + str$(numelements(exp$))
    IF numelements(exp$) <> 1 THEN
        Evaluate_Expression$ = "ERROR - Invalid characters in expression": EXIT FUNCTION
    END IF

    IF elementIsString&(exp$) THEN
        num.typ = STRINGTYPE
        num.s = exp$
    ELSE
        num.typ = elementGetNumericValue&(exp$, num.f, num.i, num.ui)
    END IF

    Evaluate_Expression$ = exp$
END FUNCTION

' Finds an innermost set of parens, returns the element indexes of the parens
'
' Gives 0 as startParen if there are no matching parens
SUB FindInnerParens (exp$, startParen AS LONG, endParen AS LONG)
    startParen = 0
    endParen = 0

    strIndex = 0
    paren = 0
    DO
        ele$ = getnextelement$(exp$, paren, strIndex)

        IF paren = -1 THEN EXIT SUB
        IF ele$ = ")" THEN endParen = paren: EXIT DO
        IF paren > 1000 THEN EXIT SUB
    LOOP

    strIndex = 0
    paren = 0
    DO
        ele$ = getprevelement$(exp$, paren, strIndex)

        ' Skip parens until we reach the ")" we found in the previous search
        IF paren > endParen THEN _CONTINUE

        IF paren = -1 THEN EXIT SUB
        IF ele$ = "(" THEN startParen = paren: EXIT DO
        IF paren > 1000 THEN EXIT SUB
    LOOP
END SUB

' Grammar (excludes parens)
'
' comma_expression := expression ',' expression
'                       | expression
'
' expression := imp_logical
'                   | str_add
'
' str_add := primary_str '+' primary_str
'               | primary_str
'
' imp_logical := eqv_logical 'IMP' eqv_logical
'                   | eqv_logical
'
' eqv_logical := xor_logical 'EQV' xor_logical
'                   | xor_logical
'
' xor_logical := or_logical 'XOR' or_logical
'                   | or_logical
'
' or_logical := and_logical 'OR' and_logical
'                   | and_logical
'
' and_logical := not_logical 'AND' not_logical
'                   | not_logical
'
' not_logical := 'NOT' relation
'               | relation
'
' relation := term '<>' term
'               | term '><' term
'               | term '<=' term
'               | term '>=' term
'               | term '=<' term
'               | term '=>' term
'               | term '>' term
'               | term '<' term
'               | term '=' term
'               | term
'
' term := mod '+' mod | mod '-' mod | mod
'
' mod := int_div 'MOD' int_div | int_div
'
' int_div := factor '\' factor | factor
'
' factor := unary '*' unary
'               | unary '/' unary
'               | unary
'
' unary := '-' exponent | exponent
'
' ' Note: NOT is a special case here similar to -, but it is handled in
' ' PreParse via parenthesis insertion
' exponent := numeric '^' unary
'               | numeric 'ROOT' unary
'               | numeric 
'
' numeric := NUMBER | CONST_VAR
'
' string := STRING | CONST_VAR

FUNCTION CommaExpression&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "CommaExpression"
    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)

        DIM tmpIndex AS LONG
        tmpIndex = state.index

        IF StrExpression&(exp$, state) = 0 THEN
            ' If StrExpression consumed any tokens and failed, then it's a real error
            IF state.index <> tmpIndex THEN EXIT FUNCTION
            IF NumericExpression&(exp$, state) = 0 THEN EXIT FUNCTION
        END IF

        IF state.result <> "" THEN pushelement state.result, ","

        IF (state.num.typ AND ISSTRING) THEN
            pushelement state.result, state.num.s
        ELSEIF (state.num.typ AND ISFLOAT) THEN
            pushelement state.result, _TRIM$(STR$(state.num.f))
        ELSEIF (state.num.typ AND ISUNSIGNED) THEN
            pushelement state.result, _TRIM$(STR$(state.num.ui)) + "~&&"
        ELSE
            pushelement state.result, _TRIM$(STR$(state.num.i)) + "&&"
        END IF

        ele$ = getnextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "," THEN _CONTINUE

        ' If we parsed all the tokens in the string, state.index should be -1
        '
        ' If there are still tokens left and it's not a comma, then something
        ' is broken
        IF state.index = -1 THEN
            CommaExpression& = -1
        ELSE
            state.errStr = "ERROR - Unexpected element '" + ele$ + "'"
        END IF
        EXIT FUNCTION
    LOOP
END FUNCTION

FUNCTION StrExpression&(exp$, state AS ParserState)
    IF ParseString&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM s AS STRING
    s = state.num.s

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "+" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF ParseString&(exp$, state) = 0 THEN FixupErrorMessage state, "+": EXIT FUNCTION

            s = elementStringConcat$(s, state.num.s)
        ELSE
            state.num.s = s
            StrExpression& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION ParseString&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "ParseString"
    ele$ = peeknextelement$(exp$, state.index, state.strIndex)

    IF elementIsString(ele$) THEN
        ele$ = getnextelement$(exp$, state.index, state.strIndex)

        ParseNumSetS state.num, ele$

        ParseString& = -1
    ELSE
        IF ParseNumHashLookup&(ele$, state) THEN
            IF (state.num.typ AND ISSTRING) = 0 THEN state.errStr = "ERROR - Expecting a string value": EXIT FUNCTION

            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            ParseString& = -1
            EXIT FUNCTION
        END IF

        state.errStr = "ERROR - Unexpected element '" + ele$ + "'"
    END IF
END FUNCTION

FUNCTION NumericExpression&(exp$, state AS ParserState)
    NumericExpression& = LogicalImp&(exp$, state)
END FUNCTION

FUNCTION LogicalImp&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "LogicalImp"
    IF LogicalEqv&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "IMP" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF LogicalEqv&(exp$, state) = 0 THEN FixupErrorMessage state, "IMP": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui IMP state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i IMP state.num.i
            END IF
        ELSE
            state.num = num
            LogicalImp& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION LogicalEqv&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "LogicalEqv"
    IF LogicalXor&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "EQV" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF LogicalXor&(exp$, state) = 0 THEN FixupErrorMessage state, "EQV": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui EQV state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i EQV state.num.i
            END IF
        ELSE
            state.num = num
            LogicalEqv& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION LogicalXor&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "LogicalXor"
    IF LogicalOr&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "XOR" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF LogicalOr&(exp$, state) = 0 THEN FixupErrorMessage state, "XOR": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui XOR state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i XOR state.num.i
            END IF
        ELSE
            state.num = num
            LogicalXor& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION LogicalOr&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "LogicalOr"
    IF LogicalAnd&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "OR" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF LogicalAnd&(exp$, state) = 0 THEN FixupErrorMessage state, "OR": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui OR state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i OR state.num.i
            END IF
        ELSE
            state.num = num
            LogicalOr& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION LogicalAnd&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "LogicalAnd"
    IF LogicalNot&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "AND" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF LogicalNot&(exp$, state) = 0 THEN FixupErrorMessage state, "AND": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui AND state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i AND state.num.i
            END IF
        ELSE
            state.num = num
            LogicalAnd& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION LogicalNot&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "LogicalNot"
    ele$ = peeknextelement$(exp$, state.index, state.strIndex)
    IF ele$ = "NOT" THEN ele$ = getnextelement$(exp$, state.index, state.strIndex)

    IF Relation&(exp$, state) = 0 THEN FixupErrorMessage state, "NOT": EXIT FUNCTION

    IF ele$ = "NOT" THEN 
        IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
            ParseNumSetUI state.num, UINTEGER64TYPE - ISPOINTER, NOT state.num.ui
        ELSE
            ParseNumSetI state.num, INTEGER64TYPE - ISPOINTER, NOT state.num.i
        END IF
    END IF

    LogicalNot& = -1
END FUNCTION

FUNCTION Relation&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "Relation"
    IF Term&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "<>" OR ele$ = "><" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Term&(exp$, state) = 0 THEN FixupErrorMessage state, "<>": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f <> state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui <> state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i <> state.num.i
            END IF
        ELSEIF ele$ = ">=" OR ele$ = "=>" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Term&(exp$, state) = 0 THEN FixupErrorMessage state, ">=": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f >= state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui >= state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i >= state.num.i
            END IF
        ELSEIF ele$ = "<=" OR ele$ = "=<" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Term&(exp$, state) = 0 THEN FixupErrorMessage state, "<=": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f <= state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui <= state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i <= state.num.i
            END IF
        ELSEIF ele$ = "<" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Term&(exp$, state) = 0 THEN FixupErrorMessage state, "<": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f < state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui < state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i < state.num.i
            END IF
        ELSEIF ele$ = ">" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Term&(exp$, state) = 0 THEN FixupErrorMessage state, ">": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f > state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui > state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i > state.num.i
            END IF
        ELSEIF ele$ = "=" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Term&(exp$, state) = 0 THEN FixupErrorMessage state, "=": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f = state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui = state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i = state.num.i
            END IF
        ELSE
            state.num = num
            Relation& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION Term&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "Term"
    IF ParseMod&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "+" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF ParseMod&(exp$, state) = 0 THEN FixupErrorMessage state, "+": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f + state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui + state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i + state.num.i
            END IF
        ELSEIF ele$ = "-" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF ParseMod&(exp$, state) = 0 THEN FixupErrorMessage state, "-": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f - state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui - state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i - state.num.i
            END IF
        ELSE
            state.num = num
            Term& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION ParseMod&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "ParseMod"
    IF IntDiv&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "MOD" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF IntDiv&(exp$, state) = 0 THEN FixupErrorMessage state, "MOD": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui MOD state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i MOD state.num.i
            END IF
        ELSE
            IF CONST_EVAL_DEBUG THEN _Echo "ParseMod done!"
            state.num = num
            ParseMod& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTIOn

FUNCTION IntDiv&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "IntDiv"
    IF Factor&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "\" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Factor&(exp$, state) = 0 THEN FixupErrorMessage state, "\": EXIT FUNCTION

            IF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui \ state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i \ state.num.i
            END IF
        ELSE
            IF CONST_EVAL_DEBUG THEN  _Echo "IntDiv done!"
            state.num = num
            IntDiv& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION Factor&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "Factor"
    IF Unary&(exp$, state) = 0 THEN EXIT FUNCTION

    DIM num As ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF ele$ = "*" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Unary&(exp$, state) = 0 THEN FixupErrorMessage state, "*": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f * state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui * state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i * state.num.i
            END IF
        ELSEIF ele$ = "/" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF Unary&(exp$, state) = 0 THEN FixupErrorMessage state, "/": EXIT FUNCTION

            ' Regular division is always done as floating-point
            ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f / state.num.f
        ELSE
            IF CONST_EVAL_DEBUG THEN  _Echo "Factor done!"
            state.num = num
            Factor& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION Unary&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "Unary"
    ele$ = peeknextelement$(exp$, state.index, state.strIndex)
    IF ele$ = "-" THEN ele$ = getnextelement$(exp$, state.index, state.strIndex)

    IF Exponent&(exp$, state) = 0 THEN FixupErrorMessage state, "-": EXIT FUNCTION

    IF ele$ = "-" THEN ParseNumSetI state.num, INTEGER64TYPE - ISPOINTER, -state.num.i
    Unary& = -1
    IF CONST_EVAL_DEBUG THEN  _Echo "Unary done!"
END FUNCTION

FUNCTION Exponent&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "Exponent"
    IF Numeric&(exp$, state) = 0 THEN EXIT FUNCTION
    IF CONST_EVAL_DEBUG THEN _Echo "Check exponent"

    DIM num AS ParseNum
    num = state.num

    DO
        ele$ = peeknextelement$(exp$, state.index, state.strIndex)
        IF CONST_EVAL_DEBUG THEN  _Echo "Exponent ele! " + ele$
        IF ele$ = "^" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)

            ' Right associative - 2 ^ 3 ^ 6 is (2 ^ (3 ^ 6))
            '
            ' Unary accounts for the special case of `2 ^ -6`, where negative
            ' applies before the exponent
            IF Unary&(exp$, state) = 0 THEN FixupErrorMessage state, "^": EXIT FUNCTION

            IF (num.typ AND ISFLOAT) OR (state.num.typ AND ISFLOAT) THEN
                ParseNumSetF num, FLOATTYPE - ISPOINTER, num.f ^ state.num.f
            ELSEIF (num.typ AND ISUNSIGNED) OR (state.num.typ AND ISUNSIGNED) THEN
                ParseNumSetUI num, UINTEGER64TYPE - ISPOINTER, num.ui ^ state.num.ui
            ELSE
                ParseNumSetI num, INTEGER64TYPE - ISPOINTER, num.i ^ state.num.i
            END IF
        ELSEIF ele$ = "ROOT" THEN
            ele$ = getnextelement$(exp$, state.index, state.strIndex)

            ' Right associative - 2 ROOT 3 ROOT 6 is (2 ROOT (3 ROOT 6))
            '
            ' Unary accounts for the special case of `2 ROOT -6`, where negative
            ' applies before the exponent
            IF Unary&(exp$, state) = 0 THEN FixupErrorMessage state, "ROOT": EXIT FUNCTION

            IF num.f < 0 AND state.num.f >= 1 THEN sig = -1: num.f = -num.f ELSE sig = 1
            expon## = (1## / state.num.f)
            IF expon## <> INT(expon##) AND state.num.f < 1 THEN sig = SGN(num.f): num.f = ABS(num.f)

            ParseNumSetF num, FLOATTYPE - ISPOINTER, sig * (num.f ^ expon##)
        ELSE
            IF CONST_EVAL_DEBUG THEN  _Echo "Exponent done!"
            state.num = num
            Exponent& = -1
            EXIT FUNCTION
        END IF
    LOOP
END FUNCTION

FUNCTION Numeric&(exp$, state AS ParserState)
    IF CONST_EVAL_DEBUG THEN _Echo "Numeric"
    ele$ = peeknextelement$(exp$, state.index, state.strIndex)
    IF CONST_EVAL_DEBUG THEN _Echo "Numeric peek ele: " + ele$

    IF elementIsNumber(ele$) THEN
        ele$ = getnextelement$(exp$, state.index, state.strIndex)

        state.num.typ = elementGetNumericValue(ele$, state.num.f, state.num.i, state.num.ui)

        Numeric& = -1
    ELSEIF ele$ = "_PI" OR (qb64prefix_set = 1 AND ele$ = "PI") THEN
        ele$ = getnextelement$(exp$, state.index, state.strIndex)

        ParseNumSetF state.num, FLOATTYPE - ISPOINTER, 3.14159265358979323846264338327950288##
        Numeric& = -1
        EXIT FUNCTION
    ELSE
        IF ParseNumHashLookup&(ele$, state) THEN
            IF state.num.typ AND ISSTRING THEN state.errStr = "ERROR - String can not be in numeric operation": EXIT FUNCTION

            ele$ = getnextelement$(exp$, state.index, state.strIndex)
            IF CONST_EVAL_DEBUG THEN _Echo "Consumed ele: " + ele$
            Numeric& = -1
            EXIT FUNCTION
        END IF

        state.errStr = "ERROR - Unexpected element '" + ele$ + "'"
        state.num.s = ele$
    END IF
END FUNCTION

FUNCTION ParseNumHashLookup&(ele$, state AS ParserState)
    ' Attempt hash lookup of existing CONST name
    ' CONST can be global or belong to the current sub/function
    hashfound = 0

    hashname$ = ele$
    unusedSymbol$ = tryRemoveSymbol$(hashname$)

    IF CONST_EVAL_DEBUG THEN _Echo "hash lookup: " + hashname$
    IF CONST_EVAL_DEBUG THEN _Echo "unused symbol: " + hashname$
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

    IF CONST_EVAL_DEBUG THEN _Echo "Hashfound: " + str$(hashfound)
    IF hashfound THEN
        IF CONST_EVAL_DEBUG THEN _Echo "is string: " + str$(consttype(hashresref) AND ISSTRING)

        IF consttype(hashresref) AND ISSTRING THEN
            ParseNumSetS state.num, conststring(hashresref)
        ELSEIF consttype(hashresref) AND ISFLOAT THEN
            ParseNumSetF state.num, consttype(hashresref), constfloat(hashresref)
        ELSE
            IF consttype(hashresref) AND ISUNSIGNED THEN
                ParseNumSetUI state.num, consttype(hashresref), constuinteger(hashresref)
            ELSE
                ParseNumSetI state.num, consttype(hashresref),  constinteger(hashresref)
            END IF
        END IF

        IF CONST_EVAL_DEBUG THEN _Echo "Found! value: " + str$(state.num.f) + state.num.s

        ParseNumHashLookup& = -1
        EXIT FUNCTION
    END IF
END FUNCTION

SUB ParseNumSetF (num AS ParseNum, t AS LONG, f AS _FLOAT)
    num.s = ""
    num.f = f
    num.i = f
    num.ui = f
    num.typ = t
END SUB

SUB ParseNumSetI (num AS ParseNum, t AS LONG, i AS _INTEGER64)
    num.s = ""
    num.i = i
    num.ui = i
    num.f = i
    num.typ = t
END SUB

SUB ParseNumSetUI (num AS ParseNum, t AS LONG, ui AS _UNSIGNED _INTEGER64)
    num.ui = ui
    num.i = ui
    num.f = ui
    num.typ = t
END SUB

SUB ParseNumSetS (num AS ParseNum, s AS STRING)
    num.s = s
    num.typ = STRINGTYPE
END SUB

SUB FixupErrorMessage (state AS ParserState, op AS STRING)
    IF state.num.s = "" THEN state.errStr = "ERROR - Expected variable/value after '" + op + "'"
END SUB

SUB ParseExpression2 (exp$)
    exp$ = DWD(exp$)

    DIM state AS ParserState
    state.index = 0
    state.strIndex = 0
    state.num.f = 0
    state.num.i = 0
    state.num.typ = 0
    state.errStr = ""
    state.result = ""

    res& = CommaExpression&(exp$, state)
    IF CONST_EVAL_DEBUG THEN _Echo "res: " + STR$(res&)
    IF CONST_EVAL_DEBUG THEN _Echo "resulting string: " + state.result
    IF CONST_EVAL_DEBUG THEN _Echo "resulting err: " + state.errStr

    IF res& = 0 THEN
        exp$ = state.errStr
    ELSE
        exp$ = state.result
    END IF
END SUB

SUB Set_ConstFunctions
    REDIM ConstFuncs(10000) AS ConstFunction

    'Functions with PL 10
    i = i + 1: ConstFuncs(i).nam = "_PI": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ACOS": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ASIN": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ARCSEC": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ARCCSC": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ARCCOT": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_SECH": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_CSCH": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_COTH": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "COS": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "SIN": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "TAN": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "LOG": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "EXP": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "ATN": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "SQR": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_D2R": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_D2G": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_R2D": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_R2G": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_G2D": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_G2R": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "ABS": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "SGN": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "INT": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ROUND": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_CEIL": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "FIX": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_SEC": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_CSC": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_COT": ConstFuncs(i).ArgCount = 1

    i = i + 1: ConstFuncs(i).nam = "_RGB32": ConstFuncs(i).ArgCount = -1
    i = i + 1: ConstFuncs(i).nam = "_RGBA32": ConstFuncs(i).ArgCount = 4
    i = i + 1: ConstFuncs(i).nam = "_RGBA": ConstFuncs(i).ArgCount = 5
    i = i + 1: ConstFuncs(i).nam = "_RGB": ConstFuncs(i).ArgCount = 4
    i = i + 1: ConstFuncs(i).nam = "_RED32": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_GREEN32": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_BLUE32": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_ALPHA32": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "_RED": ConstFuncs(i).ArgCount = 2
    i = i + 1: ConstFuncs(i).nam = "_GREEN": ConstFuncs(i).ArgCount = 2
    i = i + 1: ConstFuncs(i).nam = "_BLUE": ConstFuncs(i).ArgCount = 2
    i = i + 1: ConstFuncs(i).nam = "_ALPHA": ConstFuncs(i).ArgCount = 2

    i = i + 1: ConstFuncs(i).nam = "CHR$": ConstFuncs(i).ArgCount = 1
    i = i + 1: ConstFuncs(i).nam = "ASC": ConstFuncs(i).ArgCount = -1

    REDIM _PRESERVE ConstFuncs(i) AS ConstFunction
END SUB

' args should be an element list of the arguments separated by commas
'
' Each argument should be a single element
FUNCTION EvaluateFunction$ (p, args AS STRING)
    DIM n1 AS _FLOAT, nstr AS STRING
    Dim argCount As Long, args(5) As ParseNum, origArgs(5) As String


    argCount = countFunctionElements(args)

    IF CONST_EVAL_DEBUG THEN _Echo "argCount: " + str$(argCount)

    IF ConstFuncs(p).ArgCount > 0 AND argCount <> ConstFuncs(p).ArgCount THEN
        EvaluateFunction$ = "ERROR - Wrong number of arguments provided to " + ConstFuncs(p).nam + "!"
        EXIT FUNCTION
    END IF

    FOR i = 1 to argCount
        ele$ = getelement$(args, 1 + (i - 1) * 2)
        origArgs(i) = ele$

        IF CONST_EVAL_DEBUG THEN _Echo "arg is string: " + STR$(elementIsString(ele$)) + ", argCount: " + STR$(ConstFuncs(p).ArgCount)

        IF elementIsNumber(ele$) THEN
            ' skip the commas
            args(i).typ = elementGetNumericValue&(ele$, args(i).f, args(i).i, args(i).ui)
        ELSEIF elementIsString(ele$) AND ConstFuncs(p).ArgCount < 0 THEN ' positive arg count means arguments are all numbers
            args(i).typ = elementGetStringValue&(ele$, args(i).s)
        ELSE
            EvaluateFunction$ = "ERROR - Unexpected argument: " + ele$
            EXIT FUNCTION
        END IF

        IF CONST_EVAL_DEBUG THEN _Echo "Argument: " + str$(args(i).f) + ", str: " + getelement$(args, 1 + (i - 1) * 2)
    NEXT

    ' Default type, some functions return different types
    typ& = FLOATTYPE - ISPOINTER

    SELECT CASE ConstFuncs(p).nam 'Depending on our operator..
        CASE "_PI"
            'Future compatible in case something ever stores extra digits for PI
            n1 = 3.14159265358979323846264338327950288## * args(1).f

        CASE "_ACOS": n1 = _ACOS(args(1).f)
        CASE "_ASIN": n1 = _ASIN(args(1).f)
        CASE "_ARCSEC"
            IF ABS(args(1).f) < 1 THEN EvaluateFunction$ = "ERROR - ABS(_ARCSEC) value < 1": EXIT FUNCTION
            n1 = _ARCSEC(args(1).f)

        CASE "_ARCCSC"
            IF ABS(args(1).f) < 1 THEN EvaluateFunction$ = "ERROR - ABS(_ARCCSC) value < 1": EXIT FUNCTION
            n1 = _ARCCSC(args(1).f)

        CASE "_ARCCOT": n1 = _ARCCOT(args(1).f)
        CASE "_SECH": n1 = _SECH(args(1).f)
        CASE "_CSCH": n1 = _CSCH(args(1).f)
        CASE "_COTH": n1 = _COTH(args(1).f)
        CASE "_RGB32"
            typ& = INTEGER64TYPE - ISPOINTER
            SELECT CASE argCount
                CASE 1
                    n1 = _RGB32(args(1).ui, args(1).ui, args(1).ui)

                CASE 2
                    n1 = _RGB32(args(1).ui, args(1).ui, args(1).ui, args(2).ui)

                CASE 3
                    n1 = _RGB32(args(1).ui, args(2).ui, args(3).ui)

                CASE 4
                    n1 = _RGB32(args(1).ui, args(2).ui, args(3).ui, args(4).ui)

                CASE ELSE
                    EvaluateFunction$ = "ERROR - Invalid comma count (" + args + ")"
                    EXIT FUNCTION
            END SELECT

        CASE "_RGBA32"
            typ& = INTEGER64TYPE - ISPOINTER
            'we have to have 3 commas; not more, not less.
            n1 = _RGBA32(args(1).ui, args(2).ui, args(3).ui, args(4).ui)

        CASE "_RGB"
            typ& = INTEGER64TYPE - ISPOINTER
            'we have to have 3 commas; not more, not less.
            SELECT CASE args(4).ui
                CASE 0 TO 2, 7 TO 13, 256, 32 'these are the good screen values
                CASE ELSE
                    EvaluateFunction$ = "ERROR - Invalid Screen Mode (" + STR$(args(4).ui) + ")": EXIT FUNCTION
            END SELECT
            t = _NEWIMAGE(1, 1, args(4).ui)
            n1 = _RGB(args(1).ui, args(2).ui, args(3).ui, t)
            _FREEIMAGE t

        CASE "_RGBA"
            typ& = INTEGER64TYPE - ISPOINTER
            'we have to have 4 commas; not more, not less.
            SELECT CASE args(5).ui
                CASE 0 TO 2, 7 TO 13, 256, 32 'these are the good screen values
                CASE ELSE
                    EvaluateFunction$ = "ERROR - Invalid Screen Mode (" + STR$(args(5).ui) + ")": EXIT FUNCTION
            END SELECT
            t = _NEWIMAGE(1, 1, args(5).ui)
            n1 = _RGBA(args(1).ui, args(2).ui, args(3).ui, args(4).ui, t)
            _FREEIMAGE t

        CASE "_RED", "_GREEN", "_BLUE", "_ALPHA"
            typ& = INTEGER64TYPE - ISPOINTER
            SELECT CASE args(2).i
                CASE 0 TO 2, 7 TO 13, 256, 32 'these are the good screen values
                CASE ELSE
                    EvaluateNumbers$ = "ERROR - Invalid Screen Mode (" + STR$(args(2).i) + ")": EXIT FUNCTION
            END SELECT
            t = _NEWIMAGE(1, 1, args(2).i)
            SELECT CASE ConstFuncs(p).nam
                CASE "_RED": n1 = _RED(args(1).i, t)
                CASE "_BLUE": n1 = _BLUE(args(1).i, t)
                CASE "_GREEN": n1 = _GREEN(args(1).i, t)
                CASE "_ALPHA": n1 = _ALPHA(args(1).i, t)
            END SELECT
            _FREEIMAGE t

        CASE "_RED32", "_GREEN32", "_BLUE32", "_ALPHA32"
            typ& = INTEGER64TYPE - ISPOINTER
            SELECT CASE ConstFuncs(p).nam
                CASE "_RED32": n1 = _RED32(args(1).i)
                CASE "_BLUE32": n1 = _BLUE32(args(1).i)
                CASE "_GREEN32": n1 = _GREEN32(args(1).i)
                CASE "_ALPHA32": n1 = _ALPHA32(args(1).i)
            END SELECT

        CASE "COS": n1 = COS(args(1).f)
        CASE "SIN": n1 = SIN(args(1).f)
        CASE "TAN": n1 = TAN(args(1).f)
        CASE "LOG": n1 = LOG(args(1).f)
        CASE "EXP": n1 = EXP(args(1).f)
        CASE "ATN": n1 = ATN(args(1).f)
        CASE "SQR": n1 = SQR(args(1).f)
        CASE "_D2R": n1 = 0.0174532925 * args(1).f
        CASE "_D2G": n1 = 1.1111111111 * args(1).f
        CASE "_R2D": n1 = 57.2957795 * args(1).f
        CASE "_R2G": n1 = 0.015707963 * args(1).f
        CASE "_G2D": n1 = 0.9 * args(1).f
        CASE "_G2R": n1 = 63.661977237 * args(1).f
        CASE "ABS": n1 = ABS(args(1).f): typ& = INTEGER64TYPE - ISPOINTER
        CASE "SGN": n1 = SGN(args(1).f): typ& = INTEGER64TYPE - ISPOINTER
        CASE "INT": n1 = INT(args(1).f): typ& = INTEGER64TYPE - ISPOINTER
        CASE "_ROUND": n1 = _ROUND(args(1).f): typ& = INTEGER64TYPE - ISPOINTER
        CASE "_CEIL": n1 = _CEIL(args(1).f): typ& = INTEGER64TYPE - ISPOINTER
        CASE "FIX": n1 = FIX(args(1).f): typ& = INTEGER64TYPE - ISPOINTER
        CASE "_SEC": n1 = _SEC(args(1).f)
        CASE "_CSC": n1 = _CSC(args(1).f)
        CASE "_COT": n1 = _COT(args(1).f)

        CASE "CHR$":
            IF args(1).ui > 255 THEN EvaluateFunction$ = "ERROR - Invalid argument to CHR$, valid range is 0-255: " + origArgs(1): EXIT FUNCTION

            nstr = CHR$(args(1).ui)
            typ& = STRINGTYPE

        CASE "ASC":
            IF argCount < 1 OR argCount > 2 THEN EvaluateNumbers$ = "ERROR - Wrong number of arguments provided to ASC$": EXIT FUNCTION
            IF (args(1).typ AND ISSTRING) = 0 THEN EvaluateFunction$ = "ERROR - Unexpected argument: '" + origArgs(1) + "'": EXIT FUNCTION

            IF argCount = 1 THEN
                n1 = ASC(args(1).s)
            ELSE
                IF args(2).typ AND ISSTRING THEN EvaluateFunction$ = "ERROR - Expected integer argument: '" + origArgs(2) + "'": EXIT FUNCTION

                n1 = ASC(args(1).s, args(2).i)
            END IF

            typ& = INTEGER64TYPE - ISPOINTER
    END SELECT

    IF typ& AND ISSTRING THEN
        EvaluateFunction$ = createElementString$(nstr)
    ELSEIF typ& AND ISFLOAT THEN
        EvaluateFunction$ = _TRIM$(STR$(n1))
    ELSE
        n&& = n1
        EvaluateFunction$ = _TRIM$(STR$(n&&)) + "&&"
    END IF
END FUNCTION

FUNCTION DWD$ (exp$) 'Deal With Duplicates
    'To deal with duplicate operators in our code.
    'Such as --  becomes a +
    '++ becomes a +
    '+- becomes a -
    '-+ becomes a -
    t$ = exp$
    FOR l = 1 TO numelements(t$) - 1
        ele$ = getelement$(t$, l)
        nextele$ = getelement$(t$, l + 1)

        IF ele$ = "+" AND nextele$ = "+" THEN
            removeelement t$, l
            l = l - 1
        ELSEIF ele$ = "-" AND nextele$ = "-" THEN
            removeelements t$, l, l + 1, 0
            insertelements t$, l - 1, "+"
            l = l - 1
        ELSEIF ele$ = "-" AND nextele$ = "+" THEN
            removeelement t$, l + 1
            l = l - 1
        ELSEIF ele$ = "+" AND nextele$ = "-" THEN
            removeelement t$, l
            l = l - 1
        END IF
    NEXT
    DWD$ = t$
END FUNCTION

SUB PreParse (e$)
    t$ = e$ 'preserve the original string

    t$ = eleucase$(t$)
    IF t$ = "" THEN e$ = "ERROR - NULL string; nothing to evaluate": EXIT SUB

    'ERROR CHECK by counting our brackets
    count = numelements(t$)
    FOR l = 1 to count
        ele$ = getelement$(t$, l)
        IF ele$ = "(" THEN c = c + 1
        IF ele$ = ")" THEN c = c - 1

        IF c < 0 THEN e$ = "ERROR - Bad Parenthesis, too many )": EXIT SUB
    NEXT
    IF c <> 0 THEN e$ = "ERROR - Bad Parenthesis": EXIT SUB

    'Modify so that NOT will process properly
    FOR l = 1 to numelements(t$)
        'FIXME: This doesn't account for `x ^ NOT y + 2`, where it evaluates as `x ^ (NOT y) + 2`
        IF getelement$(t$, l) = "NOT" THEN
            FOR l2 = l to numelements(t$)
                ele$ = getelement$(t$, l2)
                IF ele$ = "AND" OR ele$ = "OR" OR ele$ = "XOR" OR ele$ = "EQV" OR ele$ = "IMP" OR ele$ = ")" THEN
                    EXIT FOR
                END IF
            NEXT

            insertelements t$, l2 - 1, ")"
            insertelements t$, l - 1, "("
            l = l + 1
        END IF
    NEXT

    e$ = t$
END SUB


' Returns 0 if given element is not the name of a function
' If it is a function, the ConstFuncs() array index of the function is returned
FUNCTION IsFunctionIdentifier&(ele$)
    FOR i = 1 TO UBOUND(ConstFuncs)
        IF ele$ = ConstFuncs(i).nam THEN
            IsFunctionIdentifier& = i
            EXIT FUNCTION
        ELSE
            IF LEFT$(ConstFuncs(i).nam, 1) = "_" AND qb64prefix_set = 1 THEN
                'try without prefix
                IF ele$ = MID$(ConstFuncs(i).nam, 2) THEN
                    IsFunctionIdentifier& = i
                    EXIT FUNCTION
                END IF
            END IF
        END IF
    NEXT
END FUNCTION
