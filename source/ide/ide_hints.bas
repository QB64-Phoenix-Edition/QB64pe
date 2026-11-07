'Inline syntax hints ("ghost text") for the IDE editor.
'
'As the user types at the end of a line, the IDE shows dimmed text after the cursor:
' - the next expected argument(s) of the statement or function being typed, taken
'   from the internal keywords' syntax references (id.hr_syntax);
' - the closing parentheses/quote still missing, which TAB inserts.

SUB IdeSyntaxRegister (kind AS LONG, syntax$)
    'Called by regid for each internal sub/function. kind: 1 = function, 2 = sub (statement)
    DIM s$, alt$, head$, key$, entry$
    DIM p AS LONG, i AS LONG

    s$ = syntax$
    DO WHILE LEN(s$)
        p = INSTR(s$, " or ")
        IF p THEN
            alt$ = _TRIM$(LEFT$(s$, p - 1)): s$ = MID$(s$, p + 4)
        ELSE
            alt$ = _TRIM$(s$): s$ = ""
        END IF

        head$ = IdeHintReadWord$(alt$, 1)
        IF LEN(head$) THEN
            key$ = IdeHintKey$(head$)
            entry$ = CHR$(1) + CHR$(kind) + key$ + CHR$(2) + MID$(alt$, LEN(head$) + 1) + CHR$(1)
            IF INSTR(IdeSyntaxIndex, entry$) = 0 THEN
                IdeSyntaxIndex = IdeSyntaxIndex + entry$
                IdeSyntaxCount = IdeSyntaxCount + 1
                IF IdeSyntaxCount > UBOUND(IdeSyntaxKey) THEN
                    i = UBOUND(IdeSyntaxKey) * 2 + 100
                    REDIM _PRESERVE IdeSyntaxKey(1 TO i) AS STRING
                    REDIM _PRESERVE IdeSyntaxKind(1 TO i) AS _BYTE
                    REDIM _PRESERVE IdeSyntaxTemplate(1 TO i) AS STRING
                END IF
                IdeSyntaxKey(IdeSyntaxCount) = key$
                IdeSyntaxKind(IdeSyntaxCount) = kind
                IdeSyntaxTemplate(IdeSyntaxCount) = MID$(alt$, LEN(head$) + 1)
            END IF
        END IF
    LOOP
END SUB

FUNCTION IdeSyntaxFind& (key$, kind AS LONG, after AS LONG)
    'Returns the next syntax entry for key$/kind after index 'after', or 0 if there is none.
    'Entries 1 to IdeUserSyntaxCount are the program's own SUBs/FUNCTIONs, which come first;
    'internal keywords follow (also matched by their $NOPREFIX spelling).
    DIM i AS LONG, start AS LONG
    FOR i = after + 1 TO IdeUserSyntaxCount
        IF IdeUserSyntaxKind(i) = kind AND IdeUserSyntaxKey(i) = key$ THEN IdeSyntaxFind& = i: EXIT FUNCTION
    NEXT
    start = after - IdeUserSyntaxCount + 1: IF start < 1 THEN start = 1
    FOR i = start TO IdeSyntaxCount
        IF IdeSyntaxKind(i) = kind THEN
            IF IdeSyntaxKey(i) = key$ OR IdeSyntaxKey(i) = "_" + key$ THEN IdeSyntaxFind& = IdeUserSyntaxCount + i: EXIT FUNCTION
        END IF
    NEXT
END FUNCTION

FUNCTION IdeSyntaxTemplateOf$ (idx AS LONG)
    IF idx <= IdeUserSyntaxCount THEN
        IdeSyntaxTemplateOf$ = IdeUserSyntaxTemplate(idx)
    ELSE
        IdeSyntaxTemplateOf$ = IdeSyntaxTemplate(idx - IdeUserSyntaxCount)
    END IF
END FUNCTION

SUB IdeUserSyntaxBegin
    IdeUserStageCount = 0
END SUB

SUB IdeUserSyntaxCommit
    DIM i AS LONG
    IF IdeUserStageCount > UBOUND(IdeUserSyntaxKey) THEN
        REDIM IdeUserSyntaxKey(1 TO IdeUserStageCount) AS STRING
        REDIM IdeUserSyntaxKind(1 TO IdeUserStageCount) AS _BYTE
        REDIM IdeUserSyntaxTemplate(1 TO IdeUserStageCount) AS STRING
    END IF
    FOR i = 1 TO IdeUserStageCount
        IdeUserSyntaxKey(i) = IdeUserStageKey(i)
        IdeUserSyntaxKind(i) = IdeUserStageKind(i)
        IdeUserSyntaxTemplate(i) = IdeUserStageTemplate(i)
    NEXT
    IdeUserSyntaxCount = IdeUserStageCount
