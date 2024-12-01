OPTION _EXPLICIT
$SCREENHIDE
$CONSOLE
DEFLNG A-Z

'Keywords as of v3.14.1, all later added keywords shouldn't appear in old $NOPREFIX code
'Removed leading @
'Line continutation formatting
'Removed all non-underscore items
'Removed all metacommands
'Removed _ALL, _ANTICLOCKWISE, _AUTO, _BEHIND, _CLEAR, _CLIP, _CLOCKWISE, _DONTWAIT,
'        _EXPLICITARRAY, _HARDWARE, _HARDWARE1, _HIDE, _MIDDLE, _NONE, _OFF (OFF apears to be just as valid)
'        _ONLY, _ONTOP, _SEAMLESS, _SHOW, _SMOOTHSHRUNK, _SMOOTHSTRETCHED, _SOFTWARE, _SQUAREPIXELS, _STRETCH, _TOGGLE
const KEYWORDS = "_ACCEPTFILEDROP@_ACOS@_ACOSH@_ADLER32@_ALLOWFULLSCREEN@_ALPHA@_ALPHA32@_ANDALSO@_ARCCOT@_ARCCSC@_ARCSEC@_ASIN@_ASINH@_ASSERT@_ATAN2@_ATANH@_AUTODISPLAY@_AXIS@"+_
"_BACKGROUNDCOLOR@_BIN$@_BIT@_BLEND@_BLINK@_BLUE@_BLUE32@_BUTTON@_BUTTONCHANGE@_BYTE@"+_
"_CAPSLOCK@_CEIL@_CINP@_CLEARCOLOR@_CLIPBOARD$@_CLIPBOARDIMAGE@_COLORCHOOSERDIALOG@_COMMANDCOUNT@_CONNECTED@_CONNECTIONADDRESS@_CONNECTIONADDRESS$@_CONSOLE@_CONSOLECURSOR@_CONSOLEFONT@_CONSOLEINPUT@_CONSOLETITLE@_CONTINUE@_CONTROLCHR@_COPYIMAGE@_COPYPALETTE@_COSH@_COT@_COTH@_CRC32@_CSC@_CSCH@_CV@_CWD$@"+_
"_D2G@_D2R@_DEFAULTCOLOR@_DEFINE@_DEFLATE$@_DELAY@_DEPTHBUFFER@_DESKTOPHEIGHT@_DESKTOPWIDTH@_DEST@_DEVICE$@_DEVICEINPUT@_DEVICES@_DIR$@_DIREXISTS@_DISPLAY@_DISPLAYORDER@_DONTBLEND@_DROPPEDFILE@_DROPPEDFILE$@"+_
"_ECHO@_EMBEDDED$@_ENVIRONCOUNT@_ERRORLINE@_ERRORMESSAGE$@_EXIT@_EXPLICIT@"+_
"_FILEEXISTS@_FILES$@_FILLBACKGROUND@_FINISHDROP@_FLOAT@_FONT@_FONTHEIGHT@_FONTWIDTH@_FPS@_FREEFONT@_FREEIMAGE@_FREETIMER@_FULLPATH$@_FULLSCREEN@"+_
"_G2D@_G2R@"+_
"_GLACCUM@_GLALPHAFUNC@_GLARETEXTURESRESIDENT@_GLARRAYELEMENT@"+_
"_GLBEGIN@_GLBINDTEXTURE@_GLBITMAP@_GLBLENDFUNC@"+_
"_GLCALLLIST@_GLCALLLISTS@_GLCLEAR@_GLCLEARACCUM@_GLCLEARCOLOR@_GLCLEARDEPTH@_GLCLEARINDEX@_GLCLEARSTENCIL@_GLCLIPPLANE@_GLCOLOR3B@_GLCOLOR3BV@_GLCOLOR3D@_GLCOLOR3DV@_GLCOLOR3F@_GLCOLOR3FV@_GLCOLOR3I@_GLCOLOR3IV@_GLCOLOR3S@_GLCOLOR3SV@_GLCOLOR3UB@_GLCOLOR3UBV@_GLCOLOR3UI@_GLCOLOR3UIV@_GLCOLOR3US@_GLCOLOR3USV@_GLCOLOR4B@_GLCOLOR4BV@_GLCOLOR4D@_GLCOLOR4DV@_GLCOLOR4F@_GLCOLOR4FV@_GLCOLOR4I@_GLCOLOR4IV@_GLCOLOR4S@_GLCOLOR4SV@_GLCOLOR4UB@_GLCOLOR4UBV@_GLCOLOR4UI@_GLCOLOR4UIV@_GLCOLOR4US@_GLCOLOR4USV@_GLCOLORMASK@_GLCOLORMATERIAL@_GLCOLORPOINTER@_GLCOPYPIXELS@_GLCOPYTEXIMAGE1D@_GLCOPYTEXIMAGE2D@_GLCOPYTEXSUBIMAGE1D@_GLCOPYTEXSUBIMAGE2D@_GLCULLFACE@"+_
"_GLDELETELISTS@_GLDELETETEXTURES@_GLDEPTHFUNC@_GLDEPTHMASK@_GLDEPTHRANGE@_GLDISABLE@_GLDISABLECLIENTSTATE@_GLDRAWARRAYS@_GLDRAWBUFFER@_GLDRAWELEMENTS@_GLDRAWPIXELS@"+_
"_GLEDGEFLAG@_GLEDGEFLAGPOINTER@_GLEDGEFLAGV@_GLENABLE@_GLENABLECLIENTSTATE@_GLEND@_GLENDLIST@_GLEVALCOORD1D@_GLEVALCOORD1DV@_GLEVALCOORD1F@_GLEVALCOORD1FV@_GLEVALCOORD2D@_GLEVALCOORD2DV@_GLEVALCOORD2F@_GLEVALCOORD2FV@_GLEVALMESH1@_GLEVALMESH2@_GLEVALPOINT1@_GLEVALPOINT2@"+_
"_GLFEEDBACKBUFFER@_GLFINISH@_GLFLUSH@_GLFOGF@_GLFOGFV@_GLFOGI@_GLFOGIV@_GLFRONTFACE@_GLFRUSTUM@"+_
"_GLGENLISTS@_GLGENTEXTURES@_GLGETBOOLEANV@_GLGETCLIPPLANE@_GLGETDOUBLEV@_GLGETERROR@_GLGETFLOATV@_GLGETINTEGERV@_GLGETLIGHTFV@_GLGETLIGHTIV@_GLGETMAPDV@_GLGETMAPFV@_GLGETMAPIV@_GLGETMATERIALFV@_GLGETMATERIALIV@_GLGETPIXELMAPFV@_GLGETPIXELMAPUIV@_GLGETPIXELMAPUSV@_GLGETPOINTERV@_GLGETPOLYGONSTIPPLE@_GLGETSTRING@_GLGETTEXENVFV@_GLGETTEXENVIV@_GLGETTEXGENDV@_GLGETTEXGENFV@_GLGETTEXGENIV@_GLGETTEXIMAGE@_GLGETTEXLEVELPARAMETERFV@_GLGETTEXLEVELPARAMETERIV@_GLGETTEXPARAMETERFV@_GLGETTEXPARAMETERIV@"+_
"_GLHINT@"+_
"_GLINDEXD@_GLINDEXDV@_GLINDEXF@_GLINDEXFV@_GLINDEXI@_GLINDEXIV@_GLINDEXMASK@_GLINDEXPOINTER@_GLINDEXS@_GLINDEXSV@_GLINDEXUB@_GLINDEXUBV@_GLINITNAMES@_GLINTERLEAVEDARRAYS@_GLISENABLED@"+_
"_GLISLIST@_GLISTEXTURE@_GLLIGHTF@_GLLIGHTFV@_GLLIGHTI@_GLLIGHTIV@_GLLIGHTMODELF@_GLLIGHTMODELFV@_GLLIGHTMODELI@_GLLIGHTMODELIV@_GLLINESTIPPLE@_GLLINEWIDTH@_GLLISTBASE@_GLLOADIDENTITY@_GLLOADMATRIXD@_GLLOADMATRIXF@_GLLOADNAME@_GLLOGICOP@"+_
"_GLMAP1D@_GLMAP1F@_GLMAP2D@_GLMAP2F@_GLMAPGRID1D@_GLMAPGRID1F@_GLMAPGRID2D@_GLMAPGRID2F@_GLMATERIALF@_GLMATERIALFV@_GLMATERIALI@_GLMATERIALIV@_GLMATRIXMODE@_GLMULTMATRIXD@_GLMULTMATRIXF@"+_
"_GLNEWLIST@_GLNORMAL3B@_GLNORMAL3BV@_GLNORMAL3D@_GLNORMAL3DV@_GLNORMAL3F@_GLNORMAL3FV@_GLNORMAL3I@_GLNORMAL3IV@_GLNORMAL3S@_GLNORMAL3SV@_GLNORMALPOINTER@"+_
"_GLORTHO@"+_
"_GLPASSTHROUGH@_GLPIXELMAPFV@_GLPIXELMAPUIV@_GLPIXELMAPUSV@_GLPIXELSTOREF@_GLPIXELSTOREI@_GLPIXELTRANSFERF@_GLPIXELTRANSFERI@_GLPIXELZOOM@_GLPOINTSIZE@_GLPOLYGONMODE@_GLPOLYGONOFFSET@_GLPOLYGONSTIPPLE@_GLPOPATTRIB@_GLPOPCLIENTATTRIB@_GLPOPMATRIX@_GLPOPNAME@_GLPRIORITIZETEXTURES@_GLPUSHATTRIB@_GLPUSHCLIENTATTRIB@_GLPUSHMATRIX@_GLPUSHNAME@"+_
"_GLRASTERPOS2D@_GLRASTERPOS2DV@_GLRASTERPOS2F@_GLRASTERPOS2FV@_GLRASTERPOS2I@_GLRASTERPOS2IV@_GLRASTERPOS2S@_GLRASTERPOS2SV@_GLRASTERPOS3D@_GLRASTERPOS3DV@_GLRASTERPOS3F@_GLRASTERPOS3FV@_GLRASTERPOS3I@_GLRASTERPOS3IV@_GLRASTERPOS3S@_GLRASTERPOS3SV@_GLRASTERPOS4D@_GLRASTERPOS4DV@_GLRASTERPOS4F@_GLRASTERPOS4FV@_GLRASTERPOS4I@_GLRASTERPOS4IV@_GLRASTERPOS4S@_GLRASTERPOS4SV@_GLREADBUFFER@_GLREADPIXELS@_GLRECTD@_GLRECTDV@_GLRECTF@_GLRECTFV@_GLRECTI@_GLRECTIV@_GLRECTS@_GLRECTSV@_GLRENDER@_GLRENDERMODE@_GLROTATED@_GLROTATEF@"+_
"_GLSCALED@_GLSCALEF@_GLSCISSOR@_GLSELECTBUFFER@_GLSHADEMODEL@_GLSTENCILFUNC@_GLSTENCILMASK@_GLSTENCILOP@"+_
"_GLTEXCOORD1D@_GLTEXCOORD1DV@_GLTEXCOORD1F@_GLTEXCOORD1FV@_GLTEXCOORD1I@_GLTEXCOORD1IV@_GLTEXCOORD1S@_GLTEXCOORD1SV@_GLTEXCOORD2D@_GLTEXCOORD2DV@_GLTEXCOORD2F@_GLTEXCOORD2FV@_GLTEXCOORD2I@_GLTEXCOORD2IV@_GLTEXCOORD2S@_GLTEXCOORD2SV@_GLTEXCOORD3D@_GLTEXCOORD3DV@_GLTEXCOORD3F@_GLTEXCOORD3FV@_GLTEXCOORD3I@_GLTEXCOORD3IV@_GLTEXCOORD3S@_GLTEXCOORD3SV@_GLTEXCOORD4D@_GLTEXCOORD4DV@_GLTEXCOORD4F@_GLTEXCOORD4FV@_GLTEXCOORD4I@_GLTEXCOORD4IV@_GLTEXCOORD4S@_GLTEXCOORD4SV@_GLTEXCOORDPOINTER@_GLTEXENVF@_GLTEXENVFV@_GLTEXENVI@_GLTEXENVIV@_GLTEXGEND@_GLTEXGENDV@_GLTEXGENF@_GLTEXGENFV@_GLTEXGENI@_GLTEXGENIV@_GLTEXIMAGE1D@_GLTEXIMAGE2D@_GLTEXPARAMETERF@_GLTEXPARAMETERFV@_GLTEXPARAMETERI@_GLTEXPARAMETERIV@_GLTEXSUBIMAGE1D@_GLTEXSUBIMAGE2D@_GLTRANSLATED@_GLTRANSLATEF@"+_
"_GLUPERSPECTIVE@"+_
"_GLVERTEX2D@_GLVERTEX2DV@_GLVERTEX2F@_GLVERTEX2FV@_GLVERTEX2I@_GLVERTEX2IV@_GLVERTEX2S@_GLVERTEX2SV@_GLVERTEX3D@_GLVERTEX3DV@_GLVERTEX3F@_GLVERTEX3FV@_GLVERTEX3I@_GLVERTEX3IV@_GLVERTEX3S@_GLVERTEX3SV@_GLVERTEX4D@_GLVERTEX4DV@_GLVERTEX4F@_GLVERTEX4FV@_GLVERTEX4I@_GLVERTEX4IV@_GLVERTEX4S@_GLVERTEX4SV@_GLVERTEXPOINTER@_GLVIEWPORT@"+_
"_GREEN@_GREEN32@"+_
"_HEIGHT@_HYPOT@"+_
"_ICON@_INCLERRORFILE$@_INCLERRORLINE@_INFLATE$@_INPUTBOX$@_INSTRREV@_INTEGER64@"+_
"_KEEPBACKGROUND@_KEYCLEAR@_KEYDOWN@_KEYHIT@"+_
"_LASTAXIS@_LASTBUTTON@_LASTHANDLER@_LASTWHEEL@_LIMIT@_LOADFONT@_LOADIMAGE@"+_
"_MAPTRIANGLE@_MAPUNICODE@_MD5$@_MEM@_MEMCOPY@_MEMELEMENT@_MEMEXISTS@_MEMFILL@_MEMFREE@_MEMGET@_MEMIMAGE@_MEMNEW@_MEMPUT@_MEMSOUND@_MESSAGEBOX@_MIDISOUNDBANK@_MK$@_MOUSEBUTTON@_MOUSEHIDE@_MOUSEINPUT@_MOUSEMOVE@_MOUSEMOVEMENTX@_MOUSEMOVEMENTY@_MOUSEPIPEOPEN@_MOUSESHOW@_MOUSEWHEEL@_MOUSEX@_MOUSEY@"+_
"_NEGATE@_NEWHANDLER@_NEWIMAGE@_NOTIFYPOPUP@_NUMLOCK@"+_
"_OFFSET@_ONLYBACKGROUND@_OPENCLIENT@_OPENCONNECTION@_OPENFILEDIALOG$@_OPENHOST@_ORELSE@_OS$@"+_
"_PALETTECOLOR@_PI@_PIXELSIZE@_PRESERVE@_PRINTIMAGE@_PRINTMODE@_PRINTSTRING@_PRINTWIDTH@_PUTIMAGE@"+_
"_R2D@_R2G@_READBIT@_READFILE$@_RED@_RED32@_RESETBIT@_RESIZE@_RESIZEHEIGHT@_RESIZEWIDTH@_RGB@_RGB32@_RGBA@_RGBA32@_ROL@_ROR@_ROUND@"+_
"_SAVEFILEDIALOG$@_SAVEIMAGE@_SCALEDHEIGHT@_SCALEDWIDTH@_SCREENCLICK@_SCREENEXISTS@_SCREENHIDE@_SCREENICON@_SCREENIMAGE@_SCREENMOVE@_SCREENPRINT@_SCREENSHOW@_SCREENX@_SCREENY@_SCROLLLOCK@_SEC@_SECH@_SELECTFOLDERDIALOG$@_SETALPHA@_SETBIT@_SHELLHIDE@_SHL@_SHR@_SINH@_SMOOTH@_SNDBAL@_SNDCLOSE@_SNDCOPY@_SNDGETPOS@_SNDLEN@_SNDLIMIT@_SNDLOOP@_SNDNEW@_SNDOPEN@_SNDOPENRAW@_SNDPAUSE@_SNDPAUSED@_SNDPLAY@_SNDPLAYCOPY@_SNDPLAYFILE@_SNDPLAYING@_SNDRATE@_SNDRAW@_SNDRAWDONE@_SNDRAWLEN@_SNDSETPOS@_SNDSTOP@_SNDVOL@_SOURCE@_STARTDIR$@_STATUSCODE@_STRCMP@_STRICMP@"+_
"_TANH@_TITLE@_TITLE$@_TOGGLEBIT@_TOTALDROPPEDFILES@_TRIM$@"+_
"_UCHARPOS@_UFONTHEIGHT@_ULINESPACING@_UNSIGNED@_UPRINTSTRING@_UPRINTWIDTH@"+_
"_WHEEL@_WIDTH@_WINDOWHANDLE@_WINDOWHASFOCUS@_WRITEFILE@"

