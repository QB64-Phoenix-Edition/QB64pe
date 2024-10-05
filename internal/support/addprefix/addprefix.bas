option _explicit
$screenhide
$console
deflng a-z

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

const FALSE = 0, TRUE = -1

const ASCII_TAB = 9
const ASCII_LF = 10
const ASCII_VTAB = 11
const ASCII_FF = 12
const ASCII_CR = 13
const ASCII_EOF = 0 'Prefer NUL over ^Z for this purpose as some people embed ^Z in their programs
const ASCII_QUOTE = 34

const TOK_EOF = 1
const TOK_NEWLINE = 2
const TOK_WORD = 3
const TOK_METACMD = 6
const TOK_COMMENT = 7
const TOK_STRING = 8
const TOK_DATA = 9
const TOK_PUNCTUATION = 11
const TOK_COLON = 15

const STATE_BEGIN = 1
const STATE_METACMD = 3
const STATE_WORD = 4
const STATE_COMMENT = 5
const STATE_STRING = 6
const STATE_DATA = 7
const STATE_NEWLINE = 12
const STATE_NEWLINE_WIN = 13

type token_t
    t as long 'TOK_ type
    c as string 'Content
    uc as string 'Content in UPPERCASE for comparisons
    spaces as string 'Any whitespace characters detected before the content
end type
dim shared token as token_t

redim shared prefix_keywords$(1) 'Stored without the prefix
redim shared prefix_colors$(0)
redim shared include_queue$(0)
dim shared exedir$
dim shared input_content$, current_include
dim shared line_count, column_count
dim shared next_chr_idx, tk_state
dim shared noprefix_detected
dim shared in_udt, in_declare_library

exedir$ = _cwd$
chdir _startdir$

build_keyword_list

if _commandcount = 0 then
    _screenshow
    print "$NOPREFIX remover"
    print "Files will be backed up before conversion."
    print "Run this program with a file as a command-line argument or enter a file now"
    print "File name: ";
    line input include_queue$(0)
else
    _dest _console
    include_queue$(0) = command$(1)
end if

do
    load_prepass_file include_queue$(current_include)
    prepass
    current_include = current_include + 1
loop while current_include <= ubound(include_queue$)
if not noprefix_detected then
    print "Program does not use $NOPREFIX, no changes made"
    if _commandcount = 0 then end else system
end if

print "Found"; ubound(include_queue$); "$INCLUDE file(s)"
current_include = 0
do
    load_file include_queue$(current_include)
    do
        process_logical_line
    loop while token.t <> TOK_EOF
    close #2
    current_include = current_include + 1
loop while current_include <= ubound(include_queue$)
print "Conversion complete"
if _commandcount = 0 then end else system

sub load_prepass_file (filename$)
    print "Analysing " + filename$
    input_content$ = _readfile$(filename$) + chr$(ASCII_EOF)
    rewind
end sub

sub prepass
    do
        next_token_raw
        select case token.t
        case TOK_METACMD
            select case token.uc
            case "$NOPREFIX"
                noprefix_detected = TRUE
            case "$COLOR:0"
                build_color0_list
            case "$COLOR:32"
                build_color32_list
            end select
        case TOK_WORD
            select case token.uc
            case "DATA"
                tk_state = STATE_DATA
            case "REM"
                tk_state = STATE_COMMENT
            end select
        case TOK_COMMENT
            process_maybe_include
        case TOK_NEWLINE
            line_count = line_count + 1
            column_count = 0
        case TOK_EOF
            exit do
        end select
    loop
end sub

sub load_file (filename$)
    dim ext, backup$
    ext = _instrrev(filename$, ".")
    if ext > 0 then
        backup$ = left$(filename$, ext - 1) + "-noprefix" + mid$(filename$, ext)
    else
        backup$ = filename$ + "-noprefix"
    end if
    name filename$ as backup$
    print "Moved " + filename$ + " to backup " + backup$
    print "Converting " + filename$
    input_content$ = _readfile$(backup$) + chr$(ASCII_EOF)
    open filename$ for binary as #2
    rewind
end sub