END SUB

SUB IdeUserSyntaxRegister (kind AS LONG, declaration$)
    'Builds a syntax template from a SUB/FUNCTION declaration line (also those inside
    'DECLARE LIBRARY blocks). kind: 1 = function, 2 = sub. Each parameter becomes one
    'token, delimited by CHR$(1), shown as declared (e.g. "x AS LONG", "a$", "arr()").
    DIM d$, w$, u$, nam$, params$, p$, tmpl$, key$
    DIM i AS LONG, j AS LONG, c AS LONG, level AS LONG, inq AS LONG, first AS LONG

    'the compiler's formatted line has its elements separated by CHR$(13)
    d$ = declaration$
    FOR i = 1 TO LEN(d$)
        IF ASC(d$, i) = 13 THEN MID$(d$, i, 1) = " "
    NEXT

    'find SUB/FUNCTION and the name after it
    i = 1
    DO
        DO WHILE i <= LEN(d$)
            IF IdeHintIsWordChar(ASC(d$, i)) THEN EXIT DO
            i = i + 1
        LOOP
        IF i > LEN(d$) THEN EXIT SUB
        w$ = IdeHintReadWord$(d$, i): i = i + LEN(w$)
        u$ = UCASE$(w$)
        IF (u$ = "SUB" AND kind = 2) OR (u$ = "FUNCTION" AND kind = 1) THEN EXIT DO
    LOOP
    DO WHILE i <= LEN(d$)
        IF ASC(d$, i) <> 32 THEN EXIT DO
        i = i + 1
    LOOP
    nam$ = IdeHintReadWord$(d$, i): i = i + LEN(nam$)
    IF LEN(nam$) = 0 THEN EXIT SUB

    'skip ALIAS name (DECLARE LIBRARY)
    DO WHILE i <= LEN(d$)
        IF ASC(d$, i) <> 32 THEN EXIT DO
        i = i + 1
    LOOP
    IF UCASE$(IdeHintReadWord$(d$, i)) = "ALIAS" THEN
        i = i + 5
        DO WHILE i <= LEN(d$)
            c = ASC(d$, i)
            IF c = 40 OR c = 39 THEN EXIT DO '( or comment
            IF c = 34 THEN inq = NOT inq
            i = i + 1
        LOOP
        inq = 0
    END IF
    DO WHILE i <= LEN(d$)
        IF ASC(d$, i) <> 32 THEN EXIT DO
        i = i + 1
    LOOP

    'parameter list
    IF i <= LEN(d$) THEN
        IF ASC(d$, i) = 40 THEN
            level = 0
            FOR j = i TO LEN(d$)
                c = ASC(d$, j)
                IF c = 34 THEN inq = NOT inq
                IF inq = 0 THEN
                    IF c = 40 THEN level = level + 1
                    IF c = 41 THEN
                        level = level - 1
                        IF level = 0 THEN EXIT FOR
                    END IF
                END IF
            NEXT
            params$ = MID$(d$, i + 1, j - i - 1)
        END IF
    END IF

    first = -1
    level = 0
    params$ = params$ + ","
    FOR j = 1 TO LEN(params$)
        c = ASC(params$, j)
        IF c = 40 THEN level = level + 1
        IF c = 41 THEN level = level - 1
        IF c = 44 AND level = 0 THEN
            p$ = _TRIM$(p$)
            IF UCASE$(LEFT$(p$, 6)) = "BYVAL " THEN p$ = LTRIM$(MID$(p$, 7))
            DO WHILE INSTR(p$, "  "): p$ = StrReplace$(p$, "  ", " "): LOOP
            p$ = StrReplace$(p$, " (", "("): p$ = StrReplace$(p$, "( ", "("): p$ = StrReplace$(p$, " )", ")")
            IF LEN(p$) THEN
                IF first = 0 THEN tmpl$ = tmpl$ + ", "
                tmpl$ = tmpl$ + CHR$(1) + p$ + CHR$(1)
                first = 0
            END IF
            p$ = ""
        ELSE
            p$ = p$ + CHR$(c)
        END IF
    NEXT
    IF kind = 1 AND LEN(tmpl$) > 0 THEN tmpl$ = "(" + tmpl$ + ")"

    key$ = IdeHintKey$(nam$)
    FOR j = 1 TO IdeUserStageCount 'first declaration wins
        IF IdeUserStageKind(j) = kind AND IdeUserStageKey(j) = key$ THEN EXIT SUB
    NEXT
    IdeUserStageCount = IdeUserStageCount + 1
    IF IdeUserStageCount > UBOUND(IdeUserStageKey) THEN
        j = UBOUND(IdeUserStageKey) * 2 + 50
        REDIM _PRESERVE IdeUserStageKey(1 TO j) AS STRING
        REDIM _PRESERVE IdeUserStageKind(1 TO j) AS _BYTE
        REDIM _PRESERVE IdeUserStageTemplate(1 TO j) AS STRING
    END IF
    IdeUserStageKey(IdeUserStageCount) = key$
    IdeUserStageKind(IdeUserStageCount) = kind
    IdeUserStageTemplate(IdeUserStageCount) = tmpl$