CONST FALSE = 0, TRUE = -1

CONST ASCII_TAB = 9
CONST ASCII_LF = 10
CONST ASCII_VTAB = 11
CONST ASCII_FF = 12
CONST ASCII_CR = 13
CONST ASCII_EOF = 0 'Prefer NUL over ^Z for this purpose as some people embed ^Z in their programs
CONST ASCII_QUOTE = 34

CONST TOK_EOF = 1
CONST TOK_NEWLINE = 2
CONST TOK_WORD = 3
CONST TOK_METACMD = 6
CONST TOK_COMMENT = 7
CONST TOK_STRING = 8
CONST TOK_DATA = 9
CONST TOK_PUNCTUATION = 11
CONST TOK_COLON = 15

CONST STATE_BEGIN = 1
CONST STATE_METACMD = 3
CONST STATE_WORD = 4
CONST STATE_COMMENT = 5
CONST STATE_STRING = 6
CONST STATE_DATA = 7
CONST STATE_NEWLINE = 12
CONST STATE_NEWLINE_WIN = 13

TYPE token_t
    t AS LONG 'TOK_ type
    c AS STRING 'Content
    uc AS STRING 'Content in UPPERCASE for comparisons
    spaces AS STRING 'Any whitespace characters detected before the content
END TYPE
DIM SHARED token AS token_t