sub process_maybe_include
    dim s$, path$, open_quote, close_quote
    s$ = token.c
    if left$(s$, 1) = "'" then s$ = mid$(s$, 2)
    s$ = ltrim$(s$)
    if ucase$(left$(s$, 8)) <> "$INCLUDE" then exit sub
    open_quote = instr(s$, "'")
    close_quote = instr(open_quote + 1, s$, "'")
    path$ = mid$(s$, open_quote + 1, close_quote - open_quote - 1)
    queue_include path$
end sub

sub queue_include (given_path$)
    dim current_path$, path$, i
    if is_absolute_path(given_path$) then
        if not _fileexists(given_path$) then
            print "WARNING: cannot locate included file '" + given_path$ + "'"
            exit sub
        end if
        path$ = given_path$
    else
        current_path$ = dir_name$(include_queue$(current_include))
        'First check relative to path of current file
        if _fileexists(current_path$ + "/" + given_path$) then
            path$ = current_path$ + "/" + given_path$
        'Next try relative to converter TODO: Change to relative to compiler
        elseif _fileexists(exedir$ + "/" + given_path$) then
            path$ = exedir$ + "/" + given_path$
        else
            print "WARNING: cannot locate included file '" + given_path$ + "'"
            exit sub
        end if
    end if
    for i = 0 to ubound(include_queue$)
        if include_queue$(i) = path$ then exit sub
    next i
    i = ubound(include_queue$)
    redim _preserve include_queue$(i + 1)
    include_queue$(i + 1) = path$
end sub

sub rewind
    line_count = 1
    column_count = 0
    next_chr_idx = 1
    tk_state = STATE_BEGIN
    token.t = 0
    token.c = ""
    token.uc = ""
end sub

sub build_keyword_list
    dim i, j, keyword$
    i = 1
    for j = 1 to len(KEYWORDS)
        if asc(KEYWORDS, j) = asc("@") then
            if asc(keyword$) = asc("_") then
                if i > ubound(prefix_keywords$) then redim _preserve prefix_keywords$(ubound(prefix_keywords$) * 2)
                prefix_keywords$(i) = mid$(keyword$, 2)
                if i > 1 and _strcmp(prefix_keywords$(i), prefix_keywords$(i - 1)) <> 1 then
                    print "Internal error: " + keyword$ + " out of order"
                    end
                end if
                i = i + 1
            end if
            keyword$ = ""
        else
            keyword$ = keyword$ + mid$(KEYWORDS, j, 1)
        end if
    next j
    redim _preserve prefix_keywords$(i - 1)
end sub

sub build_color0_list
    redim prefix_colors$(4)
    prefix_colors$(1) = "NP_BLUE"
    prefix_colors$(2) = "NP_GREEN"
    prefix_colors$(3) = "NP_RED"
    prefix_colors$(4) = "NP_BLINK"
end sub

sub build_color32_list
    redim prefix_colors$(3)
    prefix_colors$(1) = "NP_BLUE"
    prefix_colors$(2) = "NP_GREEN"
    prefix_colors$(3) = "NP_RED"
end sub

sub process_logical_line
    next_token
    select case token.t
    case TOK_METACMD
        select case token.uc
        case "$NOPREFIX"
            'Keep remenant of $noprefix so line numbers are not changed
            token.c = "'" + token.c + " removed here"
        end select
    case TOK_WORD
        if in_udt and token.uc = "END" then
            in_udt = FALSE
            in_declare_library = FALSE
        elseif in_udt then
            'In a UDT definition the field name is never a keyword
            next_token
        else
            select case token.uc
            case "SUB", "FUNCTION"
                if in_declare_library then process_declare_library_def
            case "TYPE"
                in_udt = TRUE
            case "DATA"
                tk_state = STATE_DATA
            case "DECLARE"
                process_declare
            case "PUT"
                process_put
            case "SCREENMOVE", "_SCREENMOVE"
                process_screenmove
            case "OPTION"
                process_option
            case "FULLSCREEN", "_FULLSCREEN"
                process_fullscreen
            case "ALLOWFULLSCREEN", "_ALLOWFULLSCREEN"
                process_allowfullscreen
            case "RESIZE", "_RESIZE"
                process_resize
            case "GLRENDER", "_GLRENDER"
                process_glrender
            case "DISPLAYORDER", "_DISPLAYORDER"
                process_displayorder
            case "EXIT"
                next_token 'in statement position this is EXIT SUB etc.
            case "FPS", "_FPS"
                process_fps
            case "CLEARCOLOR", "_CLEARCOLOR"
                process_clearcolor
            case "MAPTRIANGLE", "_MAPTRIANGLE"
                process_maptriangle
            case "DEPTHBUFFER", "_DEPTHBUFFER"
                process_depthbuffer
            case "WIDTH"
                next_token 'in statement position this is the set-columns command
            case "SHELL"
                process_shell
            case "CAPSLOCK", "_CAPSLOCK", "SCROLLLOCK", "_SCROLLLOCK", "NUMLOCK", "_NUMLOCK"
                process_keylock
            case "CONSOLECURSOR", "_CONSOLECURSOR"
                process_consolecursor
            end select
        end if
    end select
    process_rest_of_line