END SUB

FUNCTION IdeHintIsWordChar%% (c AS LONG)
    SELECT CASE c
        CASE 48 TO 57, 65 TO 90, 97 TO 122, 95, 46 '0-9 A-Z a-z _ .
            IdeHintIsWordChar%% = -1
    END SELECT
END FUNCTION

FUNCTION IdeHintIsSuffixChar%% (c AS LONG)
    SELECT CASE c
        CASE 36, 37, 38, 33, 35, 126, 96 '$ % & ! # ~ `
            IdeHintIsSuffixChar%% = -1
    END SELECT
END FUNCTION

FUNCTION IdeHintReadWord$ (a$, start AS LONG)
    'Reads an identifier/number (plus any type suffix) starting at 'start'
    DIM i AS LONG
    i = start
    IF i <= LEN(a$) THEN IF ASC(a$, i) = 38 THEN i = i + 1 '&H, &O, &B literals
    DO WHILE i <= LEN(a$)
        IF IdeHintIsWordChar(ASC(a$, i)) = 0 THEN EXIT DO
        i = i + 1
    LOOP
    IF i = start THEN EXIT FUNCTION
    DO WHILE i <= LEN(a$)
        IF IdeHintIsSuffixChar(ASC(a$, i)) = 0 THEN EXIT DO
        i = i + 1
    LOOP
    IdeHintReadWord$ = MID$(a$, start, i - start)
END FUNCTION

FUNCTION IdeHintKey$ (word$)
    'Upper case, without numeric type suffixes (a trailing $ is kept)
    DIM k$
    k$ = UCASE$(word$)
    DO WHILE LEN(k$)
        IF IdeHintIsSuffixChar(ASC(k$, LEN(k$))) = 0 OR RIGHT$(k$, 1) = "$" THEN EXIT DO
        k$ = LEFT$(k$, LEN(k$) - 1)
    LOOP
    IdeHintKey$ = k$
END FUNCTION

