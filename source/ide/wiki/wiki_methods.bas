FUNCTION Back2BackName$ (a$)
    IF a$ = "Keyword Reference - Alphabetical" THEN Back2BackName$ = "Alphabetical": EXIT FUNCTION
    IF a$ = "Keyword Reference - By usage" THEN Back2BackName$ = "By Usage": EXIT FUNCTION
    IF a$ = "QB64 Help Menu" THEN Back2BackName$ = "Help": EXIT FUNCTION
    IF a$ = "QB64 FAQ" THEN Back2BackName$ = "FAQ": EXIT FUNCTION
    Back2BackName$ = a$
END FUNCTION

FUNCTION Wiki$ (PageName$)
    Help_PageLoaded$ = PageName$

    'Escape all invalid and other critical chars in filenames
    PageName2$ = ""
    FOR i = 1 TO LEN(PageName$)
        c = ASC(PageName$, i)
        SELECT CASE c
            CASE 32 '                                    '(space)
                PageName2$ = PageName2$ + "_"
            CASE 34, 38, 42, 47, 58, 60, 62, 63, 92, 124 '("&*/:<>?\|)
                PageName2$ = PageName2$ + "%" + HEX$(c)
            CASE ELSE
                PageName2$ = PageName2$ + CHR$(c)
        END SELECT
    NEXT

    'Is this page in the cache?
    IF Help_IgnoreCache = 0 THEN
        IF _FILEEXISTS(Cache_Folder$ + "/" + PageName2$ + ".txt") THEN
            fh = FREEFILE
            OPEN Cache_Folder$ + "/" + PageName2$ + ".txt" FOR BINARY AS #fh
            a$ = SPACE$(LOF(fh))
            GET #fh, , a$
            CLOSE #fh
            chr13 = INSTR(a$, CHR$(13))
            removedchr13 = 0
            DO WHILE chr13 > 0
                removedchr13 = -1
                a$ = LEFT$(a$, chr13 - 1) + MID$(a$, chr13 + 1)
                chr13 = INSTR(a$, CHR$(13))
            LOOP
            IF removedchr13 THEN
                fh = FREEFILE
                OPEN Cache_Folder$ + "/" + PageName2$ + ".txt" FOR OUTPUT AS #fh: CLOSE #fh
                OPEN Cache_Folder$ + "/" + PageName2$ + ".txt" FOR BINARY AS #fh
                PUT #fh, 1, a$
                CLOSE #fh
            END IF
            Wiki$ = a$
            EXIT FUNCTION
        END IF
    END IF

    'check for curl
    IF _SHELLHIDE("curl --version") <> 0 THEN
        PCOPY 2, 0
        result = idemessagebox("QB64", "Cannot find 'curl'.", "#Abort")
        PCOPY 3, 0: SCREEN , , 3, 0
        EXIT FUNCTION
    END IF

    'download message (Status Bar)
    IF Help_Recaching = 0 THEN
        a$ = "Downloading '" + PageName$ + "' page..."
        IF LEN(a$) > 60 THEN a$ = LEFT$(a$, 57) + STRING$(3, 250)
        IF LEN(a$) < 60 THEN a$ = a$ + SPACE$(60 - LEN(a$))

        COLOR 0, 3: LOCATE idewy + idesubwindow, 2
        PRINT a$;

        PCOPY 3, 0
    END IF

    'url query and output arguments for curl
    url$ = CHR$(34) + wikiBaseAddress$ + "/index.php?title=" + PageName2$ + "&action=edit" + CHR$(34)
    outputFile$ = Cache_Folder$ + "/" + PageName2$ + ".txt"
    'wikitext delimiters
    s1$ = "name=" + CHR$(34) + "wpTextbox1" + CHR$(34) + ">"
    s2$ = "</textarea>"

    'download page using curl (opt -k = allow unsecure connections in case of cert errors)
    SHELL _HIDE "curl -k -o " + CHR$(34) + outputFile$ + CHR$(34) + " " + url$
    fh = FREEFILE
    OPEN outputFile$ FOR BINARY AS #fh 'get new content
    a$ = SPACE$(LOF(fh))
    GET #fh, 1, a$
    CLOSE #fh

    'find wikitext in the downloaded page
    s1 = INSTR(a$, s1$)
    IF s1 > 0 THEN a$ = MID$(a$, s1 + LEN(s1$)): s2 = INSTR(a$, s2$): ELSE s2 = 0
    IF s2 > 0 THEN a$ = LEFT$(a$, s2 - 1)
    IF s1 > 0 AND s2 > 0 AND a$ <> "" THEN
        'if wikitext was found, then substitute entities & save it
        '--- first HTML specific
        WHILE INSTR(a$, "&amp;") > 0 '         '&amp; must be first and looped until all
            a$ = StrReplace$(a$, "&amp;", "&") 'multi-escapes are resolved (eg. &amp;lt; &amp;amp;lt; etc.)
        WEND
        a$ = StrReplace$(a$, "&lt;", "<")
        a$ = StrReplace$(a$, "&gt;", ">")
        a$ = StrReplace$(a$, "&quot;", CHR$(34))
        '--- then other entities
        a$ = StrReplace$(a$, "&verbar;", "|")
        a$ = StrReplace$(a$, "&pi;", CHR$(227))
        a$ = StrReplace$(a$, "&theta;", CHR$(233))
        a$ = StrReplace$(a$, "&sup1;", CHR$(252))
        a$ = StrReplace$(a$, "&sup2;", CHR$(253))
        a$ = StrReplace$(a$, "&nbsp;", CHR$(255))
        '--- now save it
        OPEN outputFile$ FOR OUTPUT AS #fh
        PRINT #fh, a$
        CLOSE #fh
    ELSE
        'delete page, if empty or corrupted (force re-download on next access)
        KILL outputFile$
        a$ = "{{PageInternalError}}" + CHR$(10) +_
             "* Either the requested page is not yet available in the Wiki," + CHR$(10) +_
             "* or the download from Wiki failed and corrupted the page data." + CHR$(10) +_
             "** You may try ''Update Current Page'' from the ''Help'' menu." + CHR$(10)
    END IF

    Wiki$ = a$
