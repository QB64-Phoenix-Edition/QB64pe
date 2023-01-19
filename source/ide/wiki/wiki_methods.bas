FUNCTION Back2BackName$ (a$)
    IF a$ = "Keyword Reference - Alphabetical" THEN Back2BackName$ = "Alphabetical": EXIT FUNCTION
    IF a$ = "Keyword Reference - By usage" THEN Back2BackName$ = "By Usage": EXIT FUNCTION
    IF a$ = "Keywords currently not supported by QB64" THEN Back2BackName$ = "Unsupported": EXIT FUNCTION
    IF a$ = "QB64 Help Menu" THEN Back2BackName$ = "Help": EXIT FUNCTION
    IF a$ = "QB64 FAQ" THEN Back2BackName$ = "FAQ": EXIT FUNCTION
    Back2BackName$ = a$
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
            fh = FREEFILE
            OPEN Cache_Folder$ + "/" + PageName3$ + ".txt" FOR BINARY AS #fh
            a$ = SPACE$(LOF(fh))
            GET #fh, , a$
            CLOSE #fh
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
    END IF

    'Find wikitext in the downloaded page
    s1 = INSTR(a$, s1$)
    IF s1 > 0 THEN a$ = MID$(a$, s1 + LEN(s1$)): s2 = INSTR(a$, s2$): ELSE s2 = 0
    IF s2 > 0 THEN a$ = LEFT$(a$, s2 - 1)
    IF s1 > 0 AND s2 > 0 AND a$ <> "" THEN
        'If wikitext was found, then substitute stuff & save it
        '--- first HTML specific entities
        WHILE INSTR(a$, "&amp;") > 0 '         '&amp; must be first and looped until all
            a$ = StrReplace$(a$, "&amp;", "&") 'multi-escapes are resolved (eg. &amp;lt; &amp;amp;lt; etc.)
        WEND
        a$ = StrReplace$(a$, "&lt;", "<")
        a$ = StrReplace$(a$, "&gt;", ">")
        a$ = StrReplace$(a$, "&quot;", CHR$(34))
        a$ = StrReplace$(a$, "&apos;", "'")
        '--- then other entities
        a$ = StrReplace$(a$, "&verbar;", "|")
        a$ = StrReplace$(a$, "&pi;", CHR$(227))
        a$ = StrReplace$(a$, "&theta;", CHR$(233))
        a$ = StrReplace$(a$, "&sup1;", CHR$(252))
        a$ = StrReplace$(a$, "&sup2;", CHR$(253))
        a$ = StrReplace$(a$, "&nbsp;", CHR$(255))
        '--- useless styles in blocks
        a$ = StrReplace$(a$, "Start}}'' ''", "Start}}")
        a$ = StrReplace$(a$, "Start}} '' ''", "Start}}")
        a$ = StrReplace$(a$, "Start}}" + CHR$(10) + "'' ''", "Start}}")
        a$ = StrReplace$(a$, "'' ''" + CHR$(10) + "{{", CHR$(10) + "{{")
        a$ = StrReplace$(a$, "'' '' " + CHR$(10) + "{{", CHR$(10) + "{{")
        a$ = StrReplace$(a$, "'' ''" + CHR$(10) + CHR$(10) + "{{", CHR$(10) + "{{")
        '--- wiki redirects & crlf
        a$ = StrReplace$(a$, "#REDIRECT", "See page")
        a$ = StrReplace$(a$, CHR$(13) + CHR$(10), CHR$(10))
        IF RIGHT$(a$, 1) <> CHR$(10) THEN a$ = a$ + CHR$(10)
        '--- put a download date/time entry
        a$ = "{{QBDLDATE:" + DATE$ + "}}" + CHR$(10) + "{{QBDLTIME:" + TIME$ + "}}" + CHR$(10) + a$
        '--- now save it
        fh = FREEFILE
        OPEN outputFile$ FOR OUTPUT AS #fh
        PRINT #fh, a$;
        CLOSE #fh
    ELSE
        'Error message, if empty or corrupted (force re-download on next access)
        a$ = CHR$(10) + "{{PageInternalError}}" + CHR$(10) +_
             "* Either the requested page is not yet available in the Wiki," + CHR$(10) +_
             "* or the download from Wiki failed and corrupted the page data." + CHR$(10) +_
             "** You may try ''Update Current Page'' from the ''Help'' menu." + CHR$(10) +_
             ";Note:This may also just be a temporary server issue. If the problem persists " +_
             "after waiting some time, then please feel free to leave us a message." + CHR$(10)
    END IF

    Wiki$ = a$