SUB IdeInlineHint (lineText$, hint$, closers$)
    'lineText$ is the current line up to the cursor (the cursor must be at its end).
    'Returns the argument hint for the statement/function being typed and the closing
    'characters still missing. Both are empty when there's nothing to suggest.
    DIM t$, stack$, w$, u$, typed$, key$
    DIM i AS LONG, c AS LONG, depth AS LONG, inq AS LONG, stmtStart AS LONG, lead AS LONG
    DIM k AS LONG, p AS LONG, ws AS LONG, kind AS LONG, idx AS LONG, ok AS LONG, found AS LONG, keep AS LONG, keepAll AS LONG, callForm AS LONG, tmpl$

    hint$ = "": closers$ = ""
    t$ = lineText$
    IF LEFT$(LTRIM$(t$), 1) = "$" THEN EXIT SUB 'metacommands

    'Pass 1: track quotes, open brackets and where the current statement begins
    stmtStart = 1
    i = 1
    DO WHILE i <= LEN(t$)
        c = ASC(t$, i)
        IF inq THEN
            IF c = 34 THEN inq = 0
        ELSE
            SELECT CASE c
                CASE 34: inq = -1
                CASE 39: EXIT SUB 'comment
                CASE 40: depth = depth + 1: stack$ = stack$ + MKL$(i)
                CASE 41: IF depth THEN depth = depth - 1: stack$ = LEFT$(stack$, LEN(stack$) - 4)
                CASE 58: IF depth = 0 THEN stmtStart = i + 1
                CASE 65 TO 90, 97 TO 122, 95
                    IF i = 1 _ORELSE IdeHintIsWordChar(ASC(t$, i - 1)) = 0 THEN
                        w$ = IdeHintReadWord$(t$, i)
                        u$ = UCASE$(w$)
                        IF u$ = "REM" AND LTRIM$(MID$(t$, stmtStart, i - stmtStart)) = "" THEN EXIT SUB
                        IF u$ = "DATA" AND LTRIM$(MID$(t$, stmtStart, i - stmtStart)) = "" THEN EXIT SUB
                        IF depth = 0 AND (u$ = "THEN" OR u$ = "ELSE") THEN stmtStart = i + LEN(w$)
                        i = i + LEN(w$)
                        _CONTINUE
                    END IF
            END SELECT
        END IF
        i = i + 1
    LOOP

    IF inq THEN closers$ = CHR$(34)
    closers$ = closers$ + STRING$(depth, ")")

    lead = stmtStart
    DO WHILE lead <= LEN(t$)
        IF ASC(t$, lead) <> 32 AND ASC(t$, lead) <> 9 THEN EXIT DO
        lead = lead + 1
    LOOP

    'Pass 2: find what's being typed - the innermost open call to a known function,
    'or else the statement keyword at the start of the current statement
    FOR k = depth TO 1 STEP -1
        p = CVL(MID$(stack$, (k - 1) * 4 + 1, 4))
        ws = p
        DO WHILE ws > 1
            c = ASC(t$, ws - 1)
            IF IdeHintIsWordChar(c) = 0 AND IdeHintIsSuffixChar(c) = 0 THEN EXIT DO
            ws = ws - 1
        LOOP
        IF ws < p THEN
            key$ = IdeHintKey$(MID$(t$, ws, p - ws))
            IF ws = lead AND IdeSyntaxFind(key$, 2, 0) > 0 THEN EXIT FOR 'statement, e.g. SCREEN(...)
            IF IdeSyntaxFind(key$, 1, 0) > 0 THEN
                kind = 1
                typed$ = MID$(t$, p)
                EXIT FOR
            END IF
        END IF
    NEXT

    IF kind = 0 THEN
        w$ = IdeHintReadWord$(t$, lead)
        IF LEN(w$) = 0 THEN EXIT SUB
        typed$ = MID$(t$, lead + LEN(w$))
        IF LEN(typed$) = 0 THEN EXIT SUB 'wait for a space (the keyword may still be growing)
        IF UCASE$(w$) = "CALL" THEN 'CALL name(arguments)
            i = lead + LEN(w$)
            DO WHILE i <= LEN(t$)
                IF ASC(t$, i) <> 32 THEN EXIT DO
                i = i + 1
            LOOP
            w$ = IdeHintReadWord$(t$, i)
            IF LEN(w$) = 0 THEN EXIT SUB
            typed$ = MID$(t$, i + LEN(w$))
            IF LEN(typed$) = 0 THEN EXIT SUB
            callForm = -1
        END IF
        key$ = IdeHintKey$(w$)
        kind = 2
    END IF

    'The hint comes from the first syntax form that fits what was typed; closing is
    'suggested if any fitting form allows it (e.g. _RGB32(intensity) vs. _RGB32(r, g, b))
    keepAll = -2
    idx = IdeSyntaxFind(key$, kind, 0)
    DO WHILE idx
        'Try reading leading optional arguments as left out first; if what was typed
        'doesn't fit that way, read them as present
        tmpl$ = IdeSyntaxTemplateOf$(idx)
        IdeHintShowAll = (idx <= IdeUserSyntaxCount) 'user SUBs: list all remaining parameters
        IF callForm AND LEN(tmpl$) > 0 THEN tmpl$ = "(" + tmpl$ + ")"
        IdeHintPreferSkip = -1
        u$ = IdeHintMatch$(tmpl$, typed$, ok, keep)
        IF ok = 0 THEN
            IdeHintPreferSkip = 0
            u$ = IdeHintMatch$(tmpl$, typed$, ok, keep)
        END IF
        IF ok THEN
            IF found = 0 THEN w$ = u$: found = -1
            IF keep < 0 THEN keepAll = -1 ELSE IF keep > keepAll THEN keepAll = keep
        END IF
        idx = IdeSyntaxFind(key$, kind, idx)
    LOOP
    IF found = 0 THEN EXIT SUB
    IF keepAll >= 0 THEN closers$ = LEFT$(closers$, keepAll) 'required arguments still missing: don't suggest closing yet
    IF LEN(w$) = 0 THEN EXIT SUB

    IF RIGHT$(typed$, 1) = "," THEN
        w$ = " " + w$
    ELSEIF LEN(typed$) > 0 AND IdeHintIsWordChar(ASC(w$)) THEN
        c = ASC(typed$, LEN(typed$))
        IF IdeHintIsWordChar(c) OR IdeHintIsSuffixChar(c) THEN w$ = " " + w$ 'e.g. "_MAPUNICODE u TO ..."
    END IF
    hint$ = w$
