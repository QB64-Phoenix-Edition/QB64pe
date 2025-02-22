SUB ExportCodeAs (docFormat$)
    ' Get the current source code, convert it to the desired document format and
    ' then write the result into a file (program name or "Untitled" + extension).
    ' Forum and Wiki exports are pushed directly to the Clipboard and can directly
    ' be pasted into the Forum post or Wiki page. The exported code is highlighted
    ' according to the internal keyword lists and the keywords are linked to its
    ' respective Wiki pages. Documents will use the current IDE colors, the Forum
    ' and Wiki exports use its own fixed blue theme for highlighting. Further in
    ' documents the extended ASCII codes (>127) are encoded as UTF-8 to get them
    ' displayed correctly. However, the Forum and Wiki exports keep the original
    ' codepage encoding. Note that this actually might cause wrong characters in
    ' the Forum/Wiki, but it is required so that the examples can be copied back
    ' from the codeboxes into the IDE, which is usually the purpose of examples.
    '----------
    pNam$ = ideprogname$: IF pNam$ = "" THEN pNam$ = "Untitled" + tempfolderindexstr$ + ".bas"
    SELECT CASE LCASE$(docFormat$)
        CASE "html": ext$ = ".htm"
        CASE "rich": ext$ = ".rtf"
        CASE ELSE: ext$ = ""
    END SELECT
    IF ext$ <> "" THEN
        IF _FILEEXISTS(idepath$ + idepathsep$ + pNam$ + ext$) THEN
            IF ideyesnobox$("Export As...", "Overwrite file " + pNam$ + ext$) = "N" THEN EXIT SUB
        END IF
    END IF
    GOSUB GetThemeColors
    cEol$ = CHR$(10) '       '=> line break char(s)
    IF INSTR(_OS$, "[LINUX]") = 0 THEN cEol$ = CHR$(13) + cEol$
    '------------------------------
    PCOPY 3, 2: SCREEN , , 3, 0
    sTxt$ = getSelectedText$(-1) '=> source code text (current selection)
    IF sTxt$ = "" THEN
        FOR i& = 1 TO iden '     '=> get full source, if no selection was made
            sTxt$ = sTxt$ + idegetline(i&) + cEol$
            perc$ = str2$(INT(30 / iden * i&))
            IdeInfo = CHR$(0) + STRING$(3 - LEN(perc$), 32) + perc$ + "% exported..."
            UpdateIdeInfo
        NEXT i&
        WHILE RIGHT$(sTxt$, LEN(cEol$)) = cEol$ 'normalize line feeds at EOF
            sTxt$ = LEFT$(sTxt$, LEN(sTxt$) - LEN(cEol$))
        WEND
    END IF
    IF sTxt$ = "" THEN sTxt$ = sTxt$ + cEol$: ELSE sTxt$ = sTxt$ + cEol$ + cEol$
    sLen& = LEN(sTxt$) '         '=> source code length
    sPos& = 1 '                  '=> source code read position
    eTxt$ = SPACE$(1000000) '    '=> export text buffer
    ePos& = 1 '                  '=> export text buffer write position
    '----------
    post% = 0 ''=> GOSUB argument = 0/-1 (close pre current / post current char)
    what$ = "" '=> GOSUB argument = command descriptor
    '----------
    co% = 0 '=> comment processing
    le% = 0 '=> legacy metacommand processing
    me% = 0 '=> QB64 metacommand processing
    kw% = 0 '=> keyword processing
    nu% = 0 '=> literal number processing
    qu% = 0 '=> quote (literal string) processing
    '----------
    op% = 0 '=> simply link to OPEN page
    ma$ = "@_ARCCOT@_ARCCSC@_ARCSEC@_COT@_COTH@_COSH@_CSC@_CSCH@_SEC@_SECH@_SINH@_TANH@" 'derived math functions
    fu% = 0 '=> Wiki page +(function) check required
    fu$ = "@_AUTODISPLAY@_BLEND@_BLINK@_CAPSLOCK@_CLEARCOLOR@_CLIPBOARD$@_CLIPBOARDIMAGE@_CONTROLCHR@_DEST@_DISPLAY@_EXIT@_FONT@_FULLSCREEN@_MAPUNICODE@_MEM@_MEMGET@_MESSAGEBOX@_NUMLOCK@_OFFSET@_PALETTECOLOR@_PRINTMODE@_RESIZE@_SCREENICON@_SCROLLLOCK@_SMOOTH@_SOURCE@_WIDTH@ASC@MID$@PLAY@SCREEN@SEEK@SHELL@TIMER@"
    bo% = 0 '=> Wiki page +(boolean) check required
    bo$ = "@AND@OR@XOR@"
    '----------
    pc% = 0 ''=> pre-compiler indicator
    ml% = 0 ''=> meta line indicator
    dl% = 0 ''=> data line indicator
    cu% = 0 ''=> custom keyword indicator
    lb% = 0 ''=> line break indicator
    nl% = -1 '=> new line indicator
    nt% = -1 '=> new token indicator
    '----------
    nc% = 0 '=> parenthesis nesting counter
    in% = 0 '=> ignore next keyword
    sk% = 0 '=> skip copying current char
    '----------
    GOSUB OpenCodeBlock
    WHILE sPos& <= sLen&
        perc$ = str2$(30 + INT(70 / sLen& * sPos&))
        IdeInfo = CHR$(0) + STRING$(3 - LEN(perc$), 32) + perc$ + "% exported..."
        UpdateIdeInfo
        '----------
        curr% = ASC(sTxt$, sPos&) '=> current char's ASCII value
        SELECT CASE curr% '       '=> general parsing and handling
            CASE 10, 13 'line feed
                IF NOT lb% THEN
                    IF me% THEN
                        IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE me$ = "": in% = 0
                        me% = 0: le% = 0
                    END IF
                    IF kw% THEN
                        IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE kw$ = "": in% = 0
                        kw% = 0
                    END IF
                    IF co% THEN post% = 0: what$ = "co": GOSUB CloseText: co% = 0
                    IF nu% THEN post% = 0: what$ = "nu": GOSUB CloseText: nu% = 0
                    IF sPos& > 1 THEN
                        IF ASC(sTxt$, sPos& - 1) <> 95 THEN op% = 0: fu% = 0: bo% = 0
                    ELSE
                        op% = 0: fu% = 0: bo% = 0
                    END IF
                    GOSUB EndLineOps
                    IF curr% = 13 THEN lb% = -1
                END IF
                IF curr% = 10 THEN pc% = 0: ml% = 0: dl% = 0: lb% = 0: nl% = -1: nt% = -1
            CASE 9, 32 'space
                IF me% THEN
                    GOSUB VerifyKeyword: GOSUB WriteLink: me% = 0: le% = 0
                    SELECT CASE UCASE$(me$)
                        CASE "$LET", "$ELSE", "$END": pc% = -1
                        CASE "$IF", "$ELSEIF": pc% = -1: bo% = -1
                    END SELECT
                END IF
                IF kw% THEN
                    IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE kw$ = "": in% = 0
                    kw% = 0: IF in% THEN sk% = -1
                    SELECT CASE UCASE$(kw$)
                        CASE "REM": IF NOT (co% OR qu%) THEN co% = -1: what$ = "co": GOSUB OpenText
                        CASE "DATA": dl% = -1
                        CASE "OPEN": op% = -1
                        CASE "CASE", "IF", "ELSEIF", "UNTIL", "WHILE": fu% = -1: bo% = -1
                        CASE "THEN", "ELSE": fu% = 0: bo% = 0
                        CASE ELSE
                            FOR i& = 1 TO idn
                                IF ids(i&).subfunc = 2 AND ids(i&).args > 0 THEN
                                    id$ = RTRIM$(ids(i&).n): uw$ = UCASE$(kw$)
                                    IF id$ = uw$ THEN fu% = -2: EXIT FOR
                                END IF
                            NEXT i&
                    END SELECT
                END IF
                IF nu% THEN post% = 0: what$ = "nu": GOSUB CloseText: nu% = 0
                nt% = -1
            CASE 34 '"
                IF NOT (co% OR qu%) THEN
                    qu% = -1: what$ = "qu": GOSUB OpenText
                ELSEIF qu% THEN
                    post% = -1: what$ = "qu": GOSUB CloseText: qu% = 0
                END IF
                IF NOT sk% THEN GOSUB EscapeChar 'html
            CASE 36 '$
                IF nl% OR le% THEN ml% = -1: me% = -1: me$ = "": nt% = 0
            CASE 38 '&
                IF nt% AND NOT (co% OR qu%) THEN
                    IF sPos& + 1 <= sLen& THEN
                        IF INSTR("BHO", CHR$(ASC(sTxt$, sPos& + 1))) > 0 THEN
                            nu% = -1: what$ = "nu": GOSUB OpenText: nt% = 0
                        END IF
                    END IF
                END IF
                IF NOT (me% OR kw%) THEN GOSUB EscapeChar 'html, wiki
            CASE 39 ''
                IF nl% THEN
                    IF sPos& + 1 <= sLen& THEN
                        IF ASC(sTxt$, sPos& + 1) = 36 THEN le% = -1: nt% = 0
                    END IF
                END IF
                IF NOT (co% OR qu%) THEN co% = -1: what$ = "co": GOSUB OpenText
            CASE 40, 41 '( )
                IF kw% THEN
                    IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE kw$ = "": in% = 0
                    kw% = 0
                END IF
                IF nu% THEN post% = 0: what$ = "nu": GOSUB CloseText: nu% = 0
                IF NOT (co% OR qu%) THEN
                    IF curr% = 40 THEN nc% = nc% + 1: ELSE nc% = nc% - 1
                END IF
                IF NOT (co% OR qu% OR (fu% < -1) OR bo%) THEN
                    IF nc% > 0 THEN fu% = -1: ELSE fu% = 0
                END IF
                nt% = -1
            CASE 42 TO 44, 47, 59 TO 62, 92, 94 '* + , / ; < = > \ ^
                IF kw% THEN
                    IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE kw$ = "": in% = 0
                    kw% = 0
                END IF
                IF nu% THEN post% = 0: what$ = "nu": GOSUB CloseText: nu% = 0
                IF curr% = 61 AND NOT (co% OR qu% OR (fu% < -1) OR bo%) THEN fu% = -3
                IF curr% = 60 OR curr% = 62 OR curr% = 92 THEN GOSUB EscapeChar 'html, rtf, wiki
                nt% = -1
            CASE 45 '-
                IF kw% THEN
                    IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE kw$ = "": in% = 0
                    kw% = 0
                END IF
                IF nu% THEN post% = 0: what$ = "nu": GOSUB CloseText: nu% = 0
                nt% = -1
                IF NOT (co% OR qu%) THEN
                    SELECT CASE ASC(sTxt$, sPos& - 1)
                        CASE 9, 32, 40, 42 TO 45, 47, 59 TO 63, 92, 94 'after this it may be a neg. sign
                            IF sPos& + 1 <= sLen& THEN
                                IF INSTR(".0123456789", CHR$(ASC(sTxt$, sPos& + 1))) > 0 THEN
                                    nu% = -1: what$ = "nu": GOSUB OpenText: nt% = 0
                                END IF
                            END IF
                    END SELECT
                END IF
            CASE 46 '.
                IF nt% AND NOT (co% OR qu%) THEN
                    IF sPos& + 1 <= sLen& THEN
                        IF INSTR("0123456789", CHR$(ASC(sTxt$, sPos& + 1))) > 0 THEN
                            nu% = -1: what$ = "nu": GOSUB OpenText: nt% = 0
                        END IF
                    END IF
                END IF
            CASE 48 TO 57 '0-9
                IF nt% AND NOT (co% OR qu%) THEN nu% = -1: what$ = "nu": GOSUB OpenText: nt% = 0
            CASE 58 ':
                IF me% THEN GOSUB VerifyKeyword: GOSUB WriteLink: me% = 0: le% = 0
                IF kw% THEN
                    IF NOT in% THEN GOSUB VerifyKeyword: GOSUB WriteLink: ELSE kw$ = "": in% = 0
                    kw% = 0
                END IF
                IF nu% THEN post% = 0: what$ = "nu": GOSUB CloseText: nu% = 0
                IF NOT (co% OR qu%) THEN op% = 0: fu% = 0: bo% = 0
                nt% = -1
            CASE 123, 125 '{ }
                GOSUB EscapeChar 'rtf
            CASE 65 TO 90, 97 TO 122 'A-Z a-z
                IF nt% AND NOT (co% OR qu%) THEN kw% = -1: kw$ = "": nt% = 0
                IF nl% AND UCASE$(MID$(sTxt$, sPos&, 5)) = "REM $" THEN le% = -1
            CASE 95 '_
                IF nt% AND NOT (co% OR qu%) THEN
                    IF sPos& + 1 <= sLen& THEN
                        IF ASC(sTxt$, sPos& + 1) <> 13 AND ASC(sTxt$, sPos& + 1) <> 10 THEN
                            kw% = -1: kw$ = "": nt% = 0
                        END IF
                    END IF
                END IF
            CASE IS > 127 'ext. ASCII
                GOSUB EscapeChar 'html, rtf, forum, wiki
            CASE ELSE 'control, non-semantics, type suffix w/o further meaning
                nt% = 0
        END SELECT
        SELECT CASE curr% '       '=> keyword accumulation
            CASE 33, 35 TO 38, 46, 48 TO 57, 65 TO 90, 95 TO 122, 126
                IF me% THEN me$ = me$ + CHR$(curr%): sk% = -1
                IF kw% THEN kw$ = kw$ + CHR$(curr%): sk% = -1
        END SELECT
        IF curr% <> 9 AND curr% <> 10 AND curr% <> 32 THEN nl% = 0
        IF NOT sk% THEN ASC(eTxt$, ePos&) = curr%: ePos& = ePos& + 1
        sk% = 0: sPos& = sPos& + 1
        '----------
        IF ePos& > LEN(eTxt$) - 10000 THEN eTxt$ = eTxt$ + SPACE$(1000000) 'expand export buffer, if needed
    WEND
    GOSUB CloseCodeBlock
    '----------
    PCOPY 2, 3
    SELECT CASE LCASE$(docFormat$)
        CASE "html", "rich"
            ideerror = 7
            OPEN idepath$ + idepathsep$ + pNam$ + ext$ FOR OUTPUT AS #151
            ideerror = 1
            PRINT #151, LEFT$(eTxt$, ePos& - 1);
            CLOSE #151
            ok% = idemessagebox("Export As...", "Export to " + pNam$ + ext$ + " completed.", "")
        CASE "disc", "foru", "wiki"
            _CLIPBOARD$ = LEFT$(eTxt$, ePos& - 1)
            ok% = idemessagebox("Export As...", "Discord/Forum/Wiki export to Clipboard completed.", "")
    END SELECT
    EXIT SUB
    '------------------------------
    OpenCodeBlock:
    SELECT CASE LCASE$(docFormat$)
        CASE "html": tmp$ = "<!DOCTYPE html><html lang=" + CHR$(34) + "en" + CHR$(34) + "><head><meta charset=" + CHR$(34) + "UTF-8" + CHR$(34) + "><title>" + AnsiTextToUtf8Text$(pNam$) + "</title></head><body><pre style=" + CHR$(34) + "font-size: 18px; background-color: " + bgc$ + "; color: " + txc$ + ";" + CHR$(34) + ">"
        CASE "rich": tmp$ = "{\rtf1\ansi\deff0{\fonttbl{\f0 Courier New;}}{\colortbl " + rtc$ + "}\pard\f0\fs32\cbpat6\paperh23811\paperw16838\margl142\margr142\margt142\margb142"
        CASE "disc": tmp$ = "```ansi" + cEol$ + CHR$(27) + "[0;0;1;37m"
        CASE "foru": tmp$ = "[qb=export]"
        CASE "wiki": tmp$ = "{{CodeStart}}"
        CASE ELSE: RETURN
    END SELECT
    MID$(eTxt$, ePos&, LEN(tmp$)) = tmp$: ePos& = ePos& + LEN(tmp$)
    IF LCASE$(docFormat$) <> "disc" AND LCASE$(docFormat$) <> "foru" THEN
        MID$(eTxt$, ePos&, LEN(cEol$)) = cEol$: ePos& = ePos& + LEN(cEol$)
    END IF
    RETURN
    '----------
    CloseCodeBlock:
    SELECT CASE LCASE$(docFormat$)
        CASE "html": tmp$ = "</pre></body></html>"
        CASE "rich": tmp$ = "}": ePos& = ePos& - 4 'remove final /par
        CASE "disc": tmp$ = "```"
        CASE "foru": tmp$ = "[/qb]"
        CASE "wiki": tmp$ = "{{CodeEnd}}"
        CASE ELSE: RETURN
    END SELECT
    ePos& = ePos& - LEN(cEol$) 'remove final code line feed
    MID$(eTxt$, ePos&, LEN(tmp$)) = tmp$: ePos& = ePos& + LEN(tmp$)
    MID$(eTxt$, ePos&, LEN(cEol$)) = cEol$: ePos& = ePos& + LEN(cEol$)
    RETURN
    '----------
    OpenText:
    IF ml% < -1 AND NOT pc% AND LCASE$(what$) <> "co" THEN RETURN
    SELECT CASE LCASE$(docFormat$)
        CASE "html"
            SELECT CASE LCASE$(what$)
                CASE "co": tmp$ = "<span style=" + CHR$(34) + "color: " + coc$ + ";" + CHR$(34) + ">"
                CASE "nu": tmp$ = "<span style=" + CHR$(34) + "color: " + nuc$ + ";" + CHR$(34) + ">"
                CASE "qu": tmp$ = "<span style=" + CHR$(34) + "color: " + quc$ + ";" + CHR$(34) + ">"
                CASE ELSE: RETURN
            END SELECT
        CASE "rich"
            SELECT CASE LCASE$(what$)
                CASE "co": tmp$ = "\cf1 "
                CASE "nu": tmp$ = "\cf4 "
                CASE "qu": tmp$ = "\cf5 "
                CASE ELSE: RETURN
            END SELECT
        CASE "disc"
            SELECT CASE LCASE$(what$)
                CASE "co": tmp$ = CHR$(27) + "[30m"
                CASE "nu": tmp$ = CHR$(27) + "[31m"
                CASE "qu": tmp$ = CHR$(27) + "[33m"
                CASE ELSE: RETURN
            END SELECT
        CASE "foru"
            SELECT CASE LCASE$(what$)
                CASE "co": tmp$ = "[color=#919191]"
                CASE "nu": tmp$ = "[color=#F580B1]"
                CASE "qu": tmp$ = "[color=#FFB100]"
                CASE ELSE: RETURN
            END SELECT
        CASE "wiki"
            SELECT CASE LCASE$(what$)
                CASE "co", "qu": tmp$ = "{{Text|<nowiki>"
                CASE "nu": tmp$ = "{{Text|"
                CASE ELSE: RETURN
            END SELECT
        CASE ELSE: RETURN
    END SELECT
    MID$(eTxt$, ePos&, LEN(tmp$)) = tmp$: ePos& = ePos& + LEN(tmp$)
    RETURN
    '----------
    CloseText:
    IF ml% < -1 AND NOT pc% AND LCASE$(what$) <> "co" THEN RETURN
    SELECT CASE LCASE$(docFormat$)
        CASE "html"
            SELECT CASE LCASE$(what$)
                CASE "co", "nu", "qu": tmp$ = "</span>"
                CASE ELSE: RETURN
            END SELECT
        CASE "rich"
            SELECT CASE LCASE$(what$)
                CASE "co", "nu", "qu": tmp$ = "\cf0 "
                CASE ELSE: RETURN
            END SELECT
        CASE "disc"
            SELECT CASE LCASE$(what$)
                CASE "co", "nu", "qu": tmp$ = CHR$(27) + "[37m"
                CASE ELSE: RETURN
            END SELECT
        CASE "foru"
            SELECT CASE LCASE$(what$)
                CASE "co", "nu", "qu": tmp$ = "[/color]"
                CASE ELSE: RETURN
            END SELECT
        CASE "wiki"
            SELECT CASE LCASE$(what$)
                CASE "co": tmp$ = "</nowiki>|#919191}}"
                CASE "nu": tmp$ = "|#F580B1}}"
                CASE "qu": tmp$ = "</nowiki>|#FFB100}}"
                CASE ELSE: RETURN
            END SELECT
        CASE ELSE: RETURN
    END SELECT
    IF post% THEN
        sk% = 0: GOSUB EscapeChar
        IF NOT sk% THEN ASC(eTxt$, ePos&) = curr%: ePos& = ePos& + 1: sk% = -1
    END IF
    MID$(eTxt$, ePos&, LEN(tmp$)) = tmp$: ePos& = ePos& + LEN(tmp$)
    RETURN
    '----------
    VerifyKeyword:
    IF me% THEN veri$ = me$: ELSE veri$ = kw$
    IF ASC(veri$, 1) <> 95 THEN flp% = 1: ELSE flp% = 2
    IF (ASC(veri$, flp%) = 36 OR isalpha(ASC(veri$, flp%))) AND (INSTR(listOfKeywords$, "@" + UCASE$(veri$) + "@") > 0) THEN
        IF me% AND le% THEN
            IF INSTR("$DYNAMIC$INCLUDE$STATIC$FORMAT", UCASE$(veri$)) = 0 THEN me$ = ""
        ELSEIF me% AND NOT le% THEN
            IF INSTR("$DYNAMIC$INCLUDE$STATIC$FORMAT", UCASE$(veri$)) > 0 THEN me$ = ""
        END IF
        IF ((ml% < -1 AND NOT pc%) OR dl%) AND me% THEN me$ = ""
        IF ((ml% < -1 AND NOT pc%) OR dl%) AND kw% THEN kw$ = ""
        IF pc% THEN me$ = veri$
    ELSEIF NOT (ml% < 0 OR dl%) AND INSTR(listOfCustomKeywords$, "@" + UCASE$(removesymbol2$(veri$)) + "@") > 0 THEN
        cu% = -1
    ELSEIF pc% AND INSTR(UserDefineList$, "@" + UCASE$(veri$) + "@") > 0 THEN
        cu% = -1
    ELSE
        IF me% THEN me$ = "": ELSE kw$ = ""
    END IF
    IF ml% < 0 AND curr% <> 58 THEN ml% = -2
    RETURN
    '----------
    FindWikiPage:
    IF me% THEN page$ = UCASE$(me$): ELSE page$ = UCASE$(kw$) 'Wiki pages are all caps
    IF op% THEN
        SELECT CASE page$
            CASE "OPEN", "AS": page$ = "OPEN": RETURN
            CASE "LEN"
                la$ = LTRIM$(StrReplace$(MID$(sTxt$, sPos&, 100), CHR$(9), " "))
                IF LEFT$(la$, 1) <> "(" THEN page$ = "OPEN": RETURN
            CASE "ACCESS", "LOCK", "SHARED", "READ", "WRITE": page$ = "OPEN#File_ACCESS_and_LOCK_Permissions": RETURN
            CASE "FOR", "OUTPUT", "APPEND", "INPUT", "BINARY", "RANDOM": page$ = "OPEN#File_Access_Modes": RETURN
        END SELECT
    END IF
    IF fu% < 0 AND INSTR(fu$, "@" + page$ + "@") > 0 THEN
        page$ = page$ + " (function)"
    ELSEIF bo% AND INSTR(bo$, "@" + page$ + "@") > 0 THEN
        page$ = page$ + " (boolean)"
    ELSEIF INSTR(ma$, "@" + page$ + "@") > 0 THEN
        page$ = "Mathematical Operations#Derived_Mathematical_Functions"
    ELSE
        la$ = LTRIM$(StrReplace$(MID$(sTxt$, sPos&, 100), CHR$(9), " "))
        SELECT EVERYCASE page$
            CASE "$END": IF UCASE$(LEFT$(la$, 2)) = "IF" THEN me$ = me$ + " " + LEFT$(la$, 2): page$ = "$END IF": in% = -1
            CASE "_CONSOLECURSOR"
                IF UCASE$(LEFT$(la$, 5)) = "_HIDE" THEN kw$ = kw$ + " " + LEFT$(la$, 5): in% = -1
                IF UCASE$(LEFT$(la$, 5)) = "_SHOW" THEN kw$ = kw$ + " " + LEFT$(la$, 5): in% = -1
            CASE "CALL": IF UCASE$(LEFT$(la$, 8)) = "ABSOLUTE" THEN kw$ = kw$ + " " + LEFT$(la$, 8): page$ = "CALL ABSOLUTE": in% = -1
            CASE "CASE"
                IF UCASE$(LEFT$(la$, 2)) = "IS" THEN kw$ = kw$ + " " + LEFT$(la$, 2): page$ = "CASE IS": fu% = -1: bo% = -1: in% = -1
                IF UCASE$(LEFT$(la$, 4)) = "ELSE" THEN kw$ = kw$ + " " + LEFT$(la$, 4): page$ = "CASE ELSE": fu% = 0: bo% = 0: in% = -1
            CASE "DECLARE": IF UCASE$(LEFT$(la$, 7)) = "LIBRARY" THEN kw$ = kw$ + " " + LEFT$(la$, 7): page$ = "DECLARE LIBRARY": in% = -1
            CASE "DEF": IF UCASE$(LEFT$(la$, 3)) = "SEG" THEN kw$ = kw$ + " " + LEFT$(la$, 3): page$ = "DEF SEG": in% = -1
            CASE "DO"
                IF UCASE$(LEFT$(la$, 5)) = "WHILE" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "DO...LOOP": fu% = -1: bo% = -1: in% = -1
                IF UCASE$(LEFT$(la$, 5)) = "UNTIL" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "DO...LOOP": fu% = -1: bo% = -1: in% = -1
            CASE "END"
                IF UCASE$(LEFT$(la$, 7)) = "DECLARE" THEN kw$ = kw$ + " " + LEFT$(la$, 7): page$ = "END DECLARE": in% = -1
                IF UCASE$(LEFT$(la$, 8)) = "FUNCTION" THEN kw$ = kw$ + " " + LEFT$(la$, 8): page$ = "END FUNCTION": in% = -1
                IF UCASE$(LEFT$(la$, 2)) = "IF" THEN kw$ = kw$ + " " + LEFT$(la$, 2): page$ = "END IF": in% = -1
                IF UCASE$(LEFT$(la$, 6)) = "SELECT" THEN kw$ = kw$ + " " + LEFT$(la$, 6): page$ = "END SELECT": in% = -1
                IF UCASE$(LEFT$(la$, 3)) = "SUB" THEN kw$ = kw$ + " " + LEFT$(la$, 3): page$ = "END SUB": in% = -1
                IF UCASE$(LEFT$(la$, 4)) = "TYPE" THEN kw$ = kw$ + " " + LEFT$(la$, 4): page$ = "END TYPE": in% = -1
            CASE "EXIT"
                IF UCASE$(LEFT$(la$, 4)) = "CASE" THEN kw$ = kw$ + " " + LEFT$(la$, 4): page$ = "EXIT CASE": in% = -1
                IF UCASE$(LEFT$(la$, 2)) = "DO" THEN kw$ = kw$ + " " + LEFT$(la$, 2): page$ = "EXIT DO": in% = -1
                IF UCASE$(LEFT$(la$, 3)) = "FOR" THEN kw$ = kw$ + " " + LEFT$(la$, 3): page$ = "EXIT FOR": in% = -1
                IF UCASE$(LEFT$(la$, 8)) = "FUNCTION" THEN kw$ = kw$ + " " + LEFT$(la$, 8): page$ = "EXIT FUNCTION": in% = -1
                IF UCASE$(LEFT$(la$, 6)) = "SELECT" THEN kw$ = kw$ + " " + LEFT$(la$, 6): page$ = "EXIT SELECT": in% = -1
                IF UCASE$(LEFT$(la$, 3)) = "SUB" THEN kw$ = kw$ + " " + LEFT$(la$, 3): page$ = "EXIT SUB": in% = -1
                IF UCASE$(LEFT$(la$, 5)) = "WHILE" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "EXIT WHILE": in% = -1
            CASE "GET", "PUT": IF LEFT$(la$, 1) <> "#" THEN page$ = page$ + " (general)"
            CASE "KEY": IF UCASE$(LEFT$(la$, 4)) = "LIST" THEN kw$ = kw$ + " " + LEFT$(la$, 4): page$ = "KEY LIST": in% = -1
            CASE "LPRINT": IF UCASE$(LEFT$(la$, 5)) = "USING" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "LPRINT USING": in% = -1
            CASE "LINE"
                IF UCASE$(LEFT$(la$, 5)) = "INPUT" THEN
                    kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "LINE INPUT": in% = -1
                    IF LEFT$(LTRIM$(MID$(la$, 6)), 1) = "#" THEN page$ = page$ + " (file statement)"
                END IF
            CASE "LOOP"
                IF UCASE$(LEFT$(la$, 5)) = "WHILE" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "DO...LOOP": fu% = -1: bo% = -1: in% = -1
                IF UCASE$(LEFT$(la$, 5)) = "UNTIL" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "DO...LOOP": fu% = -1: bo% = -1: in% = -1
            CASE "ON"
                IF UCASE$(LEFT$(la$, 5)) = "ERROR" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "ON ERROR": in% = -1
                IF UCASE$(LEFT$(la$, 3)) = "KEY" THEN kw$ = kw$ + " " + LEFT$(la$, 3): page$ = "ON KEY(n)": in% = -1
                IF UCASE$(LEFT$(la$, 5)) = "STRIG" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "ON STRIG(n)": in% = -1
                IF UCASE$(LEFT$(la$, 5)) = "TIMER" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "ON TIMER(n)": in% = -1
            CASE "OPTION": IF UCASE$(LEFT$(la$, 4)) = "BASE" THEN kw$ = kw$ + " " + LEFT$(la$, 4): page$ = "OPTION BASE": in% = -1
            CASE "PALETTE": IF UCASE$(LEFT$(la$, 5)) = "USING" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "PALETTE USING": in% = -1
            CASE "PRINT"
                IF UCASE$(LEFT$(la$, 5)) = "USING" THEN
                    kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "PRINT USING": in% = -1
                    IF LEFT$(LTRIM$(MID$(la$, 6)), 1) = "#" THEN page$ = page$ + " (file statement)"
                END IF
            CASE "RANDOMIZE": IF UCASE$(LEFT$(la$, 5)) = "USING" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "RANDOMIZE USING": in% = -1
            CASE "SELECT"
                IF UCASE$(LEFT$(la$, 4)) = "CASE" THEN kw$ = kw$ + " " + LEFT$(la$, 4): page$ = "SELECT CASE": fu% = -1: bo% = -1: in% = -1
                IF UCASE$(LEFT$(la$, 9)) = "EVERYCASE" THEN kw$ = kw$ + " " + LEFT$(la$, 9): page$ = "SELECT CASE": fu% = -1: bo% = -1: in% = -1
            CASE "SOUND"
                IF UCASE$(LEFT$(la$, 4)) = "WAIT" THEN kw$ = kw$ + " " + LEFT$(la$, 4): in% = -1
                IF UCASE$(LEFT$(la$, 6)) = "RESUME" THEN kw$ = kw$ + " " + LEFT$(la$, 6): in% = -1
            CASE "VIEW": IF UCASE$(LEFT$(la$, 5)) = "PRINT" THEN kw$ = kw$ + " " + LEFT$(la$, 5): page$ = "VIEW PRINT": in% = -1
            CASE "INPUT", "PRINT", "WRITE": IF LEFT$(la$, 1) = "#" THEN page$ = page$ + " (file statement)"
        END SELECT
    END IF
    RETURN
    '----------
    WriteLink:
    IF (me% AND me$ = "") OR (kw% AND kw$ = "") GOTO UnknownNoLinkNoColor
    IF cu% GOTO CustomNoLink
    GOSUB FindWikiPage
    IF me% AND le% AND co% THEN post% = 0: what$ = "co": GOSUB CloseText: co% = 0
    IF me% OR pc% THEN lnk$ = me$: ELSE lnk$ = kw$
    pal% = LEN(page$): lkl% = LEN(lnk$)
    SELECT CASE LCASE$(docFormat$)
        CASE "html"
            IF me% OR pc% THEN lkc$ = mec$: ELSE lkc$ = kwc$
            MID$(eTxt$, ePos&, (2 * pal%) + lkl% + 120) = "<a style=" + CHR$(34) + "text-decoration: none; color: " + lkc$ + ";" + CHR$(34) + " href=" + CHR$(34) + "https://qb64phoenix.com/qb64wiki/index.php?title=" + page$ + CHR$(34) + " title=" + CHR$(34) + page$ + CHR$(34) + ">" + lnk$ + "</a>"
            ePos& = ePos& + (2 * pal%) + lkl% + 120
        CASE "rich"
            IF me% OR pc% THEN lkc$ = "\cf2": ELSE lkc$ = "\cf3"
            MID$(eTxt$, ePos&, pal% + lkl% + 108) = "{\field{\*\fldinst HYPERLINK " + CHR$(34) + "https://qb64phoenix.com/qb64wiki/index.php?title=" + page$ + CHR$(34) + "}{\fldrslt{" + lkc$ + "\ul0 " + lnk$ + "}}}\cf0 "
            ePos& = ePos& + pal% + lkl% + 108
        CASE "disc"
            'linking to wiki not supported in Discord, hence we do coloring only
            IF me% OR pc% THEN lkc$ = CHR$(27) + "[32m": ELSE lkc$ = CHR$(27) + "[34m"
            MID$(eTxt$, ePos&, lkl% + 10) = lkc$ + lnk$ + CHR$(27) + "[37m"
            ePos& = ePos& + lkl% + 10
        CASE "foru"
            IF me% OR pc% THEN lkc$ = "#55FF55": ELSE lkc$ = "#4593D8"
            MID$(eTxt$, ePos&, pal% + lkl% + 84) = "[url=https://qb64phoenix.com/qb64wiki/index.php?title=" + page$ + "][color=" + lkc$ + "]" + lnk$ + "[/color][/url]"
            ePos& = ePos& + pal% + lkl% + 84
        CASE "wiki"
            IF me% OR pc% THEN lkc$ = "{{Cm|": ELSE lkc$ = "{{Cl|"
            IF page$ = lnk$ THEN
                MID$(eTxt$, ePos&, lkl% + 7) = lkc$ + lnk$ + "}}"
                ePos& = ePos& + lkl% + 7
            ELSE
                MID$(eTxt$, ePos&, pal% + lkl% + 8) = lkc$ + page$ + "|" + lnk$ + "}}"
                ePos& = ePos& + pal% + lkl% + 8
            END IF
        CASE ELSE: RETURN
    END SELECT
    RETURN
    '---
    CustomNoLink:
    cu% = 0: kwl% = LEN(kw$)
    SELECT CASE LCASE$(docFormat$)
        CASE "html"
            kw$ = StrReplace$(kw$, "&", "&amp;"): kwl% = LEN(kw$)
            MID$(eTxt$, ePos&, kwl% + 37) = "<span style=" + CHR$(34) + "color: " + mec$ + ";" + CHR$(34) + ">" + kw$ + "</span>"
            ePos& = ePos& + kwl% + 37
        CASE "rich"
            MID$(eTxt$, ePos&, kwl% + 10) = "\cf2 " + kw$ + "\cf0 "
            ePos& = ePos& + kwl% + 10
        CASE "disc"
            MID$(eTxt$, ePos&, kwl% + 10) = CHR$(27) + "[32m" + kw$ + CHR$(27) + "[37m"
            ePos& = ePos& + kwl% + 10
        CASE "foru"
            MID$(eTxt$, ePos&, kwl% + 23) = "[color=#55FF55]" + kw$ + "[/color]"
            ePos& = ePos& + kwl% + 23
        CASE "wiki"
            MID$(eTxt$, ePos&, kwl% + 17) = "{{Text|" + kw$ + "|#55FF55}}"
            ePos& = ePos& + kwl% + 17
        CASE ELSE: RETURN
    END SELECT
    RETURN
    '---
    UnknownNoLinkNoColor:
    IF LCASE$(docFormat$) = "html" THEN veri$ = StrReplace$(veri$, "&", "&amp;")
    MID$(eTxt$, ePos&, LEN(veri$)) = veri$: ePos& = ePos& + LEN(veri$): veri$ = ""
    RETURN
    '----------
    EscapeChar:
    SELECT CASE LCASE$(docFormat$)
        CASE "html"
            SELECT CASE curr%
                CASE 34: ech$ = "&quot;": sk% = -1
                CASE 38: ech$ = "&amp;": sk% = -1
                CASE 60: ech$ = "&lt;": sk% = -1
                CASE 62: ech$ = "&gt;": sk% = -1
                CASE IS > 127
                    uni& = _MAPUNICODE(curr%)
                    IF uni& = 0 THEN uni& = 65533 'replacement character
                    ech$ = UnicodeToUtf8Char$(uni&): sk% = -1
                CASE ELSE: RETURN
            END SELECT
        CASE "rich"
            SELECT CASE curr%
                CASE 92, 123, 125: ech$ = "\"
                CASE IS > 127
                    uni& = _MAPUNICODE(curr%)
                    IF uni& = 0 THEN uni& = 65533 'replacement character
                    ech$ = "\u" + LTRIM$(STR$(uni&)) + "\'bf": sk% = -1
                CASE ELSE: RETURN
            END SELECT
        CASE "foru" '         'Keeps the original encoding, so Forum/Wiki examples can be copied
            SELECT CASE curr% 'back to the IDE. However, chars appear wrong in the Forum/Wiki.
                CASE IS > 127: ech$ = "&#" + LTRIM$(STR$(curr%)) + ";": sk% = -1
                CASE ELSE: RETURN
            END SELECT
        CASE "wiki" '         'Keeps the original encoding, so Forum/Wiki examples can be copied
            SELECT CASE curr% 'back to the IDE. However, chars appear wrong in the Forum/Wiki.
                CASE 38: ech$ = "&amp;": sk% = -1
                CASE 60: ech$ = "&lt;": sk% = -1
                CASE 62: ech$ = "&gt;": sk% = -1
                CASE IS > 127: ech$ = "&#" + LTRIM$(STR$(curr%)) + ";": sk% = -1
                CASE ELSE: RETURN
            END SELECT
        CASE ELSE: RETURN
    END SELECT
    MID$(eTxt$, ePos&, LEN(ech$)) = ech$: ePos& = ePos& + LEN(ech$)
    RETURN
    '----------
    EndLineOps:
    SELECT CASE LCASE$(docFormat$)
        CASE "rich": tmp$ = "\par"
        CASE ELSE: RETURN
    END SELECT
    MID$(eTxt$, ePos&, LEN(tmp$)) = tmp$: ePos& = ePos& + LEN(tmp$)
    RETURN
    '----------
    GetThemeColors:
    txc$ = "#" + RIGHT$(HEX$(IDETextColor), 6)
    rtc$ = "\red" + LTRIM$(STR$(_RED32(IDETextColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDETextColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDETextColor))) + ";"
    coc$ = "#" + RIGHT$(HEX$(IDECommentColor), 6)
    rtc$ = rtc$ + "\red" + LTRIM$(STR$(_RED32(IDECommentColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDECommentColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDECommentColor))) + ";"
    mec$ = "#" + RIGHT$(HEX$(IDEMetaCommandColor), 6)
    rtc$ = rtc$ + "\red" + LTRIM$(STR$(_RED32(IDEMetaCommandColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDEMetaCommandColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDEMetaCommandColor))) + ";"
    kwc$ = "#" + RIGHT$(HEX$(IDEKeywordColor), 6)
    rtc$ = rtc$ + "\red" + LTRIM$(STR$(_RED32(IDEKeywordColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDEKeywordColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDEKeywordColor))) + ";"
    nuc$ = "#" + RIGHT$(HEX$(IDENumbersColor), 6)
    rtc$ = rtc$ + "\red" + LTRIM$(STR$(_RED32(IDENumbersColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDENumbersColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDENumbersColor))) + ";"
    quc$ = "#" + RIGHT$(HEX$(IDEQuoteColor), 6)
    rtc$ = rtc$ + "\red" + LTRIM$(STR$(_RED32(IDEQuoteColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDEQuoteColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDEQuoteColor))) + ";"
    bgc$ = "#" + RIGHT$(HEX$(IDEBackgroundColor), 6)
    rtc$ = rtc$ + "\red" + LTRIM$(STR$(_RED32(IDEBackgroundColor))) + "\green" + LTRIM$(STR$(_GREEN32(IDEBackgroundColor))) + "\blue" + LTRIM$(STR$(_BLUE32(IDEBackgroundColor))) + ";"
    RETURN