REDIM SHARED prefix_keywords$(1) 'Stored without the prefix
REDIM SHARED prefix_colors$(0)
REDIM SHARED include_queue$(0)
DIM SHARED exedir$
DIM SHARED input_content$, current_include
DIM SHARED line_count, column_count
DIM SHARED next_chr_idx, tk_state
DIM SHARED noprefix_detected
DIM SHARED in_udt, in_declare_library

exedir$ = _CWD$
CHDIR _STARTDIR$

build_keyword_list

IF _COMMANDCOUNT = 0 THEN
    _SCREENSHOW
    PRINT "$NOPREFIX remover"
    PRINT "Files will be backed up before conversion."
    PRINT "Run this program with a file as a command-line argument or enter a file now"
    PRINT "File name: ";
    LINE INPUT include_queue$(0)
ELSE
    _DEST _CONSOLE
    include_queue$(0) = COMMAND$(1)
END IF

DO
    load_prepass_file include_queue$(current_include)
    prepass
    current_include = current_include + 1
LOOP WHILE current_include <= UBOUND(include_queue$)
IF NOT noprefix_detected THEN
    PRINT "Program does not use $NOPREFIX, no changes made"
    IF _COMMANDCOUNT = 0 THEN END ELSE SYSTEM
END IF

PRINT "Found"; UBOUND(include_queue$); "$INCLUDE file(s)"
current_include = 0
DO
    load_file include_queue$(current_include)
    DO
        process_logical_line
    LOOP WHILE token.t <> TOK_EOF
    CLOSE #2
    current_include = current_include + 1