END SUB

SUB IdeHintTokenize (template$)
    'Splits a syntax reference into tokens: literal punctuation, KEYWORDS, parameter
    'names, {CHOICE|LISTS} and [optional sections] (with each [ linked to its ])
    DIM i AS LONG, j AS LONG, c AS LONG, n AS LONG, opens$
    DIM w$

    n = 0
    i = 1
    DO WHILE i <= LEN(template$)
        IF n + 2 > UBOUND(IdeHintTokType) THEN
            REDIM _PRESERVE IdeHintTokType(0 TO n + 100) AS _BYTE
            REDIM _PRESERVE IdeHintTokText(0 TO n + 100) AS STRING
            REDIM _PRESERVE IdeHintTokMatch(0 TO n + 100) AS LONG
        END IF
        c = ASC(template$, i)
        SELECT CASE c
            CASE 32, 9
                i = i + 1
                _CONTINUE
            CASE 91 '[
                IdeHintTokType(n) = HINT_OPEN: IdeHintTokText(n) = "["
                opens$ = opens$ + MKL$(n)
                n = n + 1: i = i + 1
            CASE 93 ']'
                IF LEN(opens$) THEN 'stray ] are ignored
                    IdeHintTokType(n) = HINT_CLOSE: IdeHintTokText(n) = "]"
                    IdeHintTokMatch(CVL(RIGHT$(opens$, 4))) = n
                    opens$ = LEFT$(opens$, LEN(opens$) - 4)
                    n = n + 1
                END IF
                i = i + 1
            CASE 123 '{
                j = i + 1
                DO WHILE j <= LEN(template$)
                    IF ASC(template$, j) = 125 OR ASC(template$, j) = 93 THEN EXIT DO
                    j = j + 1
                LOOP
                IdeHintTokType(n) = HINT_CHOICE: IdeHintTokText(n) = MID$(template$, i + 1, j - i - 1)
                n = n + 1
                i = j: IF i <= LEN(template$) THEN IF ASC(template$, i) = 125 THEN i = i + 1
            CASE 1 'a user SUB/FUNCTION parameter, shown as declared
                j = INSTR(i + 1, template$, CHR$(1))
                IF j = 0 THEN j = LEN(template$) + 1
                IdeHintTokType(n) = HINT_PARAM: IdeHintTokText(n) = MID$(template$, i, j - i + 1)
                n = n + 1: i = j + 1
            CASE 34 'quoted text is shown as is
                j = INSTR(i + 1, template$, CHR$(34))
                IF j = 0 THEN j = LEN(template$)
                IdeHintTokType(n) = HINT_PARAM: IdeHintTokText(n) = MID$(template$, i, j - i + 1)
                n = n + 1: i = j + 1
            CASE ELSE
                w$ = ""
                IF IdeHintIsWordChar(c) THEN w$ = IdeHintReadWord$(template$, i)
                IF LEN(w$) THEN
                    IF w$ = UCASE$(w$) THEN IdeHintTokType(n) = HINT_KW ELSE IdeHintTokType(n) = HINT_PARAM
                    IdeHintTokText(n) = w$
                    i = i + LEN(w$)
                ELSE
                    IdeHintTokType(n) = HINT_LIT: IdeHintTokText(n) = CHR$(c)
                    i = i + 1
                END IF
                n = n + 1
        END SELECT
    LOOP
    DO WHILE LEN(opens$) 'unclosed [ run to the end
        IdeHintTokMatch(CVL(RIGHT$(opens$, 4))) = n
        opens$ = LEFT$(opens$, LEN(opens$) - 4)
    LOOP
    IdeHintTokCount = n
END SUB

FUNCTION IdeHintChoiceHas%% (choice$, word$)
    DIM a$, i AS LONG, c AS LONG
    a$ = UCASE$(choice$) + "|"
    FOR i = 1 TO LEN(a$)
        c = ASC(a$, i)
        IF c = 124 OR c = 44 OR c = 32 THEN 'separators: | , space
            IF LEN(w$) THEN
                IF w$ = word$ OR w$ = "_" + word$ THEN IdeHintChoiceHas%% = -1: EXIT FUNCTION
            END IF
            w$ = ""
        ELSE
            w$ = w$ + CHR$(c)
        END IF
    NEXT
