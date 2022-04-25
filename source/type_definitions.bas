$If TYPE = UNDEFINED Then
    Type usedVarList
        As Long id, linenumber, includeLevel, includedLine, scope, localIndex
        As Long arrayElementSize
        As _Byte used, watch, isarray, displayFormat 'displayFormat: 0=DEC;1=HEX;2=BIN;3=OCT
        As String name, cname, varType, includedFile, subfunc
        As String watchRange, indexes, elements, elementTypes 'for Arrays and UDTs
        As String elementOffset, storage
    End Type

    Type HashListItem
        Flags As Long
        Reference As Long
        NextItem As Long
        PrevItem As Long
        LastItem As Long 'note: this value is only valid on the first item in the list
        'note: name is stored in a seperate array of strings
    End Type

    Type Label_Type
        State As _Unsigned _Byte '0=label referenced, 1=label created
        cn As String * 256
        Scope As Long
        Data_Offset As _Integer64 'offset within data
        Data_Referenced As _Unsigned _Byte 'set to 1 if data is referenced (data_offset will be used to create the data offset variable)
        Error_Line As Long 'the line number to reference on errors
        Scope_Restriction As Long 'cannot exist inside this scope (post checked)
        SourceLineNumber As Long
    End Type

    Type idstruct

        n As String * 256 'name
        cn As String * 256 'case sensitive version of n

        arraytype As Long 'similar to t
        arrayelements As Integer
        staticarray As Integer 'set for arrays declared in the main module with static elements

        mayhave As String * 8 'mayhave and musthave are exclusive of each other
        musthave As String * 8
        t As Long 'type

        tsize As Long


        subfunc As Integer 'if function=1, sub=2 (max 100 arguments)
        Dependency As Integer
        internal_subfunc As Integer

        callname As String * 256
        ccall As Integer
        overloaded As _Byte
        args As Integer
        minargs As Integer
        arg As String * 400 'similar to t
        argsize As String * 400 'similar to tsize (used for fixed length strings)
        specialformat As String * 256
        secondargmustbe As String * 256
        secondargcantbe As String * 256
        ret As Long 'the value it returns if it is a function (again like t)

        insubfunc As String * 256
        insubfuncn As Long

        share As Integer
        nele As String * 100
        nelereq As String * 100
        linkid As Long
        linkarg As Integer
        staticscope As Integer
        'For variables which are arguments passed to a sub/function
        sfid As Long 'id number of variable's parent sub/function
        sfarg As Integer 'argument/parameter # within call (1=first)

        hr_syntax As String
    End Type

    Type GL_idstruct
        cn As String * 64 'case sensitive version of n
        subfunc As Integer 'if function=1, sub=2
        callname As String * 64
        args As Integer
        arg As String * 80 'similar to t
        ret As Long 'the value it returns if it is a function (again like t)
    End Type
    Type Help_Back_Type
        sx As Long
        sy As Long
        cx As Long
        cy As Long
    End Type
    Dim Shared id As idstruct
    Type position
        x As Integer
        y As Integer
        caption As String
    End Type
    Type vWatchPanelType
        As Integer x, y, w, h, firstVisible, hPos, vBarThumb, hBarThumb
        As Integer draggingVBar, draggingHBar, mX, mY
        As Long contentWidth, tempIndex
        As _Byte draggingPanel, resizingPanel, closingPanel, clicked
    End Type
    Type ui
        As Integer x, y, w, h
        As String caption
    End Type
    Type varDlgList
        As Long index, bgColorFlag, colorFlag, colorFlag2, indicator, indicator2
        As _Byte selected
        As String varType
    End Type
    '--------------------------------------------------------------------------------
    Type idedbptype
        x As Long
        y As Long
        w As Long
        h As Long
        nam As Long
    End Type
    '--------------------------------------------------------------------------------
    Type idedbotype
        par As idedbptype
        x As Long
        y As Long
        w As Long
        h As Long
        typ As Long
        nam As Long
        txt As Long
        dft As Long
        cx As Long
        cy As Long
        foc As Long
        sel As Long 'selected item no.
        selY As Long
        stx As Long 'selected item in string form
        issel As _Byte 'selection indicator (for text boxes only)
        sx1 As Long 'selection start (for text boxes only)
        v1 As Long
        num As Long
    End Type
    '--------------------------------------------------------------------------------
    Type IdeBmkType
        y As Long 'the vertical line
        x As Long 'the horizontal position to move cursor to
        reserved As Long
        reserved2 As Long
    End Type
    Type QuickNavType
        As Long idesx, idesy, idecx, idecy
    End Type




$End If
$Let TYPE = TRUE