LOOP WHILE current_include <= UBOUND(include_queue$)
PRINT "Conversion complete"
IF _COMMANDCOUNT = 0 THEN END ELSE SYSTEM

SUB load_prepass_file (filename$)
    PRINT "Analysing " + filename$
    input_content$ = _READFILE$(filename$) + CHR$(ASCII_EOF)
    rewind
END SUB

SUB prepass
    DO
        next_token_raw
        SELECT CASE token.t
            CASE TOK_METACMD
                SELECT CASE token.uc
                    CASE "$NOPREFIX"
                        noprefix_detected = TRUE
                    CASE "$COLOR:0"
                        build_color0_list
                    CASE "$COLOR:32"
                        build_color32_list
                END SELECT
            CASE TOK_WORD
                SELECT CASE token.uc
                    CASE "DATA"
                        tk_state = STATE_DATA
                    CASE "REM"
                        tk_state = STATE_COMMENT
                END SELECT
            CASE TOK_COMMENT
                process_maybe_include
            CASE TOK_NEWLINE
                line_count = line_count + 1
                column_count = 0
            CASE TOK_EOF
                EXIT DO
        END SELECT
    LOOP
END SUB

SUB load_file (filename$)
    DIM ext, backup$
    ext = _INSTRREV(filename$, ".")
    IF ext > 0 THEN
        backup$ = LEFT$(filename$, ext - 1) + "-noprefix" + MID$(filename$, ext)
    ELSE
        backup$ = filename$ + "-noprefix"
    END IF
    NAME filename$ AS backup$
    PRINT "Moved " + filename$ + " to backup " + backup$
    PRINT "Converting " + filename$
    input_content$ = _READFILE$(backup$) + CHR$(ASCII_EOF)
    OPEN filename$ FOR BINARY AS #2
    rewind