END FUNCTION

FUNCTION IdeHintSeek& (startTok AS LONG, endTok AS LONG, mode AS LONG, value$, allowSkip AS LONG)
    'Looks for the next token that accepts what was typed, starting at startTok.
    'Optional sections may be entered or skipped as a whole; required tokens can't be
    'skipped (except one parameter when allowSkip is set, for empty arguments).
    'Returns the index of the token after the match, or -1.
    DIM i AS LONG, r AS LONG, skip AS LONG, t$

    skip = allowSkip
    i = startTok
    DO WHILE i < endTok
        SELECT CASE IdeHintTokType(i)
            CASE HINT_OPEN
                IF IdeHintPreferSkip AND mode = HINT_MODE_PARAM AND IdeHintTokMatch(i) = i + 3 THEN
                    IF IdeHintTokType(i + 1) = HINT_PARAM AND IdeHintTokText(i + 2) = "," THEN
                        'An optional leading argument, as in INSTR([start%,] baseString$, ...):
                        'first assume it was left out
                        r = IdeHintSeek(IdeHintTokMatch(i) + 1, endTok, mode, value$, skip)
                        IF r >= 0 THEN IdeHintSeek& = r: EXIT FUNCTION
                    END IF
                END IF
                r = IdeHintSeek(i + 1, IdeHintTokMatch(i), mode, value$, skip)
                IF r >= 0 THEN IdeHintSeek& = r: EXIT FUNCTION
                i = IdeHintTokMatch(i) + 1
            CASE HINT_CLOSE
                i = i + 1
            CASE HINT_LIT
                IF mode = HINT_MODE_LIT AND IdeHintTokText(i) = value$ THEN IdeHintSeek& = i + 1: EXIT FUNCTION
                EXIT DO
            CASE HINT_KW
                t$ = UCASE$(IdeHintTokText(i))
                IF mode = HINT_MODE_WORD AND (t$ = value$ OR t$ = "_" + value$) THEN IdeHintSeek& = i + 1: EXIT FUNCTION
                EXIT DO
            CASE HINT_CHOICE
                IF mode = HINT_MODE_WORD THEN
                    IF IdeHintChoiceHas(IdeHintTokText(i), value$) THEN IdeHintSeek& = i + 1: EXIT FUNCTION
                ELSEIF mode = HINT_MODE_PARAM THEN
                    IF IdeHintChoiceHasParam(IdeHintTokText(i)) THEN IdeHintSeek& = i + 1: EXIT FUNCTION
                END IF
                EXIT DO
            CASE HINT_PARAM
                IF mode = HINT_MODE_PARAM THEN IdeHintSeek& = i + 1: EXIT FUNCTION
                IF skip = 0 THEN EXIT DO
                skip = 0: i = i + 1
        END SELECT
    LOOP
    IdeHintSeek& = -1
END FUNCTION

