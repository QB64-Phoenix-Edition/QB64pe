DIM SHARED Cache_Folder AS STRING
Cache_Folder$ = "internal\help"
IF INSTR(_OS$, "WIN") = 0 THEN Cache_Folder$ = "internal/help"
IF _DIREXISTS("internal") = 0 THEN GOTO NoInternalFolder
IF _DIREXISTS(Cache_Folder$) = 0 THEN MKDIR Cache_Folder$
DIM SHARED Help_sx, Help_sy, Help_cx, Help_cy
DIM SHARED Help_Select, Help_cx1, Help_cy1, Help_SelX1, Help_SelX2, Help_SelY1, Help_SelY2
DIM SHARED Help_MSelect
Help_sx = 1: Help_sy = 1: Help_cx = 1: Help_cy = 1
DIM SHARED Help_wx1, Help_wy1, Help_wx2, Help_wy2 'defines the text section of the help window on-screen
DIM SHARED Help_ww, Help_wh 'width & height of text region
DIM SHARED help_h, help_w 'width & height
DIM SHARED Help_Txt$ '[chr][col][link-byte1][link-byte2]
DIM SHARED Help_Txt_Len
DIM SHARED Help_Pos, Help_Wrap_Pos
DIM SHARED Help_Line$ 'index of first txt element of a line
DIM SHARED Help_Link$ 'the link info [sep][type:]...[sep]
DIM SHARED Help_Link_Sep$: Help_Link_Sep$ = CHR$(13)
DIM SHARED Help_LinkN, Help_LinkL '# of links, local link flag
'Link Types:
' PAGE:wikipagename
' SECT:dummylink (not processed, just to mark page sections as search targets for local links)
' EXTL:external link url
DIM SHARED Help_BG_Col
DIM SHARED Help_Col_Normal: Help_Col_Normal = 7
DIM SHARED Help_Col_Link: Help_Col_Link = 9
DIM SHARED Help_Col_Bold: Help_Col_Bold = 15
DIM SHARED Help_Col_Italic: Help_Col_Italic = 3
DIM SHARED Help_Col_Section: Help_Col_Section = 8
DIM SHARED Help_Bold, Help_Italic, Help_LinkTxt, Help_Heading
DIM SHARED Help_Underline, Help_ChkBlank
DIM SHARED Help_LockWrap, Help_LockParse
DIM SHARED Help_DList, Help_LIndent$
DIM SHARED Help_Center, Help_CIndent$
REDIM SHARED Help_LineLen(1)
REDIM SHARED Back$(1)
REDIM SHARED Back_Name$(1)
TYPE Help_Back_Type
    sx AS LONG
    sy AS LONG
    cx AS LONG
    cy AS LONG
END TYPE
REDIM SHARED Help_Back(1) AS Help_Back_Type
Back$(1) = "QB64 Help Menu"
Back_Name$(1) = Back2BackName$(Back$(1))
Help_Back(1).sx = 1: Help_Back(1).sy = 1: Help_Back(1).cx = 1: Help_Back(1).cy = 1
DIM SHARED Help_Back_Pos
Help_Back_Pos = 1
DIM SHARED Help_Search_Time AS DOUBLE
DIM SHARED Help_Search_Str AS STRING
DIM SHARED Help_PageLoaded AS STRING
DIM SHARED Help_Recaching, Help_IgnoreCache

'Entity replacements
'(for non HTML chars only, ie. no &amp; &lt; &gt; &quot; which are handled in SUB Wiki$ directly)
TYPE wikiEntityReplace
    enti AS STRING * 8 '= entity as supported (ie. name where available, else as decimal number)
    repl AS STRING * 8 '= replacement string (1-8 chars)
END TYPE
DIM SHARED wpEntRepl(0 TO 10) AS wikiEntityReplace
DIM SHARED wpEntReplCnt: wpEntReplCnt = -1 'wpEntRepl index counter (pre-increment, hence
'you don't need "wpEntReplCnt - 1" when used in loops, just do "0 TO wpEntReplCnt"
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&apos;": wpEntRepl(wpEntReplCnt).repl = "'" 'apostrophe
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&#91;": wpEntRepl(wpEntReplCnt).repl = "[" 'square bracket (open)
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&#93;": wpEntRepl(wpEntReplCnt).repl = "]" 'square bracket (close)
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&#123;": wpEntRepl(wpEntReplCnt).repl = "{" 'curly bracket (open)
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&#125;": wpEntRepl(wpEntReplCnt).repl = "}" 'curly bracket (close)
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&pi;": wpEntRepl(wpEntReplCnt).repl = CHR$(227) 'pi
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&theta;": wpEntRepl(wpEntReplCnt).repl = CHR$(233) 'theta
wpEntReplCnt = wpEntReplCnt + 1: wpEntRepl(wpEntReplCnt).enti = "&nbsp;": wpEntRepl(wpEntReplCnt).repl = CHR$(255) 'non-breaking space