END SUB

SUB process_maybe_include
    DIM s$, path$, open_quote, close_quote
    s$ = token.c
    IF LEFT$(s$, 1) = "'" THEN s$ = MID$(s$, 2)
    s$ = LTRIM$(s$)
    IF UCASE$(LEFT$(s$, 8)) <> "$INCLUDE" THEN EXIT SUB
    open_quote = INSTR(s$, "'")
    close_quote = INSTR(open_quote + 1, s$, "'")
    path$ = MID$(s$, open_quote + 1, close_quote - open_quote - 1)
    queue_include path$
END SUB

SUB queue_include (given_path$)
    DIM current_path$, path$, i
    IF is_absolute_path(given_path$) THEN
        IF NOT _FILEEXISTS(given_path$) THEN
            PRINT "WARNING: cannot locate included file '" + given_path$ + "'"
            EXIT SUB
        END IF
        path$ = given_path$
    ELSE
        current_path$ = dir_name$(include_queue$(current_include))
        'First check relative to path of current file
        IF _FILEEXISTS(current_path$ + "/" + given_path$) THEN
            path$ = current_path$ + "/" + given_path$
            'Next try relative to converter TODO: Change to relative to compiler
        ELSEIF _FILEEXISTS(exedir$ + "/" + given_path$) THEN
            path$ = exedir$ + "/" + given_path$
        ELSE
            PRINT "WARNING: cannot locate included file '" + given_path$ + "'"
            EXIT SUB
        END IF
    END IF
    FOR i = 0 TO UBOUND(include_queue$)
        IF include_queue$(i) = path$ THEN EXIT SUB
    NEXT i
    i = UBOUND(include_queue$)
    REDIM _PRESERVE include_queue$(i + 1)
    include_queue$(i + 1) = path$
END SUB

SUB rewind
    line_count = 1
    column_count = 0
    next_chr_idx = 1
    tk_state = STATE_BEGIN
    token.t = 0
    token.c = ""
    token.uc = ""
END SUB

SUB build_keyword_list
    DIM i, j, keyword$
    i = 1
    FOR j = 1 TO LEN(KEYWORDS)
        IF ASC(KEYWORDS, j) = ASC("@") THEN
            IF ASC(keyword$) = ASC("_") THEN
                IF i > UBOUND(prefix_keywords$) THEN REDIM _PRESERVE prefix_keywords$(UBOUND(prefix_keywords$) * 2)
                prefix_keywords$(i) = MID$(keyword$, 2)
                IF i > 1 AND _STRCMP(prefix_keywords$(i), prefix_keywords$(i - 1)) <> 1 THEN
                    PRINT "Internal error: " + keyword$ + " out of order"
                    END
                END IF
                i = i + 1
            END IF
            keyword$ = ""
        ELSE
            keyword$ = keyword$ + MID$(KEYWORDS, j, 1)
        END IF
    NEXT j
    REDIM _PRESERVE prefix_keywords$(i - 1)
END SUB

SUB build_color0_list
    REDIM prefix_colors$(4)
    prefix_colors$(1) = "NP_BLUE"
    prefix_colors$(2) = "NP_GREEN"
    prefix_colors$(3) = "NP_RED"
    prefix_colors$(4) = "NP_BLINK"
END SUB

SUB build_color32_list
    REDIM prefix_colors$(3)
    prefix_colors$(1) = "NP_BLUE"
    prefix_colors$(2) = "NP_GREEN"
    prefix_colors$(3) = "NP_RED"
END SUB