FUNCTION IdeHintMatch$ (template$, typed$, ok AS LONG, keepClosers AS LONG)
    'Walks what was typed after the keyword through the syntax reference.
    'ok is set if the typed text fits; the result is then the hint for what comes next.
    'keepClosers is -1, or how many of the suggested closers to keep when the current
    'group can't be closed yet (required arguments are still missing).
    DIM tpos AS LONG, inParam AS LONG, exprDepth AS LONG, inString AS LONG, tLevel AS LONG
    DIM i AS LONG, r AS LONG, c$, w$

    ok = 0: keepClosers = -1
    IdeHintTokenize template$

    i = 1
    DO WHILE i <= LEN(typed$)
        c$ = MID$(typed$, i, 1)
        IF inString THEN
            IF c$ = CHR$(34) THEN inString = 0
            i = i + 1: _CONTINUE
        END IF
        IF exprDepth THEN
            IF c$ = "(" THEN
                exprDepth = exprDepth + 1
            ELSEIF c$ = ")" THEN
                exprDepth = exprDepth - 1
            ELSEIF c$ = CHR$(34) THEN
                inString = -1
            END IF
            i = i + 1: _CONTINUE
        END IF
        IF c$ = " " OR c$ = CHR$(9) THEN i = i + 1: _CONTINUE

        IF c$ = CHR$(34) THEN
            IF inParam = 0 THEN
                r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_PARAM, "", 0)
                IF r < 0 THEN EXIT FUNCTION
                tpos = r: inParam = -1
            END IF
            inString = -1
            i = i + 1: _CONTINUE
        END IF

        w$ = ""
        IF IdeHintIsWordChar(ASC(c$)) OR c$ = "&" THEN w$ = IdeHintReadWord$(typed$, i)
        IF LEN(w$) THEN
            i = i + LEN(w$)
            r = -1
            IF INSTR("0123456789.&", LEFT$(w$, 1)) = 0 THEN r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_WORD, UCASE$(w$), 0)
            IF r >= 0 THEN
                tpos = r: inParam = 0
            ELSEIF inParam = 0 THEN
                r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_PARAM, "", 0)
                IF r < 0 THEN EXIT FUNCTION
                tpos = r: inParam = -1
            END IF
            _CONTINUE
        END IF

        SELECT CASE c$
            CASE "("
                IF inParam THEN
                    r = -1
                    IF tpos < IdeHintTokCount THEN
                        IF IdeHintTokType(tpos) = HINT_LIT AND IdeHintTokText(tpos) = "(" THEN r = tpos + 1
                    END IF
                    IF r >= 0 THEN tpos = r: tLevel = tLevel + 1: inParam = 0 ELSE exprDepth = 1 'array(...) vs. expression
                ELSE
                    r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_LIT, "(", 0)
                    IF r >= 0 THEN
                        tpos = r: tLevel = tLevel + 1
                    ELSE
                        r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_PARAM, "", 0)
                        IF r < 0 THEN EXIT FUNCTION
                        tpos = r: inParam = -1: exprDepth = 1
                    END IF
                END IF
            CASE ")", ","
                r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_LIT, c$, NOT inParam)
                IF r < 0 THEN EXIT FUNCTION
                tpos = r: inParam = 0
                IF c$ = ")" THEN tLevel = tLevel - 1
            CASE ELSE 'operators and other punctuation
                IF inParam = 0 THEN
                    r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_LIT, c$, 0)
                    IF r >= 0 THEN
                        tpos = r
                    ELSE
                        r = IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_PARAM, "", 0)
                        IF r < 0 THEN EXIT FUNCTION
                        tpos = r: inParam = -1
                    END IF
                END IF
        END SELECT
        i = i + 1
    LOOP

    ok = -1
    IF tLevel > 0 THEN
        IF IdeHintSeek(tpos, IdeHintTokCount, HINT_MODE_LIT, ")", 0) < 0 THEN keepClosers = exprDepth - (inString <> 0)
    END IF
    IF exprDepth OR inString THEN EXIT FUNCTION 'inside an expression's brackets or a string literal
    IF inParam THEN 'while an argument is being typed, only hint what's required next
        i = tpos
        DO WHILE i < IdeHintTokCount
            IF IdeHintTokType(i) <> HINT_CLOSE THEN EXIT DO
            i = i + 1
        LOOP
        IF i >= IdeHintTokCount THEN EXIT FUNCTION
        IF IdeHintTokType(i) = HINT_OPEN THEN EXIT FUNCTION
    END IF
    IdeHintMatch$ = IdeHintDisplay$(tpos, tLevel)
END FUNCTION

