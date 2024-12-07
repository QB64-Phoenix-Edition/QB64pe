
SUB EmitLoggingStatement (elements AS STRING, loglevel AS STRING)
    SELECT CASE loglevel
        CASE "TRACE": func$ = "sub__logtrace": scaseLayout$ = "_LogTrace"
        CASE "INFO": func$ = "sub__loginfo": scaseLayout$ = "_LogInfo"
        CASE "WARN": func$ = "sub__logwarn": scaseLayout$ = "_LogWarn"
        CASE "ERROR": func$ = "sub__logerror": scaseLayout$ = "_LogError"
    END SELECT

    e$ = fixoperationorder(elements)
    IF Error_Happened THEN EXIT SUB

    l$ = SCase$(scaseLayout$) + sp + tlayout$
    layoutdone = 1: pushelement layout$, l$

    e$ = evaluatetotyp(e$, ISSTRING)
    IF Error_Happened THEN EXIT SUB

    subfuncname$ = subfunc$

    IF subfuncname$ = "" THEN subfuncname$ = "Main QB64 Code"

    IF inclevel = 0 THEN
        IF NoIDEMode THEN
            WriteBufLine MainTxtBuf, func$ + "(" + AddQuotes$(escapeString$(sourcefile$)) + ", " + AddQuotes$(escapeString$(subfuncname$)) + ", " + _TRIM$(STR$(linenumber)) + ", " + e$ + ");"
        ELSE
            WriteBufLine MainTxtBuf, func$ + "(" + AddQuotes$(escapeString$(ideprogname$)) + ", " + AddQuotes$(escapeString$(subfuncname$)) + ", " + _TRIM$(STR$(linenumber)) + ", " + e$ + ");"
        END IF
    ELSE
        WriteBufLine MainTxtBuf, func$ + "(" + AddQuotes$(escapeString$(incname$(inclevel))) + ", " + AddQuotes$(escapeString$(subfuncname$)) + ", " + _TRIM$(STR$(inclinenumber(inclevel))) + ", " + e$ + ");"
    END IF

END SUB