SUB process_logical_line
    next_token
    SELECT CASE token.t
        CASE TOK_METACMD
            SELECT CASE token.uc
                CASE "$NOPREFIX"
                    'Keep remenant of $noprefix so line numbers are not changed
                    token.c = "'" + token.c + " removed here"
            END SELECT
        CASE TOK_WORD
            IF in_udt AND token.uc = "END" THEN
                in_udt = FALSE
                in_declare_library = FALSE
            ELSEIF in_udt THEN
                'In a UDT definition the field name is never a keyword
                next_token
            ELSE
                SELECT CASE token.uc
                    CASE "SUB", "FUNCTION"
                        IF in_declare_library THEN process_declare_library_def
                    CASE "TYPE"
                        in_udt = TRUE
                    CASE "DATA"
                        tk_state = STATE_DATA
                    CASE "DECLARE"
                        process_declare
                    CASE "PUT"
                        process_put
                    CASE "SCREENMOVE", "_SCREENMOVE"
                        process_screenmove
                    CASE "OPTION"
                        process_option
                    CASE "FULLSCREEN", "_FULLSCREEN"
                        process_fullscreen
                    CASE "ALLOWFULLSCREEN", "_ALLOWFULLSCREEN"
                        process_allowfullscreen
                    CASE "RESIZE", "_RESIZE"
                        process_resize
                    CASE "GLRENDER", "_GLRENDER"
                        process_glrender
                    CASE "DISPLAYORDER", "_DISPLAYORDER"
                        process_displayorder
                    CASE "EXIT"
                        next_token 'in statement position this is EXIT SUB etc.
                    CASE "FPS", "_FPS"
                        process_fps
                    CASE "CLEARCOLOR", "_CLEARCOLOR"
                        process_clearcolor
                    CASE "MAPTRIANGLE", "_MAPTRIANGLE"
                        process_maptriangle
                    CASE "DEPTHBUFFER", "_DEPTHBUFFER"
                        process_depthbuffer
                    CASE "WIDTH"
                        next_token 'in statement position this is the set-columns command
                    CASE "SHELL"
                        process_shell
                    CASE "CAPSLOCK", "_CAPSLOCK", "SCROLLLOCK", "_SCROLLLOCK", "NUMLOCK", "_NUMLOCK"
                        process_keylock
                    CASE "CONSOLECURSOR", "_CONSOLECURSOR"
                        process_consolecursor
                END SELECT
            END IF
    END SELECT
    process_rest_of_line
END SUB

SUB process_declare
    next_token
    IF token.uc = "SUB" OR token.uc = "FUNCTION" THEN
        WHILE NOT line_end
            next_token
        WEND
    ELSEIF token.uc = "LIBRARY" THEN
        in_declare_library = TRUE
    END IF
END SUB

SUB process_declare_library_def
    next_token
    WHILE token.uc <> "(" AND NOT line_end
        next_token
    WEND
    WHILE token.uc <> ")" AND NOT line_end
        next_token
        IF token.uc = "BYVAL" THEN next_token
        next_token 'Skip argument name
        skip_expr
    WEND
END SUB

SUB process_put
    next_token
    IF token.uc = "STEP" THEN next_token
    IF token.c = "(" THEN
        skip_parens 'Coordinates
        next_token ' ,
        next_token 'Array name
        IF line_end THEN EXIT SUB
        skip_parens 'Array index
        IF line_end THEN EXIT SUB
        next_token ' ,
        IF line_end THEN EXIT SUB
        IF token.uc = "CLIP" THEN add_prefix
    END IF
END SUB

SUB process_screenmove
    add_prefix
    next_token
    IF line_end THEN EXIT SUB
    IF token.uc = "MIDDLE" THEN add_prefix
END SUB

SUB process_option
    next_token
    IF token.uc = "EXPLICITARRAY" THEN add_prefix
END SUB

SUB process_fullscreen
    add_prefix
    next_token
    IF line_end THEN EXIT SUB
    IF token.c <> "," THEN
        add_prefix
        next_token
        IF line_end THEN EXIT SUB
    END IF
    next_token ' ,
    add_prefix
END SUB

SUB process_allowfullscreen
    add_prefix
    next_token
    IF line_end THEN EXIT SUB
    IF token.c <> "," THEN
        add_prefix
        next_token
        IF line_end THEN EXIT SUB
    END IF
    next_token ' ,
    add_prefix
END SUB

SUB process_resize
    add_prefix
    next_token
    IF token.c = "(" OR line_end THEN EXIT SUB
    IF token.c <> "," THEN next_token
    IF line_end THEN EXIT SUB
    next_token
    add_prefix
END SUB

SUB process_glrender
    add_prefix
    next_token
    add_prefix
END SUB

SUB process_displayorder
    add_prefix
    next_token
    WHILE NOT line_end
        IF token.c <> "," THEN add_prefix
        next_token
    WEND
END SUB

SUB process_fps
    add_prefix
    next_token
    IF token.uc = "AUTO" THEN add_prefix
END SUB

SUB process_clearcolor
    add_prefix
    next_token
    IF token.uc = "NONE" THEN add_prefix
END SUB

SUB process_maptriangle
    add_prefix
    next_token
    IF token.uc = "CLOCKWISE" OR token.uc = "ANTICLOCKWISE" THEN add_prefix
    IF token.uc = "_CLOCKWISE" OR token.uc = "_ANTICLOCKWISE" THEN next_token
    IF token.uc = "SEAMLESS" THEN add_prefix
    IF token.uc = "_SEAMLESS" THEN next_token
    DO
        maybe_add_prefix
        next_token
    LOOP WHILE token.uc <> "TO"
    next_token
    skip_parens
    next_token ' -
    skip_parens
    next_token ' -
    skip_parens
    IF line_end THEN EXIT SUB
    next_token ' ,
    skip_expr
    IF line_end THEN EXIT SUB
    next_token ' ,
    add_prefix
END SUB

SUB process_depthbuffer
    add_prefix
    next_token
    IF token.uc = "CLEAR" THEN add_prefix
END SUB

SUB process_shell
    next_token
    IF line_end THEN EXIT SUB
    IF token.uc = "DONTWAIT" OR token.uc = "HIDE" THEN
        add_prefix
        next_token
    END IF
    IF line_end THEN EXIT SUB
    IF token.uc = "DONTWAIT" OR token.uc = "HIDE" THEN add_prefix
END SUB

SUB process_keylock
    add_prefix
    next_token
    IF token.uc = "TOGGLE" THEN add_prefix
END SUB

SUB process_consolecursor
    add_prefix
    next_token
    IF line_end THEN EXIT SUB
    IF token.uc = "SHOW" OR token.uc = "HIDE" THEN add_prefix
END SUB

SUB skip_parens
    DIM balance
    DO
        IF token.c = "(" THEN balance = balance + 1
        IF token.c = ")" THEN balance = balance - 1
        maybe_add_prefix
        next_token
    LOOP UNTIL balance = 0
END SUB

SUB skip_expr
    DIM balance
    DO UNTIL balance <= 0 AND (token.c = "," OR line_end)
        IF token.c = "(" THEN balance = balance + 1
        IF token.c = ")" THEN balance = balance - 1
        maybe_add_prefix
        next_token
    LOOP
END SUB

SUB add_prefix
    IF ASC(token.c) <> ASC("_") THEN
        token.c = "_" + token.c
        token.uc = "_" + token.uc
    END IF
END SUB

SUB maybe_add_prefix
    IF noprefix_detected AND token.t = TOK_WORD AND ASC(token.uc) <> ASC("_") _ANDALSO is_underscored THEN add_prefix
END SUB

FUNCTION line_end
    SELECT CASE token.t
        CASE TOK_WORD
            line_end = (token.uc = "REM")
        CASE TOK_COLON, TOK_COMMENT, TOK_NEWLINE
            line_end = TRUE
    END SELECT
END FUNCTION

FUNCTION is_underscored
    DIM i
    FOR i = 1 TO UBOUND(prefix_keywords$)
        IF token.uc = prefix_keywords$(i) THEN
            is_underscored = TRUE
            EXIT FUNCTION
        END IF
    NEXT i
END FUNCTION

SUB process_rest_of_line
    DIM i, base_word$
    DO
        SELECT CASE token.t
            CASE TOK_WORD
                SELECT CASE token.uc
                    CASE "REM"
                        tk_state = STATE_COMMENT
                    CASE "THEN"
                        EXIT SUB
                    CASE ELSE
                        IF noprefix_detected AND LEFT$(token.uc, 3) = "NP_" THEN
                            base_word$ = make_base_word$(token.uc)
                            FOR i = 1 TO UBOUND(prefix_colors$)
                                IF base_word$ = prefix_colors$(i) THEN
                                    token.c = MID$(token.c, 4)
                                    token.uc = MID$(token.uc, 4)
                                    EXIT FOR
                                END IF
                            NEXT i
                            EXIT SELECT
                        END IF
                        maybe_add_prefix
                END SELECT
            CASE TOK_COLON
                EXIT SUB
            CASE TOK_NEWLINE
                line_count = line_count + 1
                column_count = 0
                EXIT SUB
            CASE TOK_EOF
                put_out
                EXIT SUB
            CASE ELSE
        END SELECT
        next_token
    LOOP
END SUB

SUB put_out
    PUT #2, , token.spaces
    PUT #2, , token.c
END SUB

FUNCTION make_base_word$ (s$)
    DIM i
    FOR i = 1 TO LEN(s$)
        SELECT CASE ASC(s$, i)
            CASE ASC("A") TO ASC("Z"), ASC("a") TO ASC("z"), ASC("0") TO ASC("9"), ASC("_")
            CASE ELSE
                EXIT FOR
        END SELECT
    NEXT i
    make_base_word$ = LEFT$(s$, i - 1)
END FUNCTION

SUB next_token
    IF token.t > 0 THEN put_out
    next_token_raw
    WHILE token.t = TOK_WORD AND token.c = "_"
        put_out
        next_token_raw
        IF token.t <> TOK_NEWLINE THEN EXIT SUB
        line_count = line_count + 1
        column_count = 0
        put_out
        next_token_raw
    WEND
END SUB

SUB next_token_raw
    DIM c, return_token, token_content$, spaces$, unread
    DO
        c = ASC(input_content$, next_chr_idx)
        next_chr_idx = next_chr_idx + 1
        column_count = column_count + 1
        SELECT CASE tk_state
            CASE STATE_BEGIN
                SELECT CASE c
            case asc("A") to asc("Z"), asc("a") to asc("z"), asc("_"), asc("0") to asc("9"), _
                 asc("&"), asc("."), asc("?")
                        tk_state = STATE_WORD
                    CASE ASC("$")
                        tk_state = STATE_METACMD
                    CASE ASC(":")
                        return_token = TOK_COLON
            case asc("^"), asc("*"), asc("-"), asc("+"), asc("="), asc("\"), asc("#"), _
                 asc(";"), asc("<"), asc(">"), asc("/"), asc("("), asc(")"), asc(",")
                        return_token = TOK_PUNCTUATION
                    CASE ASCII_QUOTE
                        tk_state = STATE_STRING
                    CASE ASC("'")
                        tk_state = STATE_COMMENT
                    CASE ASC(" "), ASCII_TAB, ASCII_VTAB
                        spaces$ = spaces$ + CHR$(c)
                        _CONTINUE
                    CASE ASCII_CR, ASCII_LF, ASCII_EOF
                        tk_state = STATE_NEWLINE
                        unread = TRUE
                    CASE ELSE
                        'Likely non-ascii special character
                        syntax_warning CHR$(c)
                        tk_state = STATE_WORD
                END SELECT
            CASE STATE_METACMD
                SELECT CASE c
                    CASE ASCII_CR, ASCII_LF, ASCII_EOF
                        tk_state = STATE_NEWLINE
                        return_token = TOK_METACMD
                        unread = TRUE
                END SELECT
            CASE STATE_WORD
                SELECT CASE c
            case asc("A") to asc("Z"), asc("a") to asc("z"), asc("_"), asc("0") to asc("9"), _
                 asc("`"), asc("~"), asc("!"), asc("#"), asc("$"), asc("%"), asc("&"), asc("."), asc("?")
                        'Continue
                    CASE ELSE
                        tk_state = STATE_BEGIN
                        return_token = TOK_WORD
                        unread = TRUE
                END SELECT
            CASE STATE_COMMENT
                SELECT CASE c
                    CASE ASCII_CR, ASCII_LF, ASCII_EOF
                        tk_state = STATE_NEWLINE
                        return_token = TOK_COMMENT
                        unread = TRUE
                END SELECT
            CASE STATE_STRING
                SELECT CASE c
                    CASE ASCII_QUOTE
                        tk_state = STATE_BEGIN
                        return_token = TOK_STRING
                    CASE ASCII_CR, ASCII_LF, ASCII_EOF
                        tk_state = STATE_NEWLINE
                        return_token = TOK_STRING
                        unread = TRUE
                END SELECT
            CASE STATE_DATA
                SELECT CASE c
                    CASE ASCII_CR, ASCII_LF, ASCII_EOF
                        tk_state = STATE_NEWLINE
                        return_token = TOK_DATA
                        unread = TRUE
                END SELECT
            CASE STATE_NEWLINE
                SELECT CASE c
                    CASE ASCII_LF
                        tk_state = STATE_BEGIN
                        return_token = TOK_NEWLINE
                    CASE ASCII_CR
                        tk_state = STATE_NEWLINE_WIN
                    CASE ASCII_EOF
                        return_token = TOK_EOF
                        unread = TRUE 'Do not insert EOF character
                    CASE ELSE
                        'Should never happen
                        syntax_warning CHR$(c)
                        tk_state = STATE_BEGIN
                        return_token = TOK_NEWLINE
                        unread = TRUE
                END SELECT
            CASE STATE_NEWLINE_WIN
                SELECT CASE c
                    CASE ASCII_LF
                        tk_state = STATE_BEGIN
                        return_token = TOK_NEWLINE
                    CASE ELSE
                        tk_state = STATE_BEGIN
                        return_token = TOK_NEWLINE
                        unread = TRUE
                END SELECT
        END SELECT

        IF unread THEN
            next_chr_idx = next_chr_idx - 1
            unread = FALSE
        ELSE
            token_content$ = token_content$ + CHR$(c)
        END IF

        IF return_token THEN
            token.t = return_token
            token.c = token_content$
            token.uc = UCASE$(token_content$)
            token.spaces = spaces$
            EXIT SUB
        END IF
    LOOP
END SUB

SUB syntax_warning (unexpected$)
    PRINT "WARNING: Line"; line_count; "column"; column_count;
    PRINT "State"; tk_state;
    PRINT "Unexpected "; unexpected$
END SUB

'Get the directory component of a path
FUNCTION dir_name$ (path$)
    DIM s1, s2
    s1 = _INSTRREV(path$, "/")
    s2 = _INSTRREV(path$, "\")
    IF s1 > s2 THEN
        dir_name$ = LEFT$(path$, s1 - 1)
    ELSEIF s2 > s1 THEN
        dir_name$ = LEFT$(path$, s2 - 1)
    ELSE
        dir_name$ = "."
    END IF
END FUNCTION

FUNCTION is_absolute_path (path$)
    IF INSTR(_OS$, "WIN") THEN
        is_absolute_path = (MID$(path$, 2, 1) = ":" OR LEFT$(path$, 1) = "\" OR LEFT$(path$, 1) = "/")
    ELSE
        is_absolute_path = LEFT$(path$, 1) = "/"
    END IF
END FUNCTION