FUNCTION IdeHintDisplay$ (startTok AS LONG, tLevel AS LONG)
    'Builds the hint text from startTok: up to the next comma when typing the statement's
    'top-level arguments, or up to the end of the group when inside parentheses.
    DIM o$, piece$, opens$, content$
    DIM i AS LONG, j AS LONG, level AS LONG, emittedWord AS LONG
    DIM lastType AS LONG, afterComma AS LONG

    i = startTok

    'A leading optional STEP isn't shown (e.g. CIRCLE [STEP] (x, y) hints "(x, y)")
    IF i + 2 < IdeHintTokCount THEN
        IF IdeHintTokType(i) = HINT_OPEN AND IdeHintTokMatch(i) = i + 2 AND UCASE$(IdeHintTokText(i + 1)) = "STEP" THEN i = i + 3
    END IF

    DO WHILE i < IdeHintTokCount
        piece$ = IdeHintTokText(i)
        SELECT CASE IdeHintTokType(i)
            CASE HINT_OPEN
                IF level = 0 AND tLevel = 0 AND emittedWord AND IdeHintShowAll = 0 THEN
                    'statement arguments: show what's required, up to the next optional part
                    '(optional keyword flags such as [STEP] don't count)
                    FOR j = i + 1 TO IdeHintTokMatch(i) - 1
                        IF IdeHintTokType(j) <> HINT_KW AND IdeHintTokType(j) <> HINT_CHOICE THEN EXIT DO
                    NEXT
                END IF
                opens$ = opens$ + MKL$(LEN(o$)): content$ = content$ + "0"
                IF afterComma THEN
                    o$ = o$ + " ": afterComma = 0
                ELSEIF RIGHT$(o$, 1) = "]" THEN
                    o$ = o$ + " " 'e.g. "[_SEAMLESS] [..."
                ELSEIF LEN(o$) > 0 AND i + 1 < IdeHintTokCount THEN
                    IF IdeHintIsWordChar(ASC(o$, LEN(o$))) AND (IdeHintTokType(i + 1) = HINT_KW OR IdeHintTokType(i + 1) = HINT_CHOICE) THEN o$ = o$ + " " 'e.g. "fileNumber [LEN = ...]"
                END IF
                o$ = o$ + "["
            CASE HINT_CLOSE
                IF LEN(opens$) THEN 'otherwise we're leaving a section the cursor was already in
                    IF RIGHT$(content$, 1) = "1" THEN
                        o$ = o$ + "]"
                    ELSE
                        o$ = LEFT$(o$, CVL(RIGHT$(opens$, 4)))
                    END IF
                    opens$ = LEFT$(opens$, LEN(opens$) - 4): content$ = LEFT$(content$, LEN(content$) - 1)
                END IF
            CASE HINT_LIT
                IF piece$ = ")" THEN
                    level = level - 1
                    IF level < 0 THEN EXIT DO
                END IF
                IF afterComma AND piece$ <> ")" AND piece$ <> "," THEN o$ = o$ + " "
                afterComma = 0
                IF piece$ = "(" THEN
                    level = level + 1
                    IF lastType = HINT_KW OR RIGHT$(o$, 1) = "]" THEN o$ = o$ + " "
                END IF
                IF piece$ = "=" THEN piece$ = " = "
                o$ = o$ + piece$
                IF piece$ = "," THEN afterComma = -1
            CASE ELSE 'keywords, choices and parameters
                IF IdeHintTokType(i) = HINT_CHOICE THEN
                    IF piece$ <> UCASE$(piece$) THEN 'drop the type suffixes of parameter names
                        FOR j = LEN(piece$) TO 1 STEP -1
                            IF INSTR("%&!#~`", MID$(piece$, j, 1)) THEN piece$ = LEFT$(piece$, j - 1) + MID$(piece$, j + 1)
                        NEXT
                    END IF
                    piece$ = "{" + piece$ + "}"
                ELSEIF IdeHintTokType(i) = HINT_PARAM AND LEFT$(piece$, 1) = CHR$(1) THEN
                    piece$ = MID$(piece$, 2, LEN(piece$) - 2)
                ELSEIF IdeHintTokType(i) = HINT_PARAM AND LEFT$(piece$, 1) <> CHR$(34) THEN
                    piece$ = IdeHintKey$(piece$): piece$ = LEFT$(IdeHintTokText(i), LEN(piece$)) 'drop the type suffix
                END IF
                IF afterComma THEN
                    o$ = o$ + " "
                ELSEIF LEN(o$) THEN
                    IF IdeHintIsWordChar(ASC(o$, LEN(o$))) OR RIGHT$(o$, 1) = ")" OR RIGHT$(o$, 1) = "}" OR RIGHT$(o$, 1) = "$" THEN o$ = o$ + " "
                END IF
                afterComma = 0
                o$ = o$ + piece$
                emittedWord = -1
                content$ = STRING$(LEN(content$), "1")
        END SELECT
        lastType = IdeHintTokType(i)
        i = i + 1
    LOOP

    DO WHILE LEN(opens$)
        IF RIGHT$(content$, 1) = "1" THEN o$ = o$ + "]" ELSE o$ = LEFT$(o$, CVL(RIGHT$(opens$, 4)))
        opens$ = LEFT$(opens$, LEN(opens$) - 4): content$ = LEFT$(content$, LEN(content$) - 1)
    LOOP
    IdeHintDisplay$ = RTRIM$(o$)
END FUNCTION

FUNCTION IdeHintChoiceHasParam%% (choice$)
    'True if any alternative of a {choice} is a parameter name (starts in lower case),
    'as in {mode%|imagehandle&} - unlike keywords such as {Wait|Resume}
    DIM i AS LONG
    FOR i = 1 TO LEN(choice$)
        IF i = 1 _ORELSE INSTR("|, ", MID$(choice$, i - 1, 1)) > 0 THEN
            IF ASC(choice$, i) >= 97 AND ASC(choice$, i) <= 122 THEN IdeHintChoiceHasParam%% = -1: EXIT FUNCTION
        END IF
    NEXT
END FUNCTION