'UTF-8 replacements
TYPE wikiUtf8Replace
    utf8 AS STRING * 4 '= MKI$(reversed hex 2-byte UTF-8 sequence) or MKL$(reversed hex 3/4-byte UTF-8 sequence)
    repl AS STRING * 8 '= replacement string (1-8 chars)
END TYPE
DIM SHARED wpUtfRepl(0 TO 65) AS wikiUtf8Replace
DIM SHARED wpUtfReplCnt: wpUtfReplCnt = -1 'wpUtfRepl index counter (pre-increment, hence
'you don't need "wpUtfReplCnt - 1" when used in loops, just do "0 TO wpUtfReplCnt"
'Note: All UTF-8 values must be reversed in MKI$/MKL$, as it flips them to little endian.
'      In the wiki text they are noted in big endian, hence we need to pre-flip them.
'2-byte sequences
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HB6C2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(20) 'pilcrow
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA7C2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(21) 'section
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA9C2): wpUtfRepl(wpUtfReplCnt).repl = "(c)" 'copyright
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA9C3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(130) 'accent (‚)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA2C3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(131) 'accent (ƒ)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA0C3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(133) 'accent (…)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA5C3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(134) 'accent (†)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA7C3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(135) 'accent (‡)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HAAC3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(136) 'accent (ˆ)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HABC3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(137) 'accent (‰)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA8C3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(138) 'accent (Š)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HAFC3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(139) 'accent (‹)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HAEC3): wpUtfRepl(wpUtfReplCnt).repl = CHR$(140) 'accent (Œ)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA2C2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(155) 'cents
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HBDC2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(171) 'fraction («)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HBCC2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(172) 'fraction (¬)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&H80CF): wpUtfRepl(wpUtfReplCnt).repl = CHR$(227) 'pi
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKI$(&HA0C2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(255) 'non-breaking space
'3-byte sequences
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA680E2): wpUtfRepl(wpUtfReplCnt).repl = "..." 'ellipsis (hori.)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HAE8BE2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(240) 'ellipsis (vert.) (ð)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8C94E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(218) 'single line draw (top/left corner)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9094E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(191) 'single line draw (top/right corner)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9494E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(192) 'single line draw (bottom/left corner)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9894E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(217) 'single line draw (bottom/right corner)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8094E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(196) 'single line draw (horizontal line)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8294E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(179) 'single line draw (vertical line)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HB494E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(193) 'single line draw (hori. line + up connection)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HAC94E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(194) 'single line draw (hori. line + down connection)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA494E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(180) 'single line draw (vert. line + left connection)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9C94E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(195) 'single line draw (vert. line + right connection)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBC94E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(197) 'single line draw (hori./vert. line cross)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HB296E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(30) 'triangle (up)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBC96E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(31) 'triangle (down)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8497E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(17) 'triangle (left)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBA96E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(16) 'triangle (right)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9186E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(24) 'arrow (up)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9386E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(25) 'arrow (down)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9586E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(18) 'arrow (up down)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA886E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(23) 'arrow (up down with base)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9086E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(27) 'arrow (left)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9286E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(26) 'arrow (right)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9486E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(29) 'arrow (left right)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBA98E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(1) 'face (drawing)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBB98E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(2) 'face (solid)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA599E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(3) 'card suit (heart)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA699E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(4) 'card suit (diamond)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA399E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(5) 'card suit (club)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA099E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(6) 'card suit (spade)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8299E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(11) 'gender sign (male)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8099E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(12) 'gender sign (female)
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HAA99E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(13) 'eighth note
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HAB99E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(14) 'beamed eighth notes
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HA280E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(7) 'bullet
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9897E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(8) 'inverse bullet
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8B97E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(9) 'circle
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9997E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(10) 'inverse circle
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H8896E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(219) 'full block
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBC98E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(15) 'sun with rays
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HBC80E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(19) 'double exclamation
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&HAC96E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(22) 'rectangle
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H9F88E2): wpUtfRepl(wpUtfReplCnt).repl = CHR$(28) 'right angle
'4-byte sequences
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H80989FF0): wpUtfRepl(wpUtfReplCnt).repl = ":)" 'smily
wpUtfReplCnt = wpUtfReplCnt + 1: wpUtfRepl(wpUtfReplCnt).utf8 = MKL$(&H88989FF0): wpUtfRepl(wpUtfReplCnt).repl = ";)" 'wink