END FUNCTION

SUB Help_AddTxt (t$, col, link)
    IF t$ = CHR$(13) THEN Help_NewLine: EXIT SUB

    FOR i = 1 TO LEN(t$)
        c = ASC(t$, i)

        IF Help_LockParse = 0 AND Help_LockWrap = 0 THEN

            'addtxt handles all wrapping issues
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

        END IF 'locks = 0

        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = c
        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = col + Help_BG_Col * 16
        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = link AND 255
        Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = link \ 256

        Help_Pos = Help_Pos + 1
    NEXT
END SUB

SUB Help_NewLine
    IF Help_Pos > help_w THEN help_w = Help_Pos

    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 13
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = Help_BG_Col * 16
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 0
    Help_Txt_Len = Help_Txt_Len + 1: ASC(Help_Txt$, Help_Txt_Len) = 0

    help_h = help_h + 1
    Help_Line$ = Help_Line$ + MKL$(Help_Txt_Len + 1)
    Help_Wrap_Pos = 0

    IF Help_Underline THEN
        w = Help_Pos
        Help_Pos = 1
        Help_AddTxt STRING$(w - 1, 196), Help_Underline, 0
        Help_Underline = 0 'keep before Help_NewLine (recursion)
        Help_NewLine
    END IF
    Help_Pos = 1

    IF Help_Center > 0 THEN 'center overrides regular indent
        Help_NewLineIndent = 0
        Help_AddTxt SPACE$(ASC(LEFT$(Help_CIndent$, 1))), col, 0
        Help_CIndent$ = MID$(Help_CIndent$, 2)
    ELSEIF Help_NewLineIndent > 0 THEN
        Help_AddTxt SPACE$(Help_NewLineIndent), Help_Col, 0
    END IF
END SUB

FUNCTION Help_Col 'helps to calculate the default color
    col = Help_Col_Normal
    IF Help_Italic THEN col = Help_Col_Italic
    IF Help_Bold THEN col = Help_Col_Bold 'Note: Bold overrides italic
    Help_Col = col
END FUNCTION