END FUNCTION

SUB Help_AddTxt (t$, col, link) 'Add help text, handle word wrap
    IF t$ = "" THEN EXIT SUB
    IF t$ = CHR$(13) THEN Help_NewLine: EXIT SUB
    IF Help_ChkBlank <> 0 THEN Help_CheckBlankLine: Help_ChkBlank = 0

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
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = Help_BG_Col * 16
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
        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
    END IF
END SUB

SUB Help_CheckBlankLine 'Make sure the last help line is a blank line (implies finish current)
    IF Help_Txt_Len >= 8 THEN
        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
        IF ASC(Help_Txt$, Help_Txt_Len - 7) <> 13 THEN Help_NewLine
    END IF
END SUB

SUB Help_CheckRemoveBlankLine 'If the last help line is blank, then remove it
    IF Help_Txt_Len >= 8 THEN
        IF ASC(Help_Txt$, Help_Txt_Len - 3) = 13 THEN
            Help_Txt_Len = Help_Txt_Len - 4
            help_h = help_h - 1
            Help_Line$ = LEFT$(Help_Line$, LEN(Help_Line$) - 4)
        END IF
        FOR i = Help_Txt_Len - 3 TO 1 STEP -4
            IF ASC(Help_Txt$, i) <> 32 THEN
                Help_Txt_Len = i + 3: EXIT FOR
            END IF
        NEXT
        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
    END IF
END SUB