end sub

sub process_declare
    next_token
    if token.uc = "SUB" or token.uc = "FUNCTION" then
        while not line_end
            next_token
        wend
    elseif token.uc = "LIBRARY" then
        in_declare_library = TRUE
    end if
end sub

sub process_declare_library_def
    next_token
    while token.uc <> "(" and not line_end
        next_token
    wend
    while token.uc <> ")" and not line_end
        next_token
        if token.uc = "BYVAL" then next_token
        next_token 'Skip argument name
        skip_expr
    wend
end sub

sub process_put
    next_token
    if token.uc = "STEP" then next_token
    if token.c = "(" then
        skip_parens 'Coordinates
        next_token ' ,
        next_token 'Array name
        if line_end then exit sub
        skip_parens 'Array index
        if line_end then exit sub
        next_token ' ,
        if line_end then exit sub
        if token.uc = "CLIP" then add_prefix
    end if
end sub

sub process_screenmove
    add_prefix
    next_token
    if line_end then exit sub
    if token.uc = "MIDDLE" then add_prefix
end sub

sub process_option
    next_token
    if token.uc = "EXPLICITARRAY" then add_prefix
end sub

sub process_fullscreen
    add_prefix
    next_token
    if line_end then exit sub
    if token.c <> "," then
        add_prefix
        next_token
        if line_end then exit sub
    end if
    next_token ' ,
    add_prefix
end sub

sub process_allowfullscreen
    add_prefix
    next_token
    if line_end then exit sub
    if token.c <> "," then
        add_prefix
        next_token
        if line_end then exit sub
    end if
    next_token ' ,
    add_prefix
end sub

sub process_resize
    add_prefix
    next_token
    if token.c = "(" or line_end then exit sub
    if token.c <> "," then next_token
    if line_end then exit sub
    next_token
    add_prefix
end sub

sub process_glrender
    add_prefix
    next_token
    add_prefix
end sub

sub process_displayorder
    add_prefix
    next_token
    while not line_end
        if token.c <> "," then add_prefix
        next_token
    wend
end sub

sub process_fps
    add_prefix
    next_token
    if token.uc = "AUTO" then add_prefix
end sub

sub process_clearcolor
    add_prefix
    next_token
    if token.uc = "NONE" then add_prefix
end sub

sub process_maptriangle
    add_prefix
    next_token
    if token.uc = "CLOCKWISE" or token.uc = "ANTICLOCKWISE" then add_prefix
    if token.uc = "_CLOCKWISE" or token.uc = "_ANTICLOCKWISE" then next_token
    if token.uc = "SEAMLESS" then add_prefix
    if token.uc = "_SEAMLESS" then next_token
    do
        maybe_add_prefix
        next_token
    loop while token.uc <> "TO"
    next_token
    skip_parens
    next_token ' -
    skip_parens
    next_token ' -
    skip_parens
    if line_end then exit sub
    next_token ' ,
    skip_expr
    if line_end then exit sub
    next_token ' ,
    add_prefix
end sub

sub process_depthbuffer
    add_prefix
    next_token
    if token.uc = "CLEAR" then add_prefix