SUB WikiParse (a$)

    'wiki page interpret

    'clear info
    help_h = 0: help_w = 0: Help_Line$ = "": Help_Link$ = "": Help_LinkN = 0
    Help_Txt$ = SPACE$(1000000)
    Help_Txt_Len = 0

    Help_Pos = 1: Help_Wrap_Pos = 0
    Help_Line$ = MKL$(1)
    'word wrap locks (lock wrapping only, but continue parsing regularly)
    Help_LockWrap = 0
    'parser locks (neg: soft lock, zero: unlocked, pos: hard lock)
    'hard:  2 = inside code blocks,  1 = inside output blocks
    'soft: -1 = inside text blocks, -2 = inside fixed blocks
    '=> all parser locks also imply a wrapping lock
    '=> hard locks almost every parsing except utf-8 substitution and line breaks
    '=> soft allows all elements not disrupting the current block, hence only
    '   paragraph creating things are locked (eg. headings, lists, rulers etc.),
    '   but text styles, links and template processing is still possible
    Help_LockParse = 0
    Help_Bold = 0: Help_Italic = 0
    Help_Underline = 0
    Help_BG_Col = 0
    Help_Center = 0: Help_CIndent$ = ""
    Help_DList = 0

    link = 0: elink = 0: cb = 0: nl = 1

    col = Help_Col

    'Syntax Notes:
    ' '''=bold
    ' ''=italic
    ' {{macroname|macroparam}} or simply {{macroname}}
    '  eg. {{KW|PRINT}}=a key word, a link to a page
    '      {{Cl|PRINT}}=a key word in a code example, will be printed in bold and aqua
    '      {{Parameter|expression}}=a parameter, in italics
    '      {{PageSyntax}} {{PageParameters}} {{PageDescription}} {{PageExamples}}
    '      {{CodeStart}} {{CodeEnd}} {{OutputStart}} {{OutputEnd}}
    '      {{PageSeeAlso}} {{PageNavigation}} {{PageLegacySupport}}
    '      {{PageQBasic}} {{PageAvailability}}
    ' [[SPACE$]]=a link to wikipage called "SPACE$"
    ' [[INTEGER|integer]]=a link, link's name is on left and text to appear is on right
    ' *=a dot point
    ' **=a sub(ie. further indented) dot point
    ' &quot;=a quotation mark
    ' :=indent (if beginning a new line)
    ' CHR$(10)=new line character

    prefetch = 20
    DIM c$(prefetch)
    FOR ii = 1 TO prefetch
        c$(ii) = SPACE$(ii)
    NEXT

    i = INSTR(a$, "<span ")
    DO WHILE i > 0
        a$ = LEFT$(a$, i - 1) + MID$(a$, INSTR(i + 1, a$, ">") + 1)
        i = INSTR(a$, "<span ")
    LOOP

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

        'Wiki specific code must be handled always
        s$ = "__NOEDITSECTION__" + CHR$(10): IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "__NOEDITSECTION__": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "__NOTOC__" + CHR$(10): IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "__NOTOC__": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "<nowiki>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone
        s$ = "</nowiki>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone

        IF Help_LockParse <= 0 THEN 'keep text AS IS in Code/Output blocks
            'Handle some direct HTML code
            s$ = "<sup>": IF c$(LEN(s$)) = s$ THEN Help_AddTxt "^", col, 0: i = i + LEN(s$) - 1: GOTO charDone
            s$ = "</sup>": IF c$(LEN(s$)) = s$ THEN i = i + LEN(s$) - 1: GOTO charDone

            s$ = "<center>"
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                wla$ = wikiLookAhead$(a$, i + 1, "</center>")
                IF INSTR(wla$, "#toc") > 0 OR INSTR(wla$, "to Top") > 0 THEN
                    i = i + LEN(wla$) + 9 'ignore TOC links
                ELSE
                    Help_Center = 1: Help_CIndent$ = wikiBuildCIndent$(wla$)
                    Help_AddTxt SPACE$(ASC(LEFT$(Help_CIndent$, 1))), col, 0 'center content
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

            s$ = "<p style="
            IF c$(LEN(s$)) = s$ THEN
                i = i + LEN(s$) - 1
                FOR ii = i TO LEN(a$) - 1
                    IF MID$(a$, ii, 1) = ">" THEN
                        wla$ = wikiLookAhead$(a$, ii + 1, "</p>")
                        IF INSTR(wla$, "#toc") > 0 OR INSTR(wla$, "to Top") > 0 THEN
                            i = ii + LEN(wla$) + 4 'ignore TOC links
                        ELSEIF INSTR(MID$(a$, i, ii - i), "center") > 0 THEN
                            Help_Center = 1: Help_CIndent$ = wikiBuildCIndent$(wla$)
                            Help_AddTxt SPACE$(ASC(LEFT$(Help_CIndent$, 1))), col, 0 'center (if in style)
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

            s$ = "<div"
            IF c$(LEN(s$)) = s$ THEN 'ignore divisions (TOC and letter links)
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

            'Links
            IF c = 91 THEN '"["
                IF c$(2) = "[[" AND link = 0 THEN
                    i = i + 1
                    link = 1
                    link$ = ""
                    GOTO charDone
                END IF
            END IF
        END IF
        IF link = 1 THEN
            IF c$(2) = "]]" OR c$(2) = "}}" THEN
                i = i + 1
                link = 0
                text$ = link$
                i2 = INSTR(link$, "|")
                IF i2 > 0 THEN
                    text$ = RIGHT$(link$, LEN(link$) - i2)
                    link$ = LEFT$(link$, i2 - 1)
                END IF

                IF INSTR(link$, "#") THEN 'local page links not supported yet
                    Help_AddTxt text$, 8, 0
                    GOTO charDone
                END IF

                Help_LinkN = Help_LinkN + 1
                Help_Link$ = Help_Link$ + "PAGE:" + link$ + Help_Link_Sep$

                IF Help_BG_Col = 0 THEN
                    Help_AddTxt text$, Help_Col_Link, Help_LinkN
                ELSE
                    Help_AddTxt text$, Help_Col_Bold, Help_LinkN
                END IF
                GOTO charDone
            END IF
            link$ = link$ + c$
            GOTO charDone
        END IF

        IF Help_LockParse <= 0 THEN 'keep text AS IS in Code/Output blocks
            'External links
            IF c = 91 THEN '"["
                IF (c$(6) = "[http:" OR c$(7) = "[https:") AND elink = 0 THEN
                    elink = 2
                    elink$ = ""
                    GOTO charDone
                END IF
            END IF
            IF elink = 2 THEN
                IF c$ = " " THEN
                    elink = 1
                    GOTO charDone
                END IF
                elink$ = elink$ + c$
                GOTO charDone
            END IF
            IF elink >= 1 THEN
                IF c$ = "]" THEN
                    elink = 0
                    elink$ = " " + elink$
                    Help_LockWrap = 1: Help_Wrap_Pos = 0
                    Help_AddTxt elink$, 8, 0
                    Help_LockWrap = 0
                    elink$ = ""
                    GOTO charDone
                END IF
            END IF
        END IF

        IF Help_LockParse <= 0 THEN 'keep text AS IS in Code/Output blocks
            'Bold & Italic styles
            IF c$(3) = "'''" THEN
                i = i + 2
                IF Help_Bold = 0 THEN Help_Bold = 1 ELSE Help_Bold = 0
                col = Help_Col
                GOTO charDone
            END IF
            IF c$(2) = "''" THEN
                i = i + 1
                IF Help_Italic = 0 THEN Help_Italic = 1 ELSE Help_Italic = 0
                col = Help_Col
                GOTO charDone
            END IF
        END IF

        'Templates
        IF c = 123 THEN '"{"
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
        END IF
        IF cb > 0 THEN
            IF c$ = "|" OR c$(2) = "}}" THEN
                IF c$ = "|" AND cb = 2 THEN
                    wla$ = wikiLookAhead$(a$, i + 1, "}}")
                    cb = 0: i = i + LEN(wla$) + 2 'after 1st, ignore all further template parameters
                ELSEIF c$(2) = "}}" THEN
                    cb = 0: i = i + 1
                END IF
                IF c$ = "|" AND cb = 1 THEN cb = 2

                IF Help_LockParse = 0 THEN 'keep text AS IS in all blocks
                    cbo$ = ""
                    'Standard section headings (section color, h3 w/o underline, h2 with underline)
                    'Recommended order of main page sections (h2) with it's considered sub-sections (h3)
                    IF cb$ = "PageSyntax" THEN cbo$ = "Syntax:"
                    IF cb$ = "PageLegacySupport" THEN cbo$ = "Legacy support" 'sub-sect
                    IF cb$ = "PageParameters" OR cb$ = "Parameters" THEN cbo$ = "Parameters:" 'w/o Page suffix is deprecated (but kept for existing pages)
                    IF cb$ = "PageDescription" THEN cbo$ = "Description:"
                    IF cb$ = "PageQBasic" THEN cbo$ = "QBasic/QuickBASIC" 'sub-sect
                    IF cb$ = "PageNotes" THEN cbo$ = "Notes" 'sub-sect
                    IF cb$ = "PageErrors" THEN cbo$ = "Errors" 'sub-sect
                    IF cb$ = "PageUseWith" THEN cbo$ = "Use with" 'sub-sect
                    IF cb$ = "PageAvailability" THEN cbo$ = "Availability:"
                    IF cb$ = "PageExamples" THEN cbo$ = "Examples:"
                    IF cb$ = "PageSeeAlso" THEN cbo$ = "See also:"
                    'Independent main page end sections (centered, no title)
                    IF cb$ = "PageCopyright" THEN cbo$ = "Copyright"
                    IF cb$ = "PageNavigation" THEN cbo$ = "" 'ignored for built-in help
                    'Internally used templates (not available in Wiki)
                    IF cb$ = "PageWelcome" THEN cbo$ = "Welcome:"
                    IF cb$ = "PageCommunity" THEN cbo$ = "Community:"
                    IF cb$ = "PageInternalError" THEN cbo$ = "Sorry, an error occurred:"
                    '----------
                    IF cbo$ <> "" THEN
                        IF RIGHT$(cbo$, 1) = ":" THEN Help_Underline = Help_Col_Section
                        Help_AddTxt cbo$, Help_Col_Section, 0: Help_NewLine
                        IF cbo$ = "Copyright" THEN
                            Help_NewLine: Help_AddTxt "1991-2006 Silicon Graphics, Inc.", 7, 0: Help_NewLine
                            Help_AddTxt "This document is licensed under the SGI Free Software B License.", 7, 0: Help_NewLine
                            Help_AddTxt "https://spdx.org/licenses/SGI-B-2.0.html https://spdx.org/licenses/SGI-B-2.0.html", 15, 0: Help_NewLine
                        END IF
                    END IF
                END IF

                IF cb$ = "CodeStart" AND Help_LockParse = 0 THEN
                    IF Help_Txt_Len >= 8 THEN
                        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
                        IF ASC(Help_Txt$, Help_Txt_Len - 7) <> 13 THEN Help_NewLine
                    END IF
                    Help_BG_Col = 1: Help_LockParse = 2
                    Help_AddTxt STRING$(Help_ww - 15, 196) + " Code Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    'Skip non-meaningful content before section begins
                    ws = 1
                    FOR ii = i + 1 TO LEN(a$)
                        IF ASC(a$, ii) = 10 THEN EXIT FOR
                        IF ASC(a$, ii) <> 32 AND ASC(a$, ii) <> 39 THEN ws = 0
                    NEXT
                    IF ws THEN i = ii
                END IF
                IF cb$ = "CodeEnd" AND Help_LockParse <> 0 THEN
                    IF nl = 0 THEN Help_NewLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                IF LEFT$(cb$, 11) = "OutputStart" AND Help_LockParse = 0 THEN 'does also match new OutputStartBGn templates
                    IF Help_Txt_Len >= 8 THEN
                        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
                        IF ASC(Help_Txt$, Help_Txt_Len - 7) <> 13 THEN Help_NewLine
                    END IF
                    Help_BG_Col = 2: Help_LockParse = 1
                    Help_AddTxt STRING$(Help_ww - 17, 196) + " Output Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    'Skip non-meaningful content before section begins
                    ws = 1
                    FOR ii = i + 1 TO LEN(a$)
                        IF ASC(a$, ii) = 10 THEN EXIT FOR
                        IF ASC(a$, ii) <> 32 AND ASC(a$, ii) <> 39 THEN ws = 0
                    NEXT
                    IF ws THEN i = ii
                END IF
                IF cb$ = "OutputEnd" AND Help_LockParse <> 0 THEN
                    IF nl = 0 THEN Help_NewLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                IF cb$ = "TextStart" AND Help_LockParse = 0 THEN
                    IF Help_Txt_Len >= 8 THEN
                        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
                        IF ASC(Help_Txt$, Help_Txt_Len - 7) <> 13 THEN Help_NewLine
                    END IF
                    Help_BG_Col = 6: Help_LockParse = -1
                    Help_AddTxt STRING$(Help_ww - 15, 196) + " Text Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    'Skip non-meaningful content before section begins
                    ws = 1
                    FOR ii = i + 1 TO LEN(a$)
                        IF ASC(a$, ii) = 10 THEN EXIT FOR
                        IF ASC(a$, ii) <> 32 AND ASC(a$, ii) <> 39 THEN ws = 0
                    NEXT
                    IF ws THEN i = ii
                END IF
                IF cb$ = "TextEnd" AND Help_LockParse <> 0 THEN
                    IF nl = 0 THEN Help_NewLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF
                IF (cb$ = "FixedStart" OR cb$ = "WhiteStart") AND Help_LockParse = 0 THEN 'White is deprecated (but kept for existing pages)
                    IF Help_Txt_Len >= 8 THEN
                        IF ASC(Help_Txt$, Help_Txt_Len - 3) <> 13 THEN Help_NewLine
                        IF ASC(Help_Txt$, Help_Txt_Len - 7) <> 13 THEN Help_NewLine
                    END IF
                    Help_BG_Col = 6: Help_LockParse = -2
                    Help_AddTxt STRING$(Help_ww - 16, 196) + " Fixed Block " + STRING$(3, 196), 15, 0: Help_NewLine
                    'Skip non-meaningful content before section begins
                    ws = 1
                    FOR ii = i + 1 TO LEN(a$)
                        IF ASC(a$, ii) = 10 THEN EXIT FOR
                        IF ASC(a$, ii) <> 32 AND ASC(a$, ii) <> 39 THEN ws = 0
                    NEXT
                    IF ws THEN i = ii
                END IF
                IF (cb$ = "FixedEnd" OR cb$ = "WhiteEnd") AND Help_LockParse <> 0 THEN 'White is deprecated (but kept for existing pages)
                    IF nl = 0 THEN Help_NewLine
                    Help_AddTxt STRING$(Help_ww, 196), 15, 0: Help_NewLine
                    Help_BG_Col = 0: Help_LockParse = 0
                    Help_Bold = 0: Help_Italic = 0: col = Help_Col
                END IF

                IF RIGHT$(cb$, 5) = "Table" AND Help_LockParse <= 0 THEN 'keep text AS IS in Code/Output blocks
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "ษออออออออออออออออออออออออออออออออออออออออออออออออออป", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "บ ", 8, 0: Help_AddTxt "The original help page has a table here, please ", 15, 0: Help_AddTxt " บ", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "บ ", 8, 0: Help_AddTxt "use the ", 15, 0: ii = Help_BG_Col: Help_BG_Col = 3: Help_AddTxt " View on Wiki ", 15, 0: Help_BG_Col = ii: Help_AddTxt " button in the upper right", 15, 0: Help_AddTxt " บ", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "บ ", 8, 0: Help_AddTxt "corner to load the page into your browser.      ", 15, 0: Help_AddTxt " บ", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "ศออออออออออออออออออออออออออออออออออออออออออออออออออผ", 8, 0
                END IF
                GOTO charDone

            END IF

            IF cb = 1 THEN cb$ = cb$ + c$ 'reading macro name
            IF cb = 2 THEN Help_AddTxt CHR$(c), col, 0 'copy macro'd text
            GOTO charDone
        END IF 'cb > 0

        IF Help_LockParse = 0 THEN 'keep text AS IS in all blocks
            'Custom section headings (current color, h3 w/o underline, h2 with underline)
            ii = 0
            IF c$(5) = " === " THEN ii = 4
            IF c$(4) = "=== " THEN ii = 3
            IF c$(4) = " ===" THEN ii = 3
            IF c$(3) = "===" THEN ii = 2
            IF ii > 0 THEN i = i + ii: GOTO charDone
            ii = 0
            IF c$(4) = " == " THEN ii = 3
            IF c$(3) = "== " THEN ii = 2
            IF c$(3) = " ==" THEN ii = 2
            IF c$(2) = "==" THEN ii = 1
            IF ii > 0 THEN i = i + ii: Help_Underline = col: GOTO charDone
        END IF

        IF Help_LockParse = 0 THEN 'keep text AS IS in all blocks
            'Rulers
            IF c$(4) = "----" AND nl = 1 THEN
                i = i + 3
                Help_AddTxt STRING$(Help_ww, 196), 8, 0
                GOTO charDone
            END IF

            'Definition lists
            IF c$ = ";" AND nl = 1 THEN 'definition (new line only)
                Help_Bold = 1: col = Help_Col: Help_DList = 1
                IF c$(3) = ";* " THEN i = i + 2: Help_DList = 3 'list dot belongs to description
                IF c$(2) = ";*" THEN i = i + 1: Help_DList = 2 'list dot belongs to description
                GOTO charDone
            END IF
            IF c$ = ":" AND Help_DList > 0 THEN 'description (same or new line)
                Help_Bold = 0: col = Help_Col
                IF nl = 0 THEN Help_NewLine
                Help_AddTxt "   ", col, 0
                Help_NewLineIndent = Help_NewLineIndent + 3
                IF Help_DList > 1 THEN
                    Help_AddTxt CHR$(4) + " ", 15, 0
                    Help_NewLineIndent = Help_NewLineIndent + 2
                END IF
                Help_DList = 0
                GOTO charDone
            END IF
            IF c$ = ":" AND nl = 1 THEN 'independent description w/o definition (new line only)
                Help_AddTxt "   ", col, 0
                Help_NewLineIndent = Help_NewLineIndent + 3
                GOTO charDoneKnl 'keep nl state for possible <UL> list bullets
            END IF

            'Unordered lists
            IF nl = 1 THEN
                IF c$(3) = "** " THEN
                    i = i + 2
                    Help_AddTxt "   " + CHR$(4) + " ", 15, 0
                    Help_NewLineIndent = Help_NewLineIndent + 5
                    GOTO charDone
                END IF
                IF c$(2) = "* " THEN
                    i = i + 1
                    Help_AddTxt CHR$(4) + " ", 15, 0
                    Help_NewLineIndent = Help_NewLineIndent + 2
                    GOTO charDone
                END IF
                IF c$(2) = "**" THEN
                    i = i + 1
                    Help_AddTxt "   " + CHR$(4) + " ", 15, 0
                    Help_NewLineIndent = Help_NewLineIndent + 5
                    GOTO charDone
                END IF
                IF c$ = "*" THEN
                    Help_AddTxt CHR$(4) + " ", 15, 0
                    Help_NewLineIndent = Help_NewLineIndent + 2
                    GOTO charDone
                END IF
            END IF
        END IF

        IF Help_LockParse <= 0 THEN 'keep text AS IS in Code/Output blocks
            'Tables (ignored)
            IF c$(2) = "{|" THEN
                wla$ = wikiLookAhead$(a$, i + 2, "|}"): iii = 0
                FOR ii = 1 TO LEN(wla$)
                    IF MID$(wla$, ii, 1) = "|" AND MID$(wla$, ii, 2) <> "|-" THEN iii = iii + 1
                NEXT
                i = i + 1 + LEN(wla$) + 2
                IF iii > 1 OR INSTR(wla$, "__TOC__") = 0 THEN 'ignore TOC only tables
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "ษออออออออออออออออออออออออออออออออออออออออออออออออออป", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "บ ", 8, 0: Help_AddTxt "The original help page has a table here, please ", 15, 0: Help_AddTxt " บ", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "บ ", 8, 0: Help_AddTxt "use the ", 15, 0: ii = Help_BG_Col: Help_BG_Col = 3: Help_AddTxt " View on Wiki ", 15, 0: Help_BG_Col = ii: Help_AddTxt " button in the upper right", 15, 0: Help_AddTxt " บ", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "บ ", 8, 0: Help_AddTxt "corner to load the page into your browser.      ", 15, 0: Help_AddTxt " บ", 8, 0: Help_NewLine
                    Help_AddTxt SPACE$((Help_ww - 52) \ 2) + "ศออออออออออออออออออออออออออออออออออออออออออออออออออผ", 8, 0
                END IF
                GOTO charDone
            END IF
        END IF

        'Unicodes
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

        'Line breaks
        IF c = 10 OR c$(4) = "<br>" OR c$(6) = "<br />" THEN
            IF c$(4) = "<br>" THEN i = i + 3
            IF c$(6) = "<br />" THEN i = i + 5
            Help_NewLineIndent = 0

            IF Help_LockParse > -2 THEN 'everywhere except in fixed blocks
                IF Help_Txt_Len >= 8 THEN 'allow max. one blank line (ie. collapse multi blanks to just one)
                    IF ASC(Help_Txt$, Help_Txt_Len - 3) = 13 AND ASC(Help_Txt$, Help_Txt_Len - 7) = 13 THEN
                        IF Help_Center > 0 THEN Help_CIndent$ = MID$(Help_CIndent$, 2) 'drop respective center indent
                        Help_DList = 0: Help_Bold = 0: col = Help_Col 'definition list incl. style ends after blank line
                        GOTO skipMultiBlanks
                    END IF
                END IF
            END IF
            Help_AddTxt CHR$(13), col, 0

            skipMultiBlanks:
            IF Help_LockParse <> 0 THEN 'in all blocks reset styles at EOL
                Help_Bold = 0: Help_Italic = 0
                col = Help_Col
            END IF
            nl = 1
            GOTO charDoneKnl 'keep just set nl state
        END IF

        Help_AddTxt CHR$(c), col, 0

        charDone:
        nl = 0
        charDoneKnl: 'done, but keep nl state
        i = i + 1
    LOOP

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
                    IF ASC(a$, 1) <> 254 THEN GOTO ignorelink

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