FUNCTION Help_Col 'Helps to calculate the default color
    col = Help_Col_Normal
    IF Help_Italic THEN col = Help_Col_Italic
    IF Help_Bold THEN col = Help_Col_Bold 'Bold overrides Italic
    IF Help_Heading THEN col = Help_Col_Section 'Heading overrides all
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
    'soft: -1 = inside text blocks, -2 = inside fixed blocks
    '=> all parser locks also imply a wrapping lock (except -1)
    '=> hard locks almost every parsing except utf-8 substitution and line breaks
    '=> soft allows all elements not disrupting the current block, hence only
    '   paragraph creating things are locked (eg. headings, lists, rulers etc.),
    '   but text styles, links and template processing is still possible
    Help_LockParse = 0
    Help_Bold = 0: Help_Italic = 0: Help_Heading = 0
    Help_Underline = 0
    Help_BG_Col = 0
    Help_Center = 0: Help_CIndent$ = ""
    Help_DList = 0: Help_ChkBlank = 0

    link = 0: elink = 0: cb = 0: nl = 1: hl = 0: ah = 0: dl = 0

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
    Help_AddTxt "  ณ ", 14, 0: Help_AddTxt t$, 12, 0: Help_AddTxt " ณ", 14, 0
    i = Help_ww - i - 2 - Help_Pos: IF i < 2 THEN i = 2
    Help_AddTxt SPACE$(i) + CHR$(4), 14, 0
    IF LEFT$(d$, 4) = "Page" THEN i = 8: ELSE i = 7
    Help_LockWrap = 1: Help_AddTxt " " + d$, i, 0: Help_NewLine: Help_LockWrap = 0
    Help_AddTxt "ฤฤม" + STRING$(ii + 2, "ฤ") + "ม" + STRING$(Help_ww - ii - 6, "ฤ"), 14, 0: Help_NewLine

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
                        wla$ = StrReplace$(wla$, "''all''", "''all versions''")
                        wla$ = "* " + LEFT$(wla$, LEN(wla$) - 3) + CHR$(10)
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
            s$ = "<sup>": IF c$(LEN(s$)) = s$ THEN Help_AddTxt "^", col, 0: i = i + LEN(s$) - 1: GOTO charDone
            s$ = "</sup>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone

            s$ = "<center>" 'centered section
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                wla$ = wikiLookAhead$(a$, i + 1, "</center>")
                IF INSTR(wla$, "#toc") > 0 OR INSTR(wla$, "#top") > 0 OR INSTR(wla$, "to Top") > 0 THEN
                    i = i + LEN(wla$) + 9 'ignore TOC/TOP links
                ELSE
                    Help_Center = 1: Help_CIndent$ = wikiBuildCIndent$(wla$)
                    Help_AddTxt SPACE$(ASC(Help_CIndent$, 1)), col, 0 'center content
                    Help_CIndent$ = MID$(Help_CIndent$, 2)
                END IF
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
                        IF INSTR(wla$, "#toc") > 0 OR INSTR(wla$, "#top") > 0 OR INSTR(wla$, "to Top") > 0 THEN
                            i = ii + LEN(wla$) + 4 'ignore TOC/TOP links
                        ELSEIF INSTR(MID$(a$, i, ii - i), "center") > 0 THEN
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
            s$ = "<span" 'custom inline attributes ignored
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                FOR ii = i TO LEN(a$) - 1
                    IF MID$(a$, ii, 1) = ">" THEN i = ii: EXIT FOR
                NEXT
                GOTO charDone
            END IF
            s$ = "</span>"
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                GOTO charDone
            END IF

            s$ = "<div" 'ignore divisions (TOC and letter links)
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                FOR ii = i TO LEN(a$) - 1
                    IF MID$(a$, ii, 6) = "</div>" THEN i = ii + 5: EXIT FOR
                NEXT
                GOTO charDone
            END IF
            s$ = "<!--" 'ignore HTML comments
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                FOR ii = i TO LEN(a$) - 1
                    IF MID$(a$, ii, 3) = "-->" THEN i = ii + 2: EXIT FOR
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
                elink = 1
                elink$ = ""
                GOTO charDone
            END IF
            IF elink = 1 THEN
                IF c$ = "]" THEN
                    elink = 0
                    etext$ = elink$
                    i2 = INSTR(elink$, " ")
                    IF i2 > 0 THEN
                        etext$ = MID$(elink$, i2 + 1) 'text part
                        elink$ = LEFT$(elink$, i2 - 1) 'link part
                    END IF

                    Help_LinkN = Help_LinkN + 1
                    Help_Link$ = Help_Link$ + "EXTL:" + elink$ + Help_Link_Sep$

                    IF Help_LockParse = 0 THEN
                        Help_AddTxt etext$, Help_Col_Link, Help_LinkN
                    ELSE
                        Help_AddTxt etext$, Help_Col_Italic, Help_LinkN
                    END IF
                    GOTO charDone
                END IF
                elink$ = elink$ + c$
                GOTO charDone
            END IF
            'Internal links
            IF c$(2) = "[[" AND link = 0 THEN
                i = i + 1
                link = 1
                link$ = ""
                GOTO charDone
            END IF
        END IF
        'However, the internal link logic must run always, as it also handles
        'the template {{Cb|, {{Cl| and {{KW| links used in code blocks
        IF link = 1 THEN
            IF c$(2) = "]]" OR c$(2) = "}}" THEN
                i = i + 1
                link = 0
                text$ = link$
                i2 = INSTR(link$, "|") 'pipe link?
                IF i2 > 0 THEN
                    text$ = MID$(link$, i2 + 1) 'text part
                    link$ = LEFT$(link$, i2 - 1) 'link part
                END IF
                i2 = INSTR(link$, "#") 'local link?
                IF i2 > 0 THEN
                    IF text$ = link$ THEN text$ = MID$(link$, i2 + 1) 'use anchor if no alternate text yet
                    IF LEFT$(link$, 1) = "#" THEN link$ = Help_PageLoaded$ + link$ 'add current page if missing
                END IF
                IF LEFT$(link$, 9) = "Category:" THEN 'ignore category links
                    Help_CheckRemoveBlankLine
                    GOTO charDone
                END IF

                Help_LinkN = Help_LinkN + 1
                Help_Link$ = Help_Link$ + "PAGE:" + link$ + Help_Link_Sep$

                IF Help_LockParse = 0 THEN
                    Help_AddTxt text$, Help_Col_Link, Help_LinkN
                ELSE
                    Help_AddTxt text$, Help_Col_Italic, Help_LinkN
                END IF
                GOTO charDone
            END IF
            link$ = link$ + c$
            GOTO charDone
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
        IF c$(5) = "{{Cb|" OR c$(5) = "{{Cl|" OR c$(5) = "{{KW|" THEN 'just nice wrapped links
            i = i + 4 '                                               'KW is deprecated (but kept for existing pages)
            link = 1
            link$ = ""
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
                    ELSEIF LCASE$(LEFT$(cb$, 5)) = "small" THEN
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
                    'Standard section headings (section color, h3 single underline, h2 double underline)
                    'Recommended order of main page sections (h2) with it's considered sub-sections (h3)
                    IF cb$ = "PageSyntax" THEN cbo$ = "Syntax:"
                    IF cb$ = "PageParameters" OR cb$ = "Parameters" THEN cbo$ = "Parameters:" 'w/o Page prefix is deprecated (but kept for existing pages)
                    IF cb$ = "PageDescription" THEN cbo$ = "Description:"
                    IF cb$ = "PageNotes" THEN cbo$ = "Notes" 'sub-sect
                    IF cb$ = "PageErrors" THEN cbo$ = "Errors" 'sub-sect
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
                'Pre Block
                IF cb$ = "PreStart" AND Help_LockParse = 0 THEN
                    Help_CheckRemoveBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_LIndent$ = "  ": Help_LockParse = -1
                    Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF cb$ = "PreEnd" AND Help_LockParse <> 0 THEN
                    Help_LIndent$ = ""
                    Help_CheckRemoveBlankLine
                    Help_LockParse = 0
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
                'Fixed Block
                IF (cb$ = "FixedStart" OR cb$ = "WhiteStart") AND Help_LockParse = 0 THEN 'White is deprecated (but kept for existing pages)
                    Help_CheckBlankLine
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                    Help_BG_Col = 6: Help_LockParse = -2
                    Help_AddTxt STRING$(Help_ww - 16, 196) + " Fixed Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    IF c$(3) = "}}" + CHR$(10) THEN i = i + 1
                END IF
                IF (cb$ = "FixedEnd" OR cb$ = "WhiteEnd") AND Help_LockParse <> 0 THEN 'White is deprecated (but kept for existing pages)
                    Help_CheckFinishLine: Help_CheckRemoveBlankLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF

                'Template wrapped plugin
                IF RIGHT$(cb$, 6) = "Plugin" AND Help_LockParse = 0 THEN 'no plugins in blocks
                    pit$ = Wiki$("Template:" + cb$)
                    IF INSTR(pit$, "{{PageInternalError}}") = 0 THEN
                        a$ = LEFT$(a$, i) + pit$ + RIGHT$(a$, LEN(a$) - i)
                        n = n + LEN(pit$)
                    END IF
                END IF

                'Template wrapped table (try to get a readable plugin first)
                IF RIGHT$(cb$, 5) = "Table" AND Help_LockParse = 0 THEN 'no table info in blocks
                    pit$ = Wiki$("Template:" + LEFT$(cb$, LEN(cb$) - 5) + "Plugin")
                    IF INSTR(pit$, "{{PageInternalError}}") = 0 THEN
                        a$ = LEFT$(a$, i) + pit$ + RIGHT$(a$, LEN(a$) - i)
                        n = n + LEN(pit$)
                    ELSE
                        Help_LinkN = Help_LinkN + 1
                        Help_Link$ = Help_Link$ + "EXTL:" + wikiBaseAddress$ + "/index.php?title=Template:" + cb$ + Help_Link_Sep$
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "ษออออออออออออออออออออออออออออออออออออออป", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "บ", 8, 0: Help_AddTxt " The original page has a table here,  ", 15, Help_LinkN: Help_AddTxt "บ", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "บ", 8, 0: Help_AddTxt " please click inside this box to load ", 15, Help_LinkN: Help_AddTxt "บ", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "บ", 8, 0: Help_AddTxt " the table into your standard browser.", 15, Help_LinkN: Help_AddTxt "บ", 8, 0: Help_NewLine
                        Help_AddTxt SPACE$((Help_ww - 40) \ 2) + "ศออออออออออออออออออออออออออออออออออออออผ", 8, 0
                    END IF
                END IF

                'Parameter template text will be italic
                IF c$ = "|" AND cb$ = "Parameter" AND Help_LockParse <= 0 THEN 'keep as is in Code/Output blocks
                    Help_Italic = 1: col = Help_Col
                END IF

                'Small template text will be centered (maybe as block note)
                IF LCASE$(cb$) = "small" AND Help_LockParse <= 0 THEN 'keep as is in Code/Output blocks
                    wla$ = wikiLookAhead$(a$, i + 1, "}}")
                    Help_CIndent$ = wikiBuildCIndent$(wla$): iii = 0
                    IF i > 31 AND ASC(Help_CIndent$, 1) >= Help_ww / 4 THEN
                        IF INSTR(MID$(a$, i - 30, 30), "{{CodeEnd}}") > 0 THEN iii = -1
                        IF INSTR(MID$(a$, i - 30, 30), "{{TextEnd}}") > 0 THEN iii = -6
                        IF INSTR(MID$(a$, i - 31, 31), "{{FixedEnd}}") > 0 THEN iii = -6
                        IF INSTR(MID$(a$, i - 31, 31), "{{WhiteEnd}}") > 0 THEN iii = -6
                    END IF
                    IF iii <> 0 THEN
                        FOR ii = Help_Txt_Len - 3 TO 1 STEP -4
                            IF ASC(Help_Txt$, ii) = 32 AND iii < 0 THEN
                                Help_Pos = Help_Pos - 1
                            ELSEIF ASC(Help_Txt$, ii) = 13 AND iii < 0 THEN
                                help_h = help_h - 1: Help_Line$ = LEFT$(Help_Line$, LEN(Help_Line$) - 4)
                            ELSEIF ASC(Help_Txt$, ii) = 196 AND iii < 0 THEN
                                iii = -iii
                            ELSEIF ASC(Help_Txt$, ii) = 13 AND iii > 0 THEN
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

            IF cb = 1 THEN cb$ = cb$ + c$ 'reading macro name
            IF cb = 2 THEN Help_AddTxt CHR$(c), col, 0 'copy macro'd text
            GOTO charDone
        END IF

        'Wiki headings (==...==}) are not handled in blocks (soft- and hard lock), as it would
        'disrupt the block, also in code blocks it could be part of the code example itself
        IF Help_LockParse = 0 THEN
            'Custom section headings (section color, h3 single underline, h2 double underline)
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
                Help_CheckBlankLine
                Help_AddTxt STRING$(Help_ww, 196), 8, 0
                Help_ChkBlank = 1
                GOTO charDone
            END IF
            IF c$(4) = "<hr>" OR c$(6) = "<hr />" THEN
                IF c$(4) = "<hr>" THEN i = i + 3
                IF c$(6) = "<hr />" THEN i = i + 5
                Help_CheckBlankLine
                Help_AddTxt STRING$(Help_ww, 196), 8, 0
                Help_ChkBlank = 1
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
                IF c$(3) = ";* " OR c$(3) = ";# " THEN i = i + 2: Help_DList = 3 'list dot belongs to description
                IF c$(2) = ";*" OR c$(2) = ";#" THEN i = i + 1: Help_DList = 2 'list dot belongs to description
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

        'Unicode handling (no restrictions)
        IF ((c AND &HE0~%%) = 192) AND ((ASC(c$(2), 2) AND &HC0~%%) = 128) THEN '2-byte UTF-8
            i = i + 1
            FOR ii = 0 TO wpUtfReplCnt
                IF wpUtfRepl(ii).utf8 = c$(2) + MKI$(&H2020) THEN
                    Help_AddTxt RTRIM$(wpUtfRepl(ii).repl), col, 0: EXIT FOR
                END IF
            NEXT
            GOTO charDone
        END IF
        IF ((c AND &HF0~%%) = 224) AND ((ASC(c$(2), 2) AND &HC0~%%) = 128) AND ((ASC(c$(3), 3) AND &HC0~%%) = 128) THEN '3-byte UTF-8
            i = i + 2
            FOR ii = 0 TO wpUtfReplCnt
                IF wpUtfRepl(ii).utf8 = c$(3) + CHR$(0) THEN
                    Help_AddTxt RTRIM$(wpUtfRepl(ii).repl), col, 0: EXIT FOR
                END IF
            NEXT
            GOTO charDone
        END IF
        IF ((c AND &HF8~%%) = 240) AND ((ASC(c$(2), 2) AND &HC0~%%) = 128) AND ((ASC(c$(3), 3) AND &HC0~%%) = 128) AND ((ASC(c$(4), 4) AND &HC0~%%) = 128) THEN '4-byte UTF-8
            i = i + 3
            FOR ii = 0 TO wpUtfReplCnt
                IF wpUtfRepl(ii).utf8 = c$(4) THEN
                    Help_AddTxt RTRIM$(wpUtfRepl(ii).repl), col, 0: EXIT FOR
                END IF
            NEXT
            GOTO charDone
        END IF

        'Line break handling (no restrictions)
        IF c = 10 OR c$(4) = "<br>" OR c$(6) = "<br />" THEN
            IF c$(4) = "<br>" THEN i = i + 3
            IF c$(6) = "<br />" THEN i = i + 5
            IF c = 10 THEN 'on real new line only
                IF dl > 1 THEN dl = dl - 1 'update def list state
                IF Help_LockParse = 0 THEN Help_LIndent$ = "" 'end indention outside blocks
            END IF

            IF Help_LockParse > -2 THEN 'everywhere except in fixed blocks
                IF Help_Txt_Len >= 8 THEN 'allow max. one blank line (ie. collapse multi blanks to just one)
                    IF ASC(Help_Txt$, Help_Txt_Len - 3) = 13 AND ASC(Help_Txt$, Help_Txt_Len - 7) = 13 THEN
                        IF Help_Center > 0 THEN Help_CIndent$ = MID$(Help_CIndent$, 2) 'drop respective center indent
                        GOTO skipMultiBlanks
                    END IF
                END IF
            END IF
            Help_AddTxt CHR$(13), col, 0

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
        Help_AddTxt CHR$(c), col, hl

        charDone:
        nl = 0
        charDoneKnl: 'done, but keep nl state
        i = i + 1
    LOOP
    'END_PARSE_LOOP

    'Trim Help_Txt$
    Help_Txt$ = LEFT$(Help_Txt$, Help_Txt_Len) + CHR$(13) 'chr13 stops reads past end of content

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
            DO UNTIL c = 13
                ASC(a$, x2) = c
                lnk = CVI(MID$(Help_Txt$, x + 2, 2))
                IF oldlnk = 0 AND lnk <> 0 THEN lnkx1 = x2
                IF (lnk = 0 OR ASC(Help_Txt$, x + 4) = 13) AND lnkx1 <> 0 THEN
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

