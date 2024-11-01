FUNCTION Back2BackName$ (a$)
    SELECT CASE a$
        CASE "Base Comparisons": Back2BackName$ = "Base Compare"
        CASE "Bitwise Operators": Back2BackName$ = "Bitwise OPs"
        CASE "Downloading Files": Back2BackName$ = "Downloads"
        CASE "Function (explanatory)": Back2BackName$ = "FUNC expl."
        CASE "Greater Than Or Equal": Back2BackName$ = "Greater|Equal"
        CASE "Keyboard scancodes": Back2BackName$ = "KB Scancodes"
        CASE "Keyword Reference - Alphabetical": Back2BackName$ = "KWs Alphab." 'scan sources for
        CASE "Keyword Reference - By usage": Back2BackName$ = "KWs by Usage" '  '!!! RS:HCWD:#1 !!!
        CASE "Keywords currently not supported by QB64": Back2BackName$ = "Unsupp. KWs"
        CASE "Less Than Or Equal": Back2BackName$ = "Less|Equal"
        CASE "Mathematical Operations": Back2BackName$ = "Math OPs"
        CASE "QB64 Help Menu": Back2BackName$ = "QB64 Help"
        CASE "Quick Reference - Tables": Back2BackName$ = "QRef. Tables"
        CASE "Relational Operations": Back2BackName$ = "Relational OPs"
        CASE "Scientific notation": Back2BackName$ = "Sci. Notation"
        CASE "Sub (explanatory)": Back2BackName$ = "SUB expl."
        CASE "Windows Environment": Back2BackName$ = "Win Env."
        CASE "Windows Libraries": Back2BackName$ = "Win Lib."
        CASE "Windows Printer Settings": Back2BackName$ = "Win Print"
        CASE "Windows Registry Access": Back2BackName$ = "Win Reg."
        CASE ELSE: Back2BackName$ = a$
    END SELECT
END FUNCTION

FUNCTION Wiki$ (PageName$) 'Read cached wiki page (download, if not yet cached)
    IF LEFT$(PageName$, 9) <> "Template:" THEN Help_PageLoaded$ = PageName$

    'Escape all invalid and other critical chars in filenames
    PageName2$ = ""
    FOR i = 1 TO LEN(PageName$)
        c = ASC(PageName$, i)
        SELECT CASE c
            CASE 32 '                                            '(space)
                PageName2$ = PageName2$ + "_"
            CASE 34, 36, 38, 42, 43, 47, 58, 60, 62, 63, 92, 124 '("$&*+/:<>?\|)
                PageName2$ = PageName2$ + "%" + HEX$(c)
            CASE ELSE
                PageName2$ = PageName2$ + CHR$(c)
        END SELECT
    NEXT
    PageName3$ = wikiSafeName$(PageName2$) 'case independent name

    'Is this page in the cache?
    IF Help_IgnoreCache = 0 THEN
        IF _FILEEXISTS(Cache_Folder$ + "/" + PageName3$ + ".txt") THEN
            a$ = _READFILE$(Cache_Folder$ + "/" + PageName3$ + ".txt")
            Wiki$ = StrReplace$(a$, CHR$(13) + CHR$(10), CHR$(10))
            EXIT FUNCTION
        END IF
    END IF

    'Download message (Status Bar)
    IF Help_Recaching = 0 THEN
        a$ = "Downloading '" + PageName$ + "' page..."
        IF LEN(a$) > 60 THEN a$ = LEFT$(a$, 57) + STRING$(3, 250)
        IF LEN(a$) < 60 THEN a$ = a$ + SPACE$(60 - LEN(a$))

        COLOR 0, 3: LOCATE idewy + idesubwindow, 2
        PRINT a$;

        PCOPY 3, 0
    END IF

    'Url query and output file name
    url$ = wikiBaseAddress$ + "/index.php?title=" + PageName2$ + "&action=edit"
    outputFile$ = Cache_Folder$ + "/" + PageName3$ + ".txt"
    'Wikitext delimiters
    s1$ = "name=" + CHR$(34) + "wpTextbox1" + CHR$(34) + ">"
    s2$ = "</textarea>"

    'Download page using (lib)curl
    IF PageName$ = "Initialize" OR PageName$ = "Update All" THEN
        a$ = "" 'dummy pages (for error display)
    ELSE
        a$ = wikiDLPage$(url$, 15)
        IF INSTR(a$, "Login required") > 0 THEN a$ = s1$ + s2$ 'continue as empty page
    END IF

    'Find wikitext in the downloaded page
    s1 = INSTR(a$, s1$)
    IF s1 > 0 THEN a$ = MID$(a$, s1 + LEN(s1$)): s2 = INSTR(a$, s2$): ELSE s2 = 0
    IF s2 > 0 THEN a$ = LEFT$(a$, s2 - 1)
    IF s1 > 0 AND s2 > 0 THEN
        IF a$ <> "" THEN
            'If wikitext was found, then substitute stuff & save it
            '--- first HTML specific entities
            WHILE INSTR(a$, "&amp;") > 0 '         '&amp; must be first and looped until all
                a$ = StrReplace$(a$, "&amp;", "&") 'multi-escapes are resolved (eg. &amp;lt; &amp;amp;lt; etc.)
            WEND
            a$ = StrReplace$(a$, "&lt;", "<")
            a$ = StrReplace$(a$, "&gt;", ">")
            a$ = StrReplace$(a$, "&quot;", CHR$(34))
            '--- wiki redirects & crlf
            a$ = StrReplace$(a$, "#REDIRECT", "See page")
            a$ = StrReplace$(a$, CHR$(13) + CHR$(10), CHR$(10))
            WHILE LEFT$(a$, 1) = CHR$(10): a$ = MID$(a$, 2): WEND
            IF LEN(a$) > 0 AND RIGHT$(a$, 1) <> CHR$(10) THEN a$ = a$ + CHR$(10)
            '--- put a download date/time entry
            a$ = "{{QBDLDATE:" + DATE$ + "}}" + CHR$(10) + "{{QBDLTIME:" + TIME$ + "}}" + CHR$(10) + a$
            '--- now save it
            fh = FREEFILE
            OPEN outputFile$ FOR OUTPUT AS #fh
            PRINT #fh, a$;
            CLOSE #fh
        ELSE
            'if page returns empty, then it's either
            IF _FILEEXISTS(outputFile$) THEN
                KILL outputFile$ 'an old no longer existing/needed page
            ELSE
                'or a new not yet created page
                a$ = CHR$(10) + "{{PageInternalError}}" + CHR$(10) +_
                     "* The requested page is not yet available in the Wiki." + CHR$(10) +_
                     "** If this is a new keyword, which was recently added to the language, then " +_
                     "please allow some time for the developers to add it and recheck later." + CHR$(10)
            END IF
        END IF
    ELSE
        'download failure, page corrupted, no text delimiters found
        a$ = CHR$(10) + "{{PageInternalError}}" + CHR$(10) +_
             "* For some unknown reason the download of the requested page failed." + CHR$(10) +_
             "** You may try ''Update Current Page'' from the ''Help'' menu." + CHR$(10) +_
             ";Note:This may also just be a temporary server issue. If the problem persists " +_
             "after waiting some time, then please feel free to leave us a message." + CHR$(10)
    END IF

    Wiki$ = a$
END FUNCTION

SUB Help_AddTxt (t$, col, link) 'Add help text, handle word wrap
    IF t$ = "" THEN EXIT SUB
    IF Help_ChkBlank <> 0 THEN Help_CheckBlankLine

    FOR i = 1 TO LEN(t$)
        c = ASC(t$, i)

        IF (Help_LockParse = -1 OR Help_LockParse = 0) AND Help_LockWrap = 0 THEN

            IF c = 32 THEN
                IF Help_Pos = Help_ww THEN Help_NewLine: _CONTINUE

                Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 32
                Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = col + Help_BG_Col * 16
                Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = link AND 255
                Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = link \ 256

                Help_Wrap_Pos = Help_Txt_Len 'pos to backtrack to when wrapping content
                Help_Pos = Help_Pos + 1: _CONTINUE
            END IF

            IF Help_Pos > Help_ww THEN
                IF Help_Wrap_Pos THEN 'attempt to wrap
                    'backtrack, insert new line, continue

                    b$ = MID$(Help_Txt$, Help_Wrap_Pos + 1, Help_Txt_Len - Help_Wrap_Pos)

                    Help_Txt_Len = Help_Wrap_Pos

                    Help_NewLine

                    MID$(Help_Txt$, Help_Txt_Len + 1, LEN(b$)) = b$: Help_Txt_Len = Help_Txt_Len + LEN(b$)

                    Help_Pos = Help_Pos + LEN(b$) \ 4
                END IF
            END IF

        END IF

        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = c
        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = col + Help_BG_Col * 16
        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = link AND 255
        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = link \ 256

        Help_Pos = Help_Pos + 1
    NEXT