FUNCTION wikiBuildCIndent$ (a$)
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
    org$ = b$: b$ = "" 'eliminate text styles and ext. links
    FOR i = 1 TO LEN(org$)
        IF MID$(org$, i, 3) = "'''" THEN i = i + 3
        IF MID$(org$, i, 2) = "''" THEN i = i + 2
        IF MID$(org$, i, 1) = "[" THEN i = i + 1
        IF MID$(org$, i, 1) = "]" THEN i = i + 1
        b$ = b$ + MID$(org$, i, 1)
    NEXT
    b$ = b$ + CHR$(10)

    i = 1: st = 1: br = 0: res$ = ""
    WHILE i <= LEN(b$)
        ws = INSTR(i, b$, " "): lb = INSTR(i, b$, CHR$(10))
        IF lb > 0 AND (ws > lb OR lb - st <= Help_ww) THEN SWAP ws, lb
        IF ws > 0 AND ws - st <= Help_ww THEN
            br = ws: i = ws + 1
            IF ASC(b$, ws) <> 10 AND i <= LEN(b$) THEN _CONTINUE
        END IF
        IF br = 0 THEN br = lb
        ci = (Help_ww - (br - st)) \ 2: IF ci < 0 THEN ci = 0
        res$ = res$ + CHR$(ci)
        i = br + 1: st = br + 1: br = 0
    WEND
    wikiBuildCIndent$ = res$
END FUNCTION

FUNCTION wikiLookAhead$ (a$, i, token$)
    wikiLookAhead$ = "": IF i >= LEN(a$) THEN EXIT FUNCTION
    j = INSTR(i, a$, token$)
    IF j = 0 THEN
        wikiLookAhead$ = MID$(a$, i)
    ELSE
        wikiLookAhead$ = MID$(a$, i, j - i)
        i = j + 1
    END IF
END FUNCTION