$UNSTABLE:HTTP
FUNCTION wikiDLPage$ (url$, timeout!)
    '--- set default result & avoid side effects ---
    wikiDLPage$ = ""
    wik$ = url$: tio# = timeout!
    '--- request wiki page ---
    retry:
    ch& = _OPENCLIENT(wik$)
    IF Help_Recaching < 2 THEN 'avoid messages for 'qb64pe -u' (build time update)
        IF ch& = 0 AND LCASE$(LEFT$(wik$, 8)) = "https://" THEN
            IF _SHELLHIDE("curl --version >NUL") <> 0 THEN
                'no external curl available (see notes below)
                IF _MESSAGEBOX("QB64-PE Help", "Can't make secure connection (https:) to Wiki, shall the IDE use unsecure (http:) instead?", "yesno", "warning" ) = 1 THEN
                    IF _MESSAGEBOX("QB64-PE Help", "Do you wanna save your choice permanently for the future?", "yesno", "question" ) = 1 THEN
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
        fh = FREEFILE
        OPEN responseFile$ FOR BINARY AS #fh
        res$ = SPACE$(LOF(fh))
        GET #fh, , res$
        CLOSE #fh
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
    b$ = StrReplace$(b$, "<br>", CHR$(10)) 'convert HTML line breaks
    b$ = StrReplace$(b$, "<br />", CHR$(10)) 'convert XHTML line breaks
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

