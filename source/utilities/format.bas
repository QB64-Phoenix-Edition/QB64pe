'Apply indentation to layout$ as per settings and remove control characters.
FUNCTION apply_layout_indent$ (original$)
    layout2$ = layout$
    'previous line was OK, so use layout if available

    IF LEN(layout2$) THEN

        'calculate recommended indent level
        l = LEN(layout2$)
        FOR i = 1 TO l
            IF ASC(layout2$, i) <> 32 OR i = l THEN
                IF ASC(layout2$, i) = 32 THEN
                    layout2$ = "": indent = i
                ELSE
                    indent = i - 1
                    layout2$ = RIGHT$(layout2$, LEN(layout2$) - i + 1)
                END IF
                EXIT FOR
            END IF
        NEXT

        layout3$ = layout2$: i2 = 1
        ignoresp = 0
        FOR i = 1 TO LEN(layout2$)
            a = ASC(layout2$, i)
            IF a = 34 THEN
                ignoresp = ignoresp + 1: IF ignoresp = 2 THEN ignoresp = 0
            END IF
            IF ignoresp = 0 THEN
                IF a = sp_asc THEN ASC(layout3$, i2) = 32: i2 = i2 + 1: GOTO skipchar
                IF a = sp2_asc THEN GOTO skipchar
            END IF
            ASC(layout3$, i2) = a: i2 = i2 + 1
            skipchar:
        NEXT
        layout2$ = LEFT$(layout3$, i2 - 1)

        IF IDEAutoIndent = 0 THEN
            'note: can assume auto-format
            'calculate old indent (if any)
            indent = 0
            l = LEN(original$)
            FOR i = 1 TO l
                IF ASC(original$, i) <> 32 OR i = l THEN
                    indent = i - 1
                    EXIT FOR
                END IF
            NEXT
            indent$ = SPACE$(indent)
        ELSE
            indent$ = SPACE$(indent * IDEAutoIndentSize)
        END IF

        olay$ = layout2$ 'save layouted line
        IF IDEAutoLayout = 0 THEN
            'note: can assume auto-indent
            l = LEN(original$)
            layout2$ = ""
            FOR i = 1 TO l
                IF ASC(original$, i) <> 32 OR i = l THEN
                    layout2$ = RIGHT$(original$, l - i + 1)
                    EXIT FOR
                END IF
            NEXT
        END IF

        IF layout2$ <> olay$ THEN
            lcnt = 0: ocnt = 0
            WHILE lcnt <= LEN(layout2$)
                lcnt = lcnt + 1: ocnt = ocnt + 1
                recheckdiff:
                IF lch$ <> "" AND lch$ <> " " THEN llch$ = lch$ 'save last non-space
                IF och$ <> "" AND och$ <> " " THEN loch$ = och$
                lch$ = MID$(layout2$, lcnt, 1) 'get chars
                och$ = MID$(olay$, ocnt, 1)
                IF lch$ = och$ THEN _CONTINUE 'no diff
                IF lch$ = " " THEN 'skip spacing diff
                    lcnt = lcnt + 1: IF lcnt > LEN(layout2$) AND ocnt > LEN(olay$) THEN EXIT WHILE
                    GOTO recheckdiff
                END IF
                IF och$ = " " THEN 'skip spacing diff
                    ocnt = ocnt + 1: IF ocnt > LEN(olay$) AND lcnt > LEN(layout2$) THEN EXIT WHILE
                    GOTO recheckdiff
                END IF
                IF lch$ = "?" AND UCASE$(MID$(olay$, ocnt, 5)) = "PRINT" THEN '? = PRINT special case
                    ps$ = "print": nlch$ = MID$(layout2$, lcnt + 1, 1)
                    IF nlch$ <> " " AND nlch$ <> "" THEN ps$ = ps$ + " "
                    layout2$ = LEFT$(layout2$, lcnt - 1) + ps$ + RIGHT$(layout2$, LEN(layout2$) - lcnt)
                    GOTO recheckdiff
                END IF
                IF och$ = CHR$(34) AND llch$ = loch$ THEN 'auto-add string closing quote special case
                    layout2$ = LEFT$(layout2$, lcnt - 1) + CHR$(34) + RIGHT$(layout2$, LEN(layout2$) - lcnt)
                    GOTO recheckdiff
                END IF
                las% = _IIF(LEN(lch$), ASC(lch$), 0): oas% = _IIF(LEN(och$), ASC(och$), 0)
                IF isalpha(las%) AND isalpha(oas%) AND ABS(las% - oas%) = 32 THEN MID$(layout2$, lcnt, 1) = och$ 'KW case diff
            WEND
        END IF

        IF LEN(layout2$) THEN
            apply_layout_indent$ = indent$ + layout2$
        END IF
    END IF 'len(layout2$)

END FUNCTION

