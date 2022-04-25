'$INCLUDE:'.\type_definitions.bas'

$If WIKI_GLOBAL = UNDEFINED Then

    Dim Shared Help_sx, Help_sy, Help_cx, Help_cy
    Dim Shared Help_Select, Help_cx1, Help_cy1, Help_SelX1, Help_SelX2, Help_SelY1, Help_SelY2
    Dim Shared Help_MSelect
    Help_sx = 1: Help_sy = 1: Help_cx = 1: Help_cy = 1
    Dim Shared Help_wx1, Help_wy1, Help_wx2, Help_wy2 'defines the text section of the help window on-screen
    Dim Shared Help_ww, Help_wh 'width & height of text region
    Dim Shared help_h, help_w 'width & height
    Dim Shared Help_Txt$ '[chr][col][link-byte1][link-byte2]
    Dim Shared Help_Txt_Len
    Dim Shared Help_Line$ 'index of first txt element of a line
    Dim Shared Help_Link$ 'the link info [sep][type:]...[sep]
    Dim Shared Help_Link_Sep$: Help_Link_Sep$ = Chr$(13)
    Dim Shared Help_LinkN
    Dim Shared Help_NewLineIndent
    Dim Shared Help_Underline
    'Link Types:
    ' PAGE:wikipagename
    Dim Shared Help_Pos, Help_Wrap_Pos
    Dim Shared Help_BG_Col
    Dim Shared Help_Col_Normal: Help_Col_Normal = 7
    Dim Shared Help_Col_Link: Help_Col_Link = 9
    Dim Shared Help_Col_Bold: Help_Col_Bold = 15
    Dim Shared Help_Col_Italic: Help_Col_Italic = 15
    Dim Shared Help_Col_Section: Help_Col_Section = 8
    Dim Shared Help_Bold, Help_Italic
    Dim Shared Help_LockWrap
    ReDim Shared Help_LineLen(1)
    ReDim Shared Back$(1)
    ReDim Shared Back_Name$(1)
    ReDim Shared Help_Back(1) As Help_Back_Type
    Back$(1) = "QB64 Help Menu"
    Back_Name$(1) = "Help"
    Help_Back(1).sx = 1: Help_Back(1).sy = 1: Help_Back(1).cx = 1: Help_Back(1).cy = 1
    Dim Shared Help_Back_Pos
    Help_Back_Pos = 1
    Dim Shared Help_Search_Time As Double
    Dim Shared Help_Search_Str As String
    Dim Shared Help_PageLoaded As String
    Dim Shared Help_Recaching, Help_IgnoreCache
$End If
$Let WIKI_GLOBAL = TRUE