END SUB

FUNCTION UnicodeToUtf8Char$ (unicode&)
    '--- UTF-8 encoding ---
    IF unicode& < 128 THEN
        '--- standard ASCII (0-127) goes as is ---
        UnicodeToUtf8Char$ = CHR$(unicode&)
    ELSE
        '--- encode the Unicode into UTF-8 notation ---
        utf$ = "": uc& = unicode& 'avoid argument side effect
        first% = &B10000000: remain% = 63
        DO
            first% = &B10000000 OR (first% \ 2): remain% = (remain% \ 2)
            conti% = &B10000000 OR (uc& AND &B00111111): uc& = uc& \ 64
            utf$ = CHR$(conti%) + utf$
            IF uc& <= remain% THEN
                first% = (first% OR uc&): uc& = 0
            END IF
        LOOP UNTIL uc& = 0
        UnicodeToUtf8Char$ = CHR$(first%) + utf$
    END IF
END FUNCTION

FUNCTION AnsiTextToUtf8Text$ (text$)
    utf$ = ""
    FOR chi& = 1 TO LEN(text$)
        '--- get ANSI char code ---
        ascii% = ASC(text$, chi&)
        IF ascii% > 127 THEN
            '--- read Unicode from active codepage ---
            unicode& = _MAPUNICODE(ascii%)
            '--- convert and add UTF-8 char ---
            IF unicode& = 0 THEN unicode& = 65533 'replacement character
            utf$ = utf$ + UnicodeToUtf8Char$(unicode&)
        ELSE
            '--- standard ASCII (0-127) goes as is ---
            utf$ = utf$ + CHR$(ascii%)
        END IF
    NEXT chi&
    AnsiTextToUtf8Text$ = utf$
END FUNCTION

