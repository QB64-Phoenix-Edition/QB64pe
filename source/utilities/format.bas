'Apply indentation to layout$ as per settings and remove control characters.
FUNCTION apply_layout_indent$ (original$)
    layout2$ = layout$
    'previous line was OK, so use layout if available
    IF IDEAutoLayout <> 0 OR IDEAutoIndent <> 0 THEN
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

            IF IDEAutoLayout THEN
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
            END IF

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

            IF LEN(layout2$) THEN
                apply_layout_indent$ = indent$ + layout2$
            END IF
        END IF 'len(layout2$)
    END IF 'using layout/indent
END FUNCTION