END SUB

SUB Help_NewLine 'Start a new help line, apply indention (if any)
    IF Help_Pos > help_w THEN help_w = Help_Pos

    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 13
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 128 + (Help_BG_Col * 16)
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 0
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 0

    help_h = help_h + 1
    Help_Line$ = Help_Line$ + MKL$(Help_Txt_Len + 1)
    Help_Wrap_Pos = 0

    IF Help_Underline > 0 THEN
        w = Help_Pos
        Help_Pos = 1
        IF Help_Underline = 2 THEN 'double/single line?
            Help_AddTxt STRING$(w - 1, 205), Help_Col_Section, 0 'อออ
        ELSE
            Help_AddTxt STRING$(w - 1, 196), Help_Col_Section, 0 'ฤฤฤ
        END IF
        Help_Underline = 0 'keep before Help_NewLine (recursion)
        Help_NewLine
    END IF
    Help_Pos = 1

    IF Help_ChkBlank = 0 THEN 'no indention on blank line checks
        IF Help_Center > 0 THEN 'center overrides regular indent
            Help_LIndent$ = ""
            Help_AddTxt SPACE$(ASC(Help_CIndent$, 1)), Help_Col, 0
            Help_CIndent$ = MID$(Help_CIndent$, 2)
        ELSEIF Help_LIndent$ <> "" THEN
            Help_AddTxt Help_LIndent$, 11, 0
        END IF
    END IF
END SUB

SUB Help_CheckFinishLine 'Make sure the current help line is finished
    IF Help_Txt_Len >= 4 THEN
        IF ASC(Help_Txt$, Help_Txt_Len - 2) < 128 THEN Help_NewLine
    END IF
END SUB

SUB Help_CheckBlankLine 'Make sure the last help line is a blank line (implies finish current)
    IF Help_Txt_Len >= 8 THEN
        IF ASC(Help_Txt$, Help_Txt_Len - 2) < 128 THEN Help_NewLine
        IF ASC(Help_Txt$, Help_Txt_Len - 6) < 128 THEN Help_NewLine
    END IF
    Help_ChkBlank = 0
END SUB

SUB Help_CheckRemoveBlankLine 'If the last help line is blank, then remove it
    IF Help_Txt_Len >= 8 THEN
        IF ASC(Help_Txt$, Help_Txt_Len - 2) > 127 THEN
            Help_Txt_Len = Help_Txt_Len - 4
            help_h = help_h - 1
            Help_Line$ = LEFT$(Help_Line$, LEN(Help_Line$) - 4)
        END IF
        FOR i = Help_Txt_Len - 3 TO 1 STEP -4
            IF ASC(Help_Txt$, i) <> 32 THEN
                Help_Txt_Len = i + 3: EXIT FOR
            END IF
        NEXT
        IF ASC(Help_Txt$, Help_Txt_Len - 2) < 128 THEN Help_NewLine
    END IF
END SUB

FUNCTION Help_Col 'Helps to calculate the default color
    col = Help_Col_Normal
    IF Help_Italic THEN col = Help_Col_Italic
    IF Help_Bold THEN col = Help_Col_Bold 'Bold overrides Italic
    IF Help_Heading THEN col = Help_Col_Section 'Heading overrides text styles
    IF Help_LinkTxt THEN 'Link overrides all
        'for better contrast use alternative color in (code)blocks
        IF Help_LockParse = 0 THEN col = Help_Col_Link: ELSE col = Help_Col_Italic
    END IF
    Help_Col = col
END FUNCTION