end sub

sub process_shell
    next_token
    if line_end then exit sub
    if token.uc = "DONTWAIT" or token.uc = "HIDE" then
        add_prefix
        next_token
    end if
    if line_end then exit sub
    if token.uc = "DONTWAIT" or token.uc = "HIDE" then add_prefix
end sub

sub process_keylock
    add_prefix
    next_token
    if token.uc = "TOGGLE" then add_prefix
end sub

sub process_consolecursor
    add_prefix
    next_token
    if line_end then exit sub
    if token.uc = "SHOW" or token.uc = "HIDE" then add_prefix
end sub

sub skip_parens
    dim balance
    do
        if token.c = "(" then balance = balance + 1
        if token.c = ")" then balance = balance - 1
        maybe_add_prefix
        next_token
    loop until balance = 0
end sub

sub skip_expr
    dim balance
    do until balance <= 0 and (token.c = "," or line_end)
        if token.c = "(" then balance = balance + 1
        if token.c = ")" then balance = balance - 1
        maybe_add_prefix
        next_token
    loop
end sub

sub add_prefix
    if asc(token.c) <> asc("_") then
        token.c = "_" + token.c
        token.uc = "_" + token.uc
    end if
end sub

sub maybe_add_prefix
    if noprefix_detected and token.t = TOK_WORD and asc(token.uc) <> asc("_") _andalso is_underscored(token.c) then add_prefix
end sub

function line_end
    select case token.t
        case TOK_WORD
            line_end = (token.uc = "REM")
        case TOK_COLON, TOK_COMMENT, TOK_NEWLINE
            line_end = TRUE
    end select
end function

function is_underscored(s$)
    dim i
    for i = 1 to ubound(prefix_keywords$)
        if token.uc = prefix_keywords$(i) then
            is_underscored = TRUE
            exit function
        end if
    next i
end function

sub process_rest_of_line
    dim i, base_word$
    do
        select case token.t
        case TOK_WORD
            select case token.uc
            case "REM"
                tk_state = STATE_COMMENT
            case "THEN"
                exit sub
            case else
                if noprefix_detected and left$(token.uc, 3) = "NP_" then
                    base_word$ = make_base_word$(token.uc)
                    for i = 1 to ubound(prefix_colors$)
                        if base_word$ = prefix_colors$(i) then
                            token.c = mid$(token.c, 4)
                            token.uc = mid$(token.uc, 4)
                            exit for
                        end if
                    next i
                    exit select
                end if
                maybe_add_prefix
            end select
        case TOK_COLON
            exit sub
        case TOK_NEWLINE
            line_count = line_count + 1
            column_count = 0
            exit sub
        case TOK_EOF
            put_out
            exit sub
        case else
        end select
        next_token
    loop
end sub

sub put_out
    put #2, , token.spaces
    put #2, , token.c
end sub

function make_base_word$(s$)
    dim i
    for i = 1 to len(s$)
        select case asc(s$, i)
        case asc("A") to asc("Z"), asc("a") to asc("z"), asc("0") to asc("9"), asc("_")
        case else
            exit for
        end select
    next i
    make_base_word$ = left$(s$, i - 1)
end function

sub next_token
    if token.t > 0 then put_out
    next_token_raw
    while token.t = TOK_WORD and token.c = "_"
        put_out
        next_token_raw
        if token.t <> TOK_NEWLINE then exit sub
        line_count = line_count + 1
        column_count = 0
        put_out
        next_token_raw
    wend
end sub