SUB WikiParse (a$) 'Wiki page interpret

    'Clear info
    help_h = 0: help_w = 0
    Help_Line$ = "": Help_Txt$ = SPACE$(1000000): Help_Txt_Len = 0
    Help_Link$ = "SECT:dummylink" + Help_Link_Sep$: Help_LinkN = 1

    Help_Pos = 1: Help_Wrap_Pos = 0
    Help_Line$ = MKL$(1)
    'Word wrap locks (lock wrapping only, but continue parsing regularly)
    Help_LockWrap = 0
    'Parser locks (neg: soft lock, zero: unlocked, pos: hard lock)
    'hard:  2 = inside code blocks,  1 = inside output blocks
    'soft: -1 = inside text blocks, -2 = inside pre or fixed blocks
    '=> all parser locks also imply a wrapping lock (except text (-1))
    '=> hard: locks almost every parsing except UTF-8 substitution and line breaks
    '=> soft: allows all elements not disrupting the current block, hence only
    '   paragraph creating things are locked (eg. headings, lists, rulers etc.),
    '   but text styles, links and template processing is still possible
    Help_LockParse = 0
    Help_Bold = 0: Help_Italic = 0: Help_LinkTxt = 0: Help_Heading = 0
    Help_Underline = 0
    Help_BG_Col = 0
    Help_Center = 0: Help_CIndent$ = ""
    Help_DList = 0: Help_ChkBlank = 0

    link = 0: elink = 0: ue = 0: uu = 0: cb = 0: nl = 1: hl = 0: ah = 0: dl = 0

    col = Help_Col

    'Syntax Notes:
    '=============
    'everywhere in text
    '------------------
    ' ''' => bold text style
    ' ''  => italic text style
    ' [url text]    => external link to url with text to appear (url ends at 1st found space)
    ' [[page]]      => link to another wikipage
    ' [[page|text]] => link to another wikipage with alternative text to appear
    ' {{templatename|param|param|param}} or simply {{templatename}} => predefined styles
    '---------------------
    'at start of line only
    '---------------------
    ' == or ===   => start a <h2> or <h3> section heading respectively
    ' ----        => create a horizontal ruler
    ' *  or #     => start a dot list item
    ' ** or ##    => start a sub (ie. further indented) dot list item
    ' ;def:desc   => create a full definition/description list (def = bold, desc indented underneath)
    ' :desc       => start a description only (desc indented as in a full def/desc list)
    ' ;* def:desc => combined list, list dot always belongs to description
    ' :* desc     => combined, description only

    'First find and write the page title and last update
    d$ = "Page not yet updated, expect visual glitches.": i = INSTR(a$, "{{QBDLDATE:")
    IF i > 0 THEN
        d$ = "Last updated: " + MID$(a$, i + 11, INSTR(i + 11, a$, "}}") - i - 11)
        i = INSTR(a$, "{{QBDLTIME:")
        IF i > 0 THEN d$ = d$ + ", at " + MID$(a$, i + 11, INSTR(i + 11, a$, "}}") - i - 11)
    ELSEIF INSTR(a$, "{{PageInternalError}}") > 0 THEN
        d$ = "Page not found."
    END IF
    t$ = Help_PageLoaded$: i = INSTR(a$, "{{DISPLAYTITLE:")
    IF i > 0 THEN t$ = MID$(a$, i + 15, INSTR(i + 15, a$, "}}") - i - 15)
    IF LEFT$(t$, 4) = "agp@" THEN
        d$ = "Auto-generated temporary page."
        t$ = MID$(t$, 5)
    END IF
    i = LEN(d$): ii = LEN(t$)
    Help_AddTxt "  ฺ" + STRING$(ii + 2, "ฤ") + "ฟ", 14, 0: Help_NewLine
    Help_AddTxt "  ณ ", 14, 0: Help_AddTxt t$, 9, 0: Help_AddTxt " ณ", 14, 0
    i = Help_ww - i - 2 - Help_Pos: IF i < 2 THEN i = 2
    Help_AddTxt SPACE$(i) + CHR$(4), 14, 0
    IF LEFT$(d$, 4) = "Page" THEN i = 8: ELSE i = 7
    Help_LockWrap = 1: Help_AddTxt " " + d$, i, 0: Help_NewLine: Help_LockWrap = 0
    Help_AddTxt "ฤฤม", 14, 1 '#toc/#top local link anchor
    Help_AddTxt STRING$(ii + 2, "ฤ") + "ม" + STRING$(Help_ww - ii - 6, "ฤ"), 14, 0: Help_NewLine

    'Init prefetch array
    prefetch = 20
    DIM c$(prefetch)
    FOR ii = 1 TO prefetch
        c$(ii) = SPACE$(ii)
    NEXT

    'BEGIN_PARSE_LOOP
    n = LEN(a$)
    i = 1
    DO WHILE i <= n

        'Get next char and fill prefetch array
        c = ASC(a$, i): c$ = CHR$(c)
        FOR i1 = 1 TO prefetch
            ii = i
            FOR i2 = 1 TO i1
                IF ii <= n THEN
                    ASC(c$(i1), i2) = ASC(a$, ii)
                ELSE
                    ASC(c$(i1), i2) = 32
                END IF
                ii = ii + 1
            NEXT
        NEXT

        'Wiki specific code handling (no restrictions)
        s$ = "__NOEDITSECTION__" + CHR$(10): IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDoneKnl
        s$ = "__NOEDITSECTION__": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "__NOTOC__" + CHR$(10): IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDoneKnl
        s$ = "__NOTOC__": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "__TOC__" + CHR$(10): IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDoneKnl
        s$ = "__TOC__": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "<nowiki>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "</nowiki>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "<gallery" 'Wiki gallery (supported for command availability only)
        IF c$(LEN(s$)) = s$ THEN
            i = i + LEN(s$) - 1: nl = 0
            FOR ii = i TO LEN(a$) - 1
                IF MID$(a$, ii, 1) = ">" THEN
                    wla$ = wikiLookAhead$(a$, ii + 1, "</gallery>"): v$ = wla$: nl = 1
                    IF INSTR(MID$(a$, i, ii - i), "48px") = 0 OR INSTR(MID$(a$, i, ii - i), "nolines") = 0 THEN
                        i = ii + LEN(wla$) 'ignore this gallery
                    ELSE
                        wla$ = StrRemove$(wla$, " "): wla$ = StrRemove$(wla$, CHR$(10))
                        wla$ = StrRemove$(wla$, "File:Apix.png") 'alpha pixels image (separator only)
                        wla$ = StrReplace$(wla$, "|'''", "|*"): wla$ = StrReplace$(wla$, "'''", "'' / ")
                        wla$ = StrReplace$(wla$, "File:Qb64.png|*", "'''QB64;''' ''")
                        wla$ = StrReplace$(wla$, "File:Qbpe.png|*", "'''QB64-PE;''' ''")
                        wla$ = StrReplace$(wla$, "File:Win.png|*", "'''Windows;''' ''")
                        wla$ = StrReplace$(wla$, "File:Lnx.png|*", "'''Linux;''' ''")
                        wla$ = StrReplace$(wla$, "File:Osx.png|*", "'''macOS;''' ''")
                        IF INSTR(wla$, ":") > 0 THEN
                            i = ii + LEN(v$) 'although gallery parameters match, at least
                            EXIT FOR '       'one image does not, so ignore this gallery
                        END IF
                        wla$ = StrReplace$(wla$, ";", ":")
                        wla$ = StrReplace$(wla$, "''none''", "''no versions''")
                        wla$ = StrReplace$(wla$, "''all''", "''all versions''")
                        wla$ = "* " + LEFT$(wla$, LEN(wla$) - 3) + MKI$(&H0A0A)
                        a$ = LEFT$(a$, ii) + wla$ + MID$(a$, ii + LEN(v$) + 1)
                        n = LEN(a$): i = ii
                    END IF
                    EXIT FOR
                END IF
            NEXT
            GOTO charDoneKnl 'keep nl state for next wiki token
        END IF
        s$ = "</gallery>" + CHR$(10): IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDoneKnl
        s$ = "</gallery>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone

        'Direct HTML code is not handled in Code/Output blocks (hard lock), as all text
        'could be part of the code example itself (just imagine a HTML parser/writer demo)
        IF Help_LockParse <= 0 THEN
            s$ = "<center>" 'centered section
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                wla$ = wikiLookAhead$(a$, i + 1, "</center>")
                Help_Center = 1: Help_CIndent$ = wikiBuildCIndent$(wla$)
                Help_AddTxt SPACE$(ASC(Help_CIndent$, 1)), col, 0 'center content
                Help_CIndent$ = MID$(Help_CIndent$, 2)
                GOTO charDone
            END IF
            s$ = "</center>"
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                Help_Center = 0
                Help_NewLine
                GOTO charDone
            END IF
            s$ = "<p style=" 'custom paragraph (maybe centered)
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                FOR ii = i TO LEN(a$) - 1
                    IF MID$(a$, ii, 1) = ">" THEN
                        wla$ = wikiLookAhead$(a$, ii + 1, "</p>")
                        IF INSTR(MID$(a$, i, ii - i), "center") > 0 THEN
                            Help_Center = 1: Help_CIndent$ = wikiBuildCIndent$(wla$)
                            Help_AddTxt SPACE$(ASC(Help_CIndent$, 1)), col, 0 'center (if in style)
                            Help_CIndent$ = MID$(Help_CIndent$, 2)
                            i = ii
                        END IF
                        EXIT FOR
                    END IF
                NEXT
                GOTO charDone
            END IF
            s$ = "</p>"
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                Help_Center = 0
                Help_NewLine
                GOTO charDone
            END IF

            s$ = "<!--" 'ignore HTML comments
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                FOR ii = i TO LEN(a$) - 1
                    IF MID$(a$, ii, 4) = "-->" + CHR$(10) THEN i = ii + 3: GOTO charDoneKnl
                    IF MID$(a$, ii, 3) = "-->" THEN i = ii + 2: GOTO charDone
                NEXT
                GOTO charDone
            END IF
        END IF

        'Wiki text styles are not handled in Code/Output blocks (hard lock),
        'as they could be part of the code example itself
        IF Help_LockParse <= 0 THEN
            'Bold style
            IF c$(3) = "'''" THEN
                i = i + 2
                IF Help_Bold = 0 THEN Help_Bold = 1 ELSE Help_Bold = 0
                col = Help_Col
                GOTO charDone
            END IF
            'Italic style
            IF c$(2) = "''" THEN
                i = i + 1
                IF Help_Italic = 0 THEN Help_Italic = 1 ELSE Help_Italic = 0
                col = Help_Col
                GOTO charDone
            END IF
        END IF

        'Wiki links ([ext], [[int]]) are not handled in Code/Output blocks (hard lock),
        'as all text could be part of the code example itself
        IF Help_LockParse <= 0 THEN
            'External links
            IF c$(5) = "[http" AND elink = 0 THEN
                elink = 1: elink$ = "": elcol$ = ""
                Help_LinkTxt = 1: col = Help_Col
                GOTO charDone
            END IF
            IF elink = 1 THEN
                IF c$ = "]" THEN
                    elink = 0: Help_LinkTxt = 0: col = Help_Col
                    etext$ = elink$
                    i2 = INSTR(elink$, " ")
                    IF i2 > 0 THEN
                        etext$ = MID$(elink$, i2 + 1) 'text part
                        elcol$ = MID$(elcol$, i2 + 1) 'text color part
                        elink$ = LEFT$(elink$, i2 - 1) 'link part
                    END IF

                    Help_LinkN = Help_LinkN + 1
                    Help_Link$ = Help_Link$ + "EXTL:" + elink$ + Help_Link_Sep$

                    FOR j = 1 TO LEN(etext$)
                        Help_AddTxt CHR$(ASC(etext$, j)), ASC(elcol$, j), Help_LinkN
                    NEXT
                    GOTO charDone
                END IF
                GOTO chkEntUtf
            END IF
            'Internal links
            IF c$(2) = "[[" AND link = 0 THEN
                i = i + 1
                link = 1: link$ = "": lcol$ = ""
                Help_LinkTxt = 1: col = Help_Col
                GOTO charDone
            END IF
        END IF
        'However, the internal link logic must run always, as it also handles
        'the template {{Cb|, {{Cl| and {{Cm| links used in text/code blocks
        IF link = 1 THEN
            IF c$(2) = "]]" OR c$(2) = "}}" THEN
                i = i + 1
                link = 0: Help_LinkTxt = 0: col = Help_Col
                text$ = link$
                i2 = INSTR(link$, "|") 'pipe link?
                IF i2 > 0 THEN
                    text$ = MID$(link$, i2 + 1) 'text part
                    lcol$ = MID$(lcol$, i2 + 1) 'text color part
                    link$ = LEFT$(link$, i2 - 1) 'link part
                END IF
                i2 = INSTR(link$, "#") 'local link?
                IF i2 > 0 THEN
                    IF text$ = link$ THEN 'no alternate text for local link?
                        text$ = MID$(link$, i2 + 1) 'use anchor part
                        lcol$ = MID$(lcol$, i2 + 1) 'and respective color part
                    END IF
                    IF MID$(link$, i2 + 1, 3) = "toc" THEN MID$(link$, i2 + 1, 3) = "ฤฤม" 'redirect #toc to page head
                    IF MID$(link$, i2 + 1, 3) = "top" THEN MID$(link$, i2 + 1, 3) = "ฤฤม" 'redirect #top to page head
                    IF LEFT$(link$, 1) = "#" THEN link$ = Help_PageLoaded$ + link$ 'add current page if missing
                END IF
                IF LEFT$(link$, 9) = "Category:" THEN 'ignore category links
                    Help_CheckRemoveBlankLine
                    GOTO charDone
                END IF

                Help_LinkN = Help_LinkN + 1
                IF LEFT$(link$, 10) = "Wikipedia:" THEN 'expand Wikipedia as external links
                    Help_Link$ = Help_Link$ + "EXTL:https://en.wikipedia.org/wiki/" + MID$(link$, 11) + Help_Link_Sep$
                ELSE '                                  'else as internal help page link
                    Help_Link$ = Help_Link$ + "PAGE:" + link$ + Help_Link_Sep$
                END IF

                FOR j = 1 TO LEN(text$)
                    Help_AddTxt CHR$(ASC(text$, j)), ASC(lcol$, j), Help_LinkN
                NEXT
                GOTO charDone
            END IF
            GOTO chkEntUtf
        END IF

        'Wiki tables ({|...|}) are not handled in Code/Output blocks (hard lock),
        'as everything could be part of the code example itself
        IF Help_LockParse <= 0 THEN
            'Tables (ignored, give info, if not in blocks)
            IF c$(2) = "{|" THEN
                wla$ = wikiLookAhead$(a$, i + 2, "|}"): iii = 0
                FOR ii = 1 TO LEN(wla$)
                    IF MID$(wla$, ii, 1) = "|" AND MID$(wla$, ii, 2) <> "|-" THEN iii = iii + 1
                NEXT
                i = i + 1 + LEN(wla$) + 2
                IF iii > 1 OR INSTR(wla$, "__TOC__") = 0 THEN 'ignore TOC only tables
                    IF Help_LockParse = 0 THEN
                        Help_LinkN = Help_LinkN + 1
                        Help_Link$ = Help_Link$ + "EXTL:" + wikiBaseAddress$ + "/index.php?title=" + Help_PageLoaded$ + Help_Link_Sep$
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "ษออออออออออออออออออออออออออออออออออออออป", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "บ", 8, 0: Help_AddTxt " The original page has a table here,  ", 15, Help_LinkN: Help_AddTxt "บ", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "บ", 8, 0: Help_AddTxt " please click inside this box to load ", 15, Help_LinkN: Help_AddTxt "บ", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "บ", 8, 0: Help_AddTxt " the page into your standard browser. ", 15, Help_LinkN: Help_AddTxt "บ", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "ศออออออออออออออออออออออออออออออออออออออผ", 8, 0
                    END IF
                END IF
                GOTO charDone
            END IF
        END IF

        'Wiki templates are handled always, as these are the basic building blocks of all
        'the wiki pages, but look for special conditions inside (Help_LockParse checks)
        IF c$(5) = "{{Cb|" OR c$(5) = "{{Cl|" OR c$(5) = "{{Cm|" THEN 'just nice wrapped links
            i = i + 4
            link = 1: link$ = "": lcol$ = ""
            Help_LinkTxt = 1: col = Help_Col
            GOTO charDone
        END IF
        IF c$(2) = "{{" THEN 'any other templates
            i = i + 1
            cb = 1
            cb$ = ""
            GOTO charDone
        END IF
        IF cb > 0 THEN
            IF c$ = "|" OR c$(2) = "}}" THEN
                IF c$ = "|" AND cb = 2 THEN
                    wla$ = wikiLookAhead$(a$, i + 1, "}}")
                    cb = 0: i = i + LEN(wla$) + 2 'after 1st, ignore all further template parameters
                ELSEIF c$(2) = "}}" THEN
                    IF cb$ = "Parameter" THEN
                        Help_Italic = 0: col = Help_Col
                    ELSEIF LEFT$(cb$, 5) = "Small" THEN
                        IF ASC(cb$, 6) = 196 THEN
                            Help_AddTxt " " + STRING$(Help_ww - Help_Pos, 196), 15, 0
                            Help_BG_Col = 0: col = Help_Col
                        ELSE
                            Help_Center = 0
                        END IF
                        Help_NewLine: cb$ = "" 'avoid reactivation below
                    END IF
                    cb = 0: i = i + 1
                END IF
                IF c$ = "|" AND cb = 1 THEN cb = 2

                IF Help_LockParse = 0 THEN 'no section headings in blocks
                    cbo$ = ""
                    'Standard section headings (section color, double underline)
                    IF cb$ = "PageSyntax" THEN cbo$ = "Syntax:"
                    IF cb$ = "PageParameters" THEN cbo$ = "Parameters:"
                    IF cb$ = "PageDescription" THEN cbo$ = "Description:"
                    IF cb$ = "PageAvailability" THEN cbo$ = "Availability:"
                    IF cb$ = "PageExamples" THEN cbo$ = "Examples:"
                    IF cb$ = "PageSeeAlso" THEN cbo$ = "See also:"
                    'Internally used templates (not available in Wiki)
                    IF cb$ = "PageInternalError" THEN cbo$ = "Sorry, an error occurred:"
                    '----------
                    IF cbo$ <> "" THEN
                        IF RIGHT$(cbo$, 1) = ":" THEN Help_Underline = 2: ELSE Help_Underline = 1
                        Help_AddTxt cbo$, Help_Col_Section, 1: ah = 2
                    END IF
                END IF

                'Code Block
                IF cb$ = "InlineCode" AND Help_LockParse = 0 THEN
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_BG_Col = 1: Help_LockParse = 2
                END IF
                IF cb$ = "InlineCodeEnd" AND Help_LockParse <> 0 THEN
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                IF cb$ = "CodeStart" AND Help_LockParse = 0 THEN
                    Help_CheckBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_BG_Col = 1: Help_LockParse = 2
                    Help_AddTxt STRING$(Help_ww - 15, 196) + " Code Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF cb$ = "CodeEnd" AND Help_LockParse <> 0 THEN
                    Help_CheckFinishLine: Help_CheckRemoveBlankLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                'Output Block
                IF LEFT$(cb$, 11) = "OutputStart" AND Help_LockParse = 0 THEN 'does also match new OutputStartBGn templates
                    Help_CheckBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_BG_Col = 2: Help_LockParse = 1
                    Help_AddTxt STRING$(Help_ww - 17, 196) + " Output Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF cb$ = "OutputEnd" AND Help_LockParse <> 0 THEN
                    Help_CheckFinishLine: Help_CheckRemoveBlankLine
                    Help_AddTxt STRING$((Help_ww - 54) \ 2, 196), 15, 0
                    Help_AddTxt " This block does not reflect the actual output colors ", 15, 0
                    Help_AddTxt STRING$(Help_ww - Help_Pos + 1, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                'Text Block
                IF cb$ = "TextStart" AND Help_LockParse = 0 THEN
                    Help_CheckBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_BG_Col = 6: Help_LockParse = -1
                    Help_AddTxt STRING$(Help_ww - 15, 196) + " Text Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF cb$ = "TextEnd" AND Help_LockParse <> 0 THEN
                    Help_CheckFinishLine: Help_CheckRemoveBlankLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                'Pre Block
                IF cb$ = "PreStart" AND Help_LockParse = 0 THEN
                    Help_CheckRemoveBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_LIndent$ = "  ": Help_LockParse = -2
                    Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF cb$ = "PreEnd" AND Help_LockParse <> 0 THEN
                    Help_LIndent$ = ""
                    Help_CheckRemoveBlankLine
                    Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                'Fixed Block
                IF cb$ = "FixedStart" AND Help_LockParse = 0 THEN
                    Help_CheckBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_BG_Col = 6: Help_LockParse = -2
                    Help_AddTxt STRING$(Help_ww - 16, 196) + " Fixed Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF cb$ = "FixedEnd" AND Help_LockParse <> 0 THEN
                    Help_CheckFinishLine: Help_CheckRemoveBlankLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF

                'Template wrapped plugin
                IF (cb$ = "PageNavigation" OR cb$ = "PageReferences" OR RIGHT$(cb$, 6) = "Plugin") AND Help_LockParse = 0 THEN 'no plugins in blocks
                    pit$ = Wiki$("Template:" + cb$)
                    IF INSTR(pit$, "{{PageInternalError}}") = 0 THEN
                        a$ = LEFT$(a$, i) + pit$ + RIGHT$(a$, LEN(a$) - i)
                        n = n + LEN(pit$)
                    END IF
                END IF

                'Parameter template text will be italic
                IF c$ = "|" AND cb$ = "Parameter" AND Help_LockParse <= 0 THEN 'keep as is in Code/Output blocks
                    Help_Italic = 1: col = Help_Col
                END IF

                'Small template text will be centered (maybe as block note)
                IF cb$ = "Small" AND Help_LockParse <= 0 THEN 'keep as is in Code/Output blocks
                    wla$ = wikiLookAhead$(a$, i + 1, "}}")
                    Help_CIndent$ = wikiBuildCIndent$(wla$): iii = 0
                    IF i > 31 AND ASC(Help_CIndent$, 1) >= Help_ww / 4 THEN
                        IF INSTR(MID$(a$, i - 30, 30), "{{CodeEnd}}") > 0 THEN iii = -1
                        IF INSTR(MID$(a$, i - 30, 30), "{{TextEnd}}") > 0 THEN iii = -6
                        IF INSTR(MID$(a$, i - 31, 31), "{{FixedEnd}}") > 0 THEN iii = -6
                    END IF
                    IF iii <> 0 THEN
                        FOR ii = Help_Txt_Len - 3 TO 1 STEP -4
                            IF ASC(Help_Txt$, ii) = 32 AND iii < 0 THEN
                                Help_Pos = Help_Pos - 1
                            ELSEIF ASC(Help_Txt$, ii + 1) > 127 AND iii < 0 THEN
                                help_h = help_h - 1: Help_Line$ = LEFT$(Help_Line$, LEN(Help_Line$) - 4)
                            ELSEIF ASC(Help_Txt$, ii) = 196 AND iii < 0 THEN
                                iii = -iii
                            ELSEIF ASC(Help_Txt$, ii + 1) > 127 AND iii > 0 THEN
                                Help_Txt_Len = ii + 3: EXIT FOR
                            END IF
                        NEXT
                        Help_BG_Col = iii: cb$ = cb$ + CHR$(196) 'special signal byte
                        Help_AddTxt STRING$(ASC(Help_CIndent$, 1) - 1, 196) + " ", 15, 0
                        col = 15 'further text color until closing
                    ELSE
                        Help_Center = 1: cb$ = cb$ + CHR$(0) 'no special signal
                        Help_AddTxt SPACE$(ASC(Help_CIndent$, 1)), col, 0 'center content
                    END IF
                    Help_CIndent$ = MID$(Help_CIndent$, 2)
                END IF

                GOTO charDone
            END IF

            IF cb = 1 THEN cb$ = cb$ + c$ 'reading template name
            IF cb = 2 GOTO chkEntUtf 'copy text with proper Entity/UTF-8 substitution
            GOTO charDone
        END IF

        'Wiki headings (==...==}) are not handled in blocks (soft- and hard lock), as it would
        'disrupt the block, also in code blocks it could be part of the code example itself
        IF Help_LockParse = 0 THEN
            'Custom section headings (section color, h4 no underline, h3 single underline, h2 double underline)
            ii = 0
            IF c$(5) = " ====" AND Help_Heading = 4 THEN ii = 4: Help_Heading = 0: hl = 0: ah = 2
            IF c$(4) = "====" AND Help_Heading = 4 THEN ii = 3: Help_Heading = 0: hl = 0: ah = 2
            IF c$(4) = "====" AND nl = 1 THEN ii = 3: Help_CheckBlankLine: Help_Heading = 4: hl = 1
            IF c$(5) = "==== " AND nl = 1 THEN ii = 4: Help_CheckBlankLine: Help_Heading = 4: hl = 1
            IF ii > 0 THEN i = i + ii: col = Help_Col: Help_Underline = 0: GOTO charDone
            ii = 0
            IF c$(4) = " ===" AND Help_Heading = 3 THEN ii = 3: Help_Heading = 0: hl = 0: ah = 2
            IF c$(3) = "===" AND Help_Heading = 3 THEN ii = 2: Help_Heading = 0: hl = 0: ah = 2
            IF c$(3) = "===" AND nl = 1 THEN ii = 2: Help_CheckBlankLine: Help_Heading = 3: hl = 1
            IF c$(4) = "=== " AND nl = 1 THEN ii = 3: Help_CheckBlankLine: Help_Heading = 3: hl = 1
            IF ii > 0 THEN i = i + ii: col = Help_Col: Help_Underline = 1: GOTO charDone
            ii = 0
            IF c$(3) = " ==" AND Help_Heading = 2 THEN ii = 2: Help_Heading = 0: hl = 0: ah = 2
            IF c$(2) = "==" AND Help_Heading = 2 THEN ii = 1: Help_Heading = 0: hl = 0: ah = 2
            IF c$(2) = "==" AND nl = 1 THEN ii = 1: Help_CheckBlankLine: Help_Heading = 2: hl = 1
            IF c$(3) = "== " AND nl = 1 THEN ii = 2: Help_CheckBlankLine: Help_Heading = 2: hl = 1
            IF ii > 0 THEN i = i + ii: col = Help_Col: Help_Underline = 2: GOTO charDone
        END IF

        'Wiki/HTML rulers (----, <hr>) are not handled in blocks (soft- and hard lock), as it would
        'disrupt the block, also in code blocks it could be part of the code example itself
        IF Help_LockParse = 0 THEN
            'Rulers
            IF c$(4) = "----" AND nl = 1 THEN
                i = i + 3
                IF Help_ChkBlank = -1 THEN Help_ChkBlank = 0: ELSE Help_CheckBlankLine
                Help_AddTxt STRING$(Help_ww, 196), 14, 0
                Help_ChkBlank = -1
                GOTO charDone
            END IF
            IF c$(4) = "<hr>" OR c$(6) = "<hr />" THEN
                IF c$(4) = "<hr>" THEN i = i + 3
                IF c$(6) = "<hr />" THEN i = i + 5
                IF Help_ChkBlank = -1 THEN Help_ChkBlank = 0: ELSE Help_CheckBlankLine
                Help_AddTxt STRING$(Help_ww, 196), 14, 0
                Help_ChkBlank = -1
                GOTO charDone
            END IF
        END IF

        'Wiki definition lists (;...:...) are not handled in blocks (soft- and hard lock), as it would
        'disrupt the block, also in code blocks it could be part of the code example itself
        IF Help_LockParse = 0 THEN
            'Definition lists
            IF c$ = ";" AND nl = 1 THEN 'definition (new line only)
                IF c$(2) = "; " THEN i = i + 1
                IF ah = 0 AND dl = 0 THEN Help_CheckBlankLine
                Help_Bold = 1: col = Help_Col: Help_DList = 1
                IF c$(2) = ";*" OR c$(2) = ";#" THEN i = i + 1: Help_DList = 2 'list dot belongs to description
                IF c$(3) = ";* " OR c$(3) = ";# " THEN i = i + 1: Help_DList = 3 'list dot belongs to description
                IF dl < 3 THEN
                    Help_AddTxt "Þ ", 11, 0: dl = 1
                    Help_LIndent$ = Help_LIndent$ + "Þ "
                END IF
                GOTO charDone
            END IF
            IF c$ = ":" AND Help_DList > 0 THEN 'description (same line)
                IF c$(2) = ": " THEN i = i + 1
                Help_Bold = 0: col = Help_Col: Help_NewLine
                Help_AddTxt " ", 11, 0: dl = 3
                Help_LIndent$ = Help_LIndent$ + " "
                IF Help_DList > 1 THEN
                    Help_AddTxt CHR$(4) + " ", 14, 0
                    Help_LIndent$ = Help_LIndent$ + "  "
                END IF
                Help_DList = 0
                GOTO charDone
            END IF
            IF c$ = ":" AND nl = 1 THEN 'description w/o definition (new line)
                IF c$(2) = ": " THEN i = i + 1
                IF ah = 0 AND dl = 0 THEN Help_CheckBlankLine
                IF dl < 3 THEN
                    Help_AddTxt "Þ ", 11, 0: dl = 2
                    Help_LIndent$ = Help_LIndent$ + "Þ "
                END IF
                IF ASC(c$(2), 2) <> 58 AND ASC(c$(2), 2) <> 59 THEN
                    Help_AddTxt " ", 11, 0: dl = 3
                    Help_LIndent$ = Help_LIndent$ + " "
                END IF
                GOTO charDoneKnl 'keep nl state for possible <UL> list bullets
            END IF
        END IF

        'Wiki lists (*, **) are not handled in blocks (soft- and hard lock), as it would
        'disrupt the block, also in code blocks it could be part of the code example itself
        IF Help_LockParse = 0 THEN
            'Unordered/Ordered lists
            IF nl = 1 THEN
                IF c$(2) = "**" OR c$(2) = "##" THEN
                    IF c$(3) = "** " OR c$(3) = "## " THEN i = i + 2: ELSE i = i + 1
                    Help_AddTxt "   " + CHR$(4) + " ", 14, 0
                    Help_LIndent$ = Help_LIndent$ + "     "
                    GOTO charDone
                END IF
                IF c$ = "*" OR c$ = "#" THEN
                    IF c$(2) = "* " OR c$(2) = "# " THEN i = i + 1
                    Help_AddTxt CHR$(4) + " ", 14, 0
                    Help_LIndent$ = Help_LIndent$ + "  "
                    GOTO charDone
                END IF
            END IF
        END IF

        'Entities are not handled in Code/Output blocks (hard lock), as all text could
        'be part of the code example itself (just imagine a HTML parser/writer demo)
        chkEntUtf:
        ocol = col 'save original current color for later reset
        IF Help_LockParse <= 0 THEN
            IF c$ = "&" THEN 'possible entity
                FOR ii = 0 TO wpEntReplCnt
                    ent$ = RTRIM$(wpEntRepl(ii).enti)
                    IF c$(LEN(ent$)) = ent$ THEN
                        c$ = RTRIM$(wpEntRepl(ii).repl)
                        i = i + LEN(ent$) - 1: GOTO charAccum
                    END IF
                NEXT
                IF Help_LockParse = 0 THEN 'take as is in other blocks (skip unknown check)
                    ii = INSTR(c$(8), ";"): iii = INSTR(c$(8), " ") 'unknown entity?
                    IF ii > 2 AND (iii = 0 OR iii > ii) THEN
                        c$ = c$(ii): col = 8: ue = -1
                        i = i + ii - 1: GOTO charAccum
                    END IF
                END IF
            END IF
        END IF

        'UTF-8 handling (no restrictions)
        IF ((c AND &HE0~%%) = 192) AND ((ASC(c$(2), 2) AND &HC0~%%) = 128) THEN '2-byte UTF-8
            i = i + 1
            FOR ii = 0 TO wpUtfReplCnt
                IF wpUtfRepl(ii).utf8 = c$(2) + MKI$(&H2020) THEN
                    c$ = RTRIM$(wpUtfRepl(ii).repl): GOTO charAccum
                END IF
            NEXT
            c$ = CHR$(168): col = 8: uu = -1: GOTO charAccum
        END IF
        IF ((c AND &HF0~%%) = 224) AND ((ASC(c$(2), 2) AND &HC0~%%) = 128) AND ((ASC(c$(3), 3) AND &HC0~%%) = 128) THEN '3-byte UTF-8
            i = i + 2
            FOR ii = 0 TO wpUtfReplCnt
                IF wpUtfRepl(ii).utf8 = c$(3) + CHR$(0) THEN
                    c$ = RTRIM$(wpUtfRepl(ii).repl): GOTO charAccum
                END IF
            NEXT
            c$ = CHR$(168): col = 8: uu = -1: GOTO charAccum
        END IF
        IF ((c AND &HF8~%%) = 240) AND ((ASC(c$(2), 2) AND &HC0~%%) = 128) AND ((ASC(c$(3), 3) AND &HC0~%%) = 128) AND ((ASC(c$(4), 4) AND &HC0~%%) = 128) THEN '4-byte UTF-8
            i = i + 3
            FOR ii = 0 TO wpUtfReplCnt
                IF wpUtfRepl(ii).utf8 = c$(4) THEN
                    c$ = RTRIM$(wpUtfRepl(ii).repl): GOTO charAccum
                END IF
            NEXT
            c$ = CHR$(168): col = 8: uu = -1: GOTO charAccum
        END IF

        'Line break handling (no restrictions)
        IF c = 10 OR c$(4) = "<br>" OR c$(6) = "<br />" THEN
            IF c$(4) = "<br>" THEN i = i + 3: IF ASC(c$(5), 5) = 10 THEN i = i + 1
            IF c$(6) = "<br />" THEN i = i + 5: IF ASC(c$(7), 7) = 10 THEN i = i + 1
            IF c = 10 THEN 'on real new line only
                IF dl > 1 THEN dl = dl - 1 'update def list state
                IF Help_LockParse = 0 THEN Help_LIndent$ = "" 'end indention outside blocks
            END IF

            IF Help_LockParse > -2 THEN 'everywhere except in fixed blocks
                IF Help_Txt_Len >= 8 THEN 'allow max. one blank line (ie. collapse multi blanks to just one)
                    IF ASC(Help_Txt$, Help_Txt_Len - 2) > 127 AND ASC(Help_Txt$, Help_Txt_Len - 6) > 127 THEN
                        IF Help_Center > 0 THEN Help_CIndent$ = MID$(Help_CIndent$, 2) 'drop respective center indent
                        GOTO skipMultiBlanks
                    END IF
                END IF
            END IF
            Help_NewLine

            skipMultiBlanks:
            IF Help_LockParse <> 0 THEN 'in all blocks reset styles at EOL
                Help_Bold = 0: Help_Italic = 0: col = Help_Col
            ELSE
                IF c = 10 THEN 'on real new line only
                    Help_DList = 0: Help_Bold = 0: col = Help_Col 'def list incl. style ends
                    IF ah > 0 THEN ah = ah - 1 'update after heading state
                    IF dl > 0 THEN
                        IF ASC(c$(2), 2) <> 59 AND ASC(c$(2), 2) <> 58 THEN
                            dl = 0 'end of def list indention
                            Help_ChkBlank = 1
                        END IF
                    END IF
                END IF
            END IF
            nl = 1
            GOTO charDoneKnl 'keep just set nl state
        END IF

        charAccum: 'accumulate char(s) in the correct channel
        IF elink = 1 THEN
            elink$ = elink$ + c$
            FOR j = 1 TO LEN(c$): elcol$ = elcol$ + CHR$(col): NEXT
        ELSEIF link = 1 THEN
            link$ = link$ + c$
            FOR j = 1 TO LEN(c$): lcol$ = lcol$ + CHR$(col): NEXT
        ELSE
            Help_AddTxt c$, col, hl
        END IF
        col = ocol 'reset signal color (Entity/UTF-8 check) to original current color
        charDone:
        nl = 0
        charDoneKnl: 'done, but keep nl state
        i = i + 1
    LOOP
    'END_PARSE_LOOP

    'Write and rearrange missing Entity & UTF-8 warnings (if any)
    IF ue OR uu THEN
        Help_LinkN = Help_LinkN + 1
        Help_Link$ = Help_Link$ + "EXTL:https://qb64phoenix.com/forum/forumdisplay.php?fid=25" + Help_Link_Sep$
        stp = CVL(RIGHT$(Help_Line$, 4))
        Help_AddTxt STRING$(Help_ww, 196), 14, 0: Help_NewLine
        itp = CVL(MID$(Help_Line$, 13, 4)): dtl = CVL(RIGHT$(Help_Line$, 4)) - stp
        txt$ = MID$(Help_Txt$, stp, dtl) + MID$(Help_Txt$, itp, stp - itp): MID$(Help_Txt$, itp, LEN(txt$)) = txt$
        Help_Line$ = LEFT$(Help_Line$, 12) + MKL$(itp) + MID$(Help_Line$, 13, LEN(Help_Line$) - 16)
        FOR i = 17 TO LEN(Help_Line$) STEP 4: MID$(Help_Line$, i, 4) = MKL$(CVL(MID$(Help_Line$, i, 4)) + dtl): NEXT
        IF uu THEN
            stp = CVL(RIGHT$(Help_Line$, 4))
            Help_AddTxt "!>", 4, 0
            Help_AddTxt " Page uses ", Help_Col_Normal, 0
            Help_AddTxt "unknown UTF-8 characters", 8, 0
            Help_AddTxt ", please report it in the ", Help_Col_Normal, 0
            Help_AddTxt "Wiki Forum.", Help_Col_Link, Help_LinkN: Help_NewLine
            itp = CVL(MID$(Help_Line$, 13, 4)): dtl = CVL(RIGHT$(Help_Line$, 4)) - stp
            txt$ = MID$(Help_Txt$, stp, dtl) + MID$(Help_Txt$, itp, stp - itp): MID$(Help_Txt$, itp, LEN(txt$)) = txt$
            Help_Line$ = LEFT$(Help_Line$, 12) + MKL$(itp) + MID$(Help_Line$, 13, LEN(Help_Line$) - 16)
            FOR i = 17 TO LEN(Help_Line$) STEP 4: MID$(Help_Line$, i, 4) = MKL$(CVL(MID$(Help_Line$, i, 4)) + dtl): NEXT
        END IF
        IF ue THEN
            stp = CVL(RIGHT$(Help_Line$, 4))
            Help_AddTxt "!>", 4, 0
            Help_AddTxt " Page uses ", Help_Col_Normal, 0
            Help_AddTxt "unknown HTML entities", 8, 0
            Help_AddTxt ", please report it in the ", Help_Col_Normal, 0
            Help_AddTxt "Wiki Forum.", Help_Col_Link, Help_LinkN: Help_NewLine
            itp = CVL(MID$(Help_Line$, 13, 4)): dtl = CVL(RIGHT$(Help_Line$, 4)) - stp
            txt$ = MID$(Help_Txt$, stp, dtl) + MID$(Help_Txt$, itp, stp - itp): MID$(Help_Txt$, itp, LEN(txt$)) = txt$
            Help_Line$ = LEFT$(Help_Line$, 12) + MKL$(itp) + MID$(Help_Line$, 13, LEN(Help_Line$) - 16)
            FOR i = 17 TO LEN(Help_Line$) STEP 4: MID$(Help_Line$, i, 4) = MKL$(CVL(MID$(Help_Line$, i, 4)) + dtl): NEXT
        END IF
    END IF
    'Finish and Trim Help_Txt$
    Help_CheckFinishLine: Help_Txt$ = LEFT$(Help_Txt$, Help_Txt_Len)

    IF Help_PageLoaded$ = "Keyword Reference - Alphabetical" THEN

        fh = FREEFILE
        OPEN "internal\help\links.bin" FOR OUTPUT AS #fh
        a$ = SPACE$(1000)
        FOR cy = 1 TO help_h
            'isolate and REVERSE select link
            l = CVL(MID$(Help_Line$, (cy - 1) * 4 + 1, 4))
            x = l
            x2 = 1
            c = ASC(Help_Txt$, x)
            oldlnk = 0
            lnkx1 = 0: lnkx2 = 0
            DO UNTIL ASC(Help_Txt$, x + 1) > 127
                ASC(a$, x2) = c
                lnk = CVI(MID$(Help_Txt$, x + 2, 2))
                IF oldlnk = 0 AND lnk <> 0 THEN lnkx1 = x2
                IF (lnk = 0 OR ASC(Help_Txt$, x + 5) > 127) AND lnkx1 <> 0 THEN
                    lnkx2 = x2: IF lnk = 0 THEN lnkx2 = lnkx2 - 1

                    IF lnkx1 <> 3 THEN GOTO ignorelink
                    IF ASC(a$, 1) <> 4 THEN GOTO ignorelink

                    'retrieve lnk info
                    lnk2 = lnk: IF lnk2 = 0 THEN lnk2 = oldlnk
                    l1 = 1
                    FOR lx = 1 TO lnk2 - 1
                        l1 = INSTR(l1, Help_Link$, Help_Link_Sep$) + 1
                    NEXT
                    l2 = INSTR(l1, Help_Link$, Help_Link_Sep$) - 1
                    l$ = MID$(Help_Link$, l1, l2 - l1 + 1)
                    'assume PAGE
                    l$ = RIGHT$(l$, LEN(l$) - 5)

                    a2$ = MID$(a$, lnkx1, lnkx2 - lnkx1 + 1)

                    IF INSTR(a2$, "(") THEN a2$ = LEFT$(a2$, INSTR(a2$, "(") - 1)
                    IF INSTR(a2$, " ") THEN a2$ = LEFT$(a2$, INSTR(a2$, " ") - 1)
                    IF INSTR(a2$, "...") THEN
                        a3$ = RIGHT$(a2$, LEN(a2$) - INSTR(a2$, "...") - 2)

                        skip = 0

                        IF UCASE$(LEFT$(a3$, 3)) <> "_GL" THEN
                            FOR ci = 1 TO LEN(a3$)
                                ca = ASC(a3$, ci)
                                IF ca >= 97 AND ca <= 122 THEN skip = 1
                                IF ca = 44 THEN skip = 1
                            NEXT
                        END IF

                        IF skip = 0 THEN PRINT #fh, a3$ + "," + l$

                        a2$ = LEFT$(a2$, INSTR(a2$, "...") - 1)
                    END IF

                    skip = 0
                    IF UCASE$(LEFT$(a2$, 3)) <> "_GL" THEN
                        FOR ci = 1 TO LEN(a2$)
                            ca = ASC(a2$, ci)
                            IF ca >= 97 AND ca <= 122 THEN skip = 1
                            IF ca = 44 THEN skip = 1
                        NEXT
                    END IF
                    IF skip = 0 THEN PRINT #fh, a2$ + "," + l$
                    oa2$ = a2$

                    a2$ = l$
                    IF INSTR(a2$, "(") THEN a2$ = LEFT$(a2$, INSTR(a2$, "(") - 1)
                    IF INSTR(a2$, " ") THEN a2$ = LEFT$(a2$, INSTR(a2$, " ") - 1)
                    IF INSTR(a2$, "...") THEN
                        a3$ = RIGHT$(a2$, LEN(a2$) - INSTR(a2$, "...") - 2)

                        skip = 0
                        IF UCASE$(LEFT$(a3$, 3)) <> "_GL" THEN
                            FOR ci = 1 TO LEN(a3$)
                                ca = ASC(a3$, ci)
                                IF ca >= 97 AND ca <= 122 THEN skip = 1
                                IF ca = 44 THEN skip = 1
                            NEXT
                        END IF
                        IF skip = 0 THEN PRINT #fh, a3$ + "," + l$

                        a2$ = LEFT$(a2$, INSTR(a2$, "...") - 1)
                    END IF

                    skip = 0
                    IF UCASE$(LEFT$(a2$, 3)) <> "_GL" THEN
                        FOR ci = 1 TO LEN(a2$)
                            ca = ASC(a2$, ci)
                            IF ca >= 97 AND ca <= 122 THEN skip = 1
                            IF ca = 44 THEN skip = 1
                        NEXT
                    END IF
                    IF skip = 0 AND a2$ <> oa2$ THEN PRINT #fh, a2$ + "," + l$

                    ignorelink:

                    lnkx1 = 0: lnkx2 = 0
                END IF
                x = x + 4: c = ASC(Help_Txt$, x)
                x2 = x2 + 1
                oldlnk = lnk
            LOOP
        NEXT
        CLOSE #fh

    END IF
END SUB

FUNCTION wikiSafeName$ (page$) 'create a unique name for both case sensitive & insensitive systems
    ext$ = SPACE$(LEN(page$))
    FOR i = 1 TO LEN(page$)
        c = ASC(page$, i)
        SELECT CASE c
            CASE 65 TO 90: ASC(ext$, i) = 49 'upper = 1
            CASE 97 TO 122: ASC(ext$, i) = 48 'lower = 0
            CASE ELSE: ASC(ext$, i) = c 'non-letter = take as is
        END SELECT
    NEXT
    wikiSafeName$ = page$ + "_" + ext$
END FUNCTION

FUNCTION wikiDLPage$ (url$, timeout#)
    '--- set default result & avoid side effects ---
    wikiDLPage$ = ""
    wik$ = url$: tio# = timeout#
    '--- request wiki page ---
    retry:
    ch& = _OPENCLIENT(wik$)
    IF Help_Recaching < 2 THEN 'avoid messages for 'qb64pe -u' (build time update)
        IF ch& = 0 AND LCASE$(LEFT$(wik$, 8)) = "https://" THEN
            IF _SHELLHIDE("curl --version >NUL") <> 0 THEN
                'no external curl available (see notes below)
                IF _MESSAGEBOX("QB64-PE Help", "Can't make secure connection (https:) to Wiki, shall the IDE use unsecure (http:) instead?", "yesno", "warning") = 1 THEN
                    IF _MESSAGEBOX("QB64-PE Help", "Do you wanna save your choice permanently for the future?", "yesno", "question") = 1 THEN
                        wikiBaseAddress$ = "http://" + MID$(wikiBaseAddress$, 9)
                        WriteConfigSetting generalSettingsSection$, "WikiBaseAddress", wikiBaseAddress$
                    END IF
                    wik$ = "http://" + MID$(wik$, 9): GOTO retry
                END IF
            END IF
        END IF
    END IF
    IF ch& = 0 GOTO oneLastChance
    '--- read the response ---
    IF _STATUSCODE(ch&) = 200 THEN
        res$ = "": st# = TIMER(0.001)
        DO
            _DELAY 0.05
            GET ch&, , rec$
            IF LEN(rec$) > 0 THEN st# = TIMER(0.001)
            res$ = res$ + rec$
            IF EOF(ch&) THEN
                wikiDLPage$ = res$: EXIT DO
            END IF
            IF st# + tio# >= 86400 THEN st# = st# - 86400
        LOOP UNTIL TIMER(0.001) > st# + tio#
    END IF
    CLOSE ch&
    EXIT FUNCTION
    '--- try external curl ---
    oneLastChance:
    'The external curl tool (if available), together with its local CA
    'bundle is used as a silent fallback option. It's for people on old
    'systems with outdated CA stores. They can use the unsecure http:
    'choice given above for now, but who knows how long http: is still
    'supported by web hosters with today's raising security concerns.
    'Once it isn't supported anymore, those users can then simply drop
    'the external curl & CA bundle into the qb64pe folder as done in
    'former QB64-PE versions, to get secure access again.
    ' However, we shouldn't promote this too much in the release notes
    'and instead only give that information to people who complain about
    'not working Wiki downloads in the Forum/Discord.
    '--- check for curl ---
    IF _SHELLHIDE("curl --version >NUL") = 0 THEN
        '--- 1st restore https: protocol, if changed above ---
        IF LCASE$(LEFT$(wik$, 7)) = "http://" THEN wik$ = "https://" + MID$(wik$, 8)
        '--- issue curl request ---
        responseFile$ = Cache_Folder$ + "/curlResponse.txt"
        SHELL _HIDE "curl --silent -o " + CHR$(34) + responseFile$ + CHR$(34) + " " + CHR$(34) + wik$ + CHR$(34)
        '--- read the response ---
        res$ = _READFILE$(responseFile$)
        KILL responseFile$
        '--- set result ---
        wikiDLPage$ = res$
    END IF
END FUNCTION

FUNCTION wikiLookAhead$ (a$, i, token$) 'Prefetch further wiki text
    wikiLookAhead$ = "": IF i >= LEN(a$) THEN EXIT FUNCTION
    j = INSTR(i, a$, token$)
    IF j = 0 THEN
        wikiLookAhead$ = MID$(a$, i)
    ELSE
        wikiLookAhead$ = MID$(a$, i, j - i)
    END IF
END FUNCTION

FUNCTION wikiBuildCIndent$ (a$) 'Pre-calc center indentions
    wikiBuildCIndent$ = "": IF a$ = "" THEN EXIT FUNCTION

    org$ = a$: b$ = "" 'eliminate internal links
    FOR i = 1 TO LEN(org$)
        IF MID$(org$, i, 2) = "[[" THEN
            FOR ii = i + 2 TO LEN(org$)
                IF MID$(org$, ii, 1) = "|" THEN i = ii + 1: EXIT FOR
                IF MID$(org$, ii, 2) = "]]" THEN i = i + 2: EXIT FOR
            NEXT
        END IF
        IF MID$(org$, i, 2) = "]]" THEN i = i + 2
        b$ = b$ + MID$(org$, i, 1)
    NEXT
    org$ = b$: b$ = "" 'eliminate external links
    FOR i = 1 TO LEN(org$)
        IF MID$(org$, i, 5) = "[http" THEN
            FOR ii = i + 5 TO LEN(org$)
                IF MID$(org$, ii, 1) = " " THEN i = ii + 1: EXIT FOR
                IF MID$(org$, ii, 1) = "]" THEN i = i + 1: EXIT FOR
            NEXT
        END IF
        IF MID$(org$, i, 1) = "]" THEN i = i + 1
        b$ = b$ + MID$(org$, i, 1)
    NEXT
    org$ = b$: b$ = "" 'eliminate templates
    FOR i = 1 TO LEN(org$)
        IF MID$(org$, i, 2) = "{{" THEN
            FOR ii = i + 2 TO LEN(org$)
                IF MID$(org$, ii, 1) = "|" THEN i = ii + 1: EXIT FOR
                IF MID$(org$, ii, 2) = "}}" THEN i = i + 2: EXIT FOR
            NEXT
        END IF
        IF MID$(org$, i, 1) = "|" THEN
            FOR ii = i + 1 TO LEN(org$)
                IF MID$(org$, ii, 2) = "}}" THEN i = ii: EXIT FOR
            NEXT
        END IF
        IF MID$(org$, i, 2) = "}}" THEN i = i + 2
        b$ = b$ + MID$(org$, i, 1)
    NEXT
    org$ = b$: b$ = "" 'eliminate text styles
    FOR i = 1 TO LEN(org$)
        IF MID$(org$, i, 3) = "'''" THEN i = i + 3
        IF MID$(org$, i, 2) = "''" THEN i = i + 2
        b$ = b$ + MID$(org$, i, 1)
    NEXT
    org$ = b$: b$ = "" 'substitute Entities
    FOR i = 1 TO LEN(org$)
        IF MID$(org$, i, 1) = "&" THEN 'possible entity
            FOR ii = 0 TO wpEntReplCnt
                ent$ = RTRIM$(wpEntRepl(ii).enti)
                IF MID$(org$, i, LEN(ent$)) = ent$ THEN
                    b$ = b$ + RTRIM$(wpEntRepl(ii).repl)
                    i = i + LEN(ent$): GOTO charDoneEnt
                END IF
            NEXT
        END IF
        b$ = b$ + MID$(org$, i, 1)
        charDoneEnt:
    NEXT
    org$ = b$: b$ = "" 'substitute UTF-8
    FOR i = 1 TO LEN(org$)
        IF i + 1 <= LEN(org$) THEN
            IF ((ASC(org$, i) AND &HE0~%%) = 192) AND ((ASC(org$, i + 1) AND &HC0~%%) = 128) THEN '2-byte UTF-8
                utf$ = MID$(org$, i, 2) + MKI$(&H2020): i = i + 2
                FOR ii = 0 TO wpUtfReplCnt
                    IF wpUtfRepl(ii).utf8 = utf$ THEN
                        b$ = b$ + RTRIM$(wpUtfRepl(ii).repl): GOTO charDoneUtf
                    END IF
                NEXT
                b$ = b$ + CHR$(168): GOTO charDoneUtf
            END IF
        END IF
        IF i + 2 <= LEN(org$) THEN
            IF ((ASC(org$, i) AND &HF0~%%) = 224) AND ((ASC(org$, i + 1) AND &HC0~%%) = 128) AND ((ASC(org$, i + 2) AND &HC0~%%) = 128) THEN '3-byte UTF-8
                utf$ = MID$(org$, i, 3) + CHR$(0): i = i + 3
                FOR ii = 0 TO wpUtfReplCnt
                    IF wpUtfRepl(ii).utf8 = utf$ THEN
                        b$ = b$ + RTRIM$(wpUtfRepl(ii).repl): GOTO charDoneUtf
                    END IF
                NEXT
                b$ = b$ + CHR$(168): GOTO charDoneUtf
            END IF
        END IF
        IF i + 3 <= LEN(org$) THEN
            IF ((ASC(org$, i) AND &HF8~%%) = 240) AND ((ASC(org$, i + 1) AND &HC0~%%) = 128) AND ((ASC(org$, i + 2) AND &HC0~%%) = 128) AND ((ASC(org$, i + 3) AND &HC0~%%) = 128) THEN '4-byte UTF-8
                utf$ = MID$(org$, i, 4): i = i + 4
                FOR ii = 0 TO wpUtfReplCnt
                    IF wpUtfRepl(ii).utf8 = utf$ THEN
                        b$ = b$ + RTRIM$(wpUtfRepl(ii).repl): GOTO charDoneUtf
                    END IF
                NEXT
                b$ = b$ + CHR$(168): GOTO charDoneUtf
            END IF
        END IF
        b$ = b$ + MID$(org$, i, 1)
        charDoneUtf:
    NEXT
    b$ = StrReplace$(b$, "<br>" + CHR$(10), CHR$(10)) 'convert HTML line breaks
    b$ = StrReplace$(b$, "<br>", CHR$(10))
    b$ = StrReplace$(b$, "<br />" + CHR$(10), CHR$(10)) 'convert XHTML line breaks
    b$ = StrReplace$(b$, "<br />", CHR$(10))
    b$ = _TRIM$(b$) + CHR$(10) 'safety fallback

    i = 1: st = 1: br = 0: res$ = ""
    WHILE i <= LEN(b$)
        ws = INSTR(i, b$, " "): lb = INSTR(i, b$, CHR$(10))
        IF lb > 0 AND (ws > lb OR lb - st <= Help_ww) THEN SWAP ws, lb
        IF ws > 0 AND ws - st <= Help_ww THEN
            br = ws: i = ws + 1
            IF ASC(b$, ws) <> 10 AND i <= LEN(b$) THEN _CONTINUE
        END IF
        IF br = 0 THEN
            IF lb < ws THEN
                br = lb
            ELSE
                IF ws > 0 THEN br = ws: ELSE br = lb
            END IF
        END IF
        ci = (Help_ww - (br - st)) \ 2: IF ci < 0 THEN ci = 0
        res$ = res$ + CHR$(ci)
        i = br + 1: st = br + 1: br = 0
    WEND
    wikiBuildCIndent$ = res$
END FUNCTION