sub next_token_raw
    dim c, return_token, token_content$, spaces$, unread
    do
        c = asc(input_content$, next_chr_idx)
        next_chr_idx = next_chr_idx + 1
        column_count = column_count + 1
        select case tk_state
        case STATE_BEGIN
            select case c
            case asc("A") to asc("Z"), asc("a") to asc("z"), asc("_"), asc("0") to asc("9"), _
                 asc("&"), asc("."), asc("?")
                tk_state = STATE_WORD
            case asc("$")
                tk_state = STATE_METACMD
            case asc(":")
                return_token = TOK_COLON
            case asc("^"), asc("*"), asc("-"), asc("+"), asc("="), asc("\"), asc("#"), _
                 asc(";"), asc("<"), asc(">"), asc("/"), asc("("), asc(")"), asc(",")
                return_token = TOK_PUNCTUATION
            case ASCII_QUOTE
                tk_state = STATE_STRING
            case asc("'")
                tk_state = STATE_COMMENT
            case asc(" "), ASCII_TAB, ASCII_VTAB
                spaces$ = spaces$ + chr$(c)
                _continue
            case ASCII_CR, ASCII_LF, ASCII_EOF
                tk_state = STATE_NEWLINE
                unread = TRUE
            case else
                'Likely non-ascii special character
                syntax_warning chr$(c)
                tk_state = STATE_WORD
            end select
        case STATE_METACMD
            select case c
            case ASCII_CR, ASCII_LF, ASCII_EOF
                tk_state = STATE_NEWLINE
                return_token = TOK_METACMD
                unread = TRUE
            end select
        case STATE_WORD
            select case c
            case asc("A") to asc("Z"), asc("a") to asc("z"), asc("_"), asc("0") to asc("9"), _
                 asc("`"), asc("~"), asc("!"), asc("#"), asc("$"), asc("%"), asc("&"), asc("."), asc("?")
                'Continue
            case else
                tk_state = STATE_BEGIN
                return_token = TOK_WORD
                unread = TRUE
            end select
        case STATE_COMMENT
            select case c
            case ASCII_CR, ASCII_LF, ASCII_EOF
                tk_state = STATE_NEWLINE
                return_token = TOK_COMMENT
                unread = TRUE
            end select
        case STATE_STRING
            select case c
            case ASCII_QUOTE
                tk_state = STATE_BEGIN
                return_token = TOK_STRING
            case ASCII_CR, ASCII_LF, ASCII_EOF
                tk_state = STATE_NEWLINE
                return_token = TOK_STRING
                unread = TRUE
            end select
        case STATE_DATA
            select case c
            case ASCII_CR, ASCII_LF, ASCII_EOF
                tk_state = STATE_NEWLINE
                return_token = TOK_DATA
                unread = TRUE
            end select
        case STATE_NEWLINE
            select case c
            case ASCII_LF
                tk_state = STATE_BEGIN
                return_token = TOK_NEWLINE
            case ASCII_CR
                tk_state = STATE_NEWLINE_WIN
            case ASCII_EOF
                return_token = TOK_EOF
                unread = TRUE 'Do not insert EOF character
            case else
                'Should never happen
                syntax_warning chr$(c)
                tk_state = STATE_BEGIN
                return_token = TOK_NEWLINE
                unread = TRUE
            end select
        case STATE_NEWLINE_WIN
            select case c
            case ASCII_LF
                tk_state = STATE_BEGIN
                return_token = TOK_NEWLINE
            case else
                tk_state = STATE_BEGIN
                return_token = TOK_NEWLINE
                unread = TRUE
            end select
        end select

        if unread then
            next_chr_idx = next_chr_idx - 1
            unread = FALSE
        else
            token_content$ = token_content$ + chr$(c)
        end if

        if return_token then
            token.t = return_token
            token.c = token_content$
            token.uc = ucase$(token_content$)
            token.spaces = spaces$
            exit function
        end if
    loop
end function

sub syntax_warning(unexpected$)
    print "WARNING: Line"; line_count; "column"; column_count;
    print "State"; tk_state;
    print "Unexpected "; unexpected$
end sub

'Get the directory component of a path
function dir_name$(path$)
    dim s1, s2
    s1 = _instrrev(path$, "/")
    s2 = _instrrev(path$, "\")
    if s1 > s2 then
        dir_name$ = left$(path$, s1 - 1)
    elseif s2 > s1 then
        dir_name$ = left$(path$, s2 - 1)
    else
        dir_name$ = "."
    end if
end function

function is_absolute_path(path$)
    if instr(_os$, "WIN") then
        is_absolute_path = (mid$(path$, 2, 1) = ":" or left$(path$, 1) = "\" or left$(path$, 1) = "/")
    else
        is_absolute_path = left$(path$, 1) = "/"
    end if
end function