' This file holds the logic for the $EMBED code generation, and also the embeds.txt file that tracks the contents of embedded.cpp

' Writes embedded.cpp: one C array per $EMBED file the program actually reads back
' with _EMBEDDED$(), plus the func__embedded() dispatcher over them.
'
' Does nothing when embeds.txt says the file on disk already contains exactly
' what we'd be creating.
SUB WriteEmbeddedCpp
    DIM eflPacked AS _BYTE, eflHash AS STRING
    DIM eflSize AS _INTEGER64, eflTime AS _INTEGER64

    eflUB = UBOUND(embedFileList$, 2)
    IF EmbedManMatches%(eflUB) THEN
        'Nothing embedded.cpp was generated from has changed, so it is left exactly
        'as it is
        '
        'We have to check if any of them were packed to know whether ZLIB needs
        'to be linked.
        FOR i = 0 TO EmbedManCount - 1
            IF EmbedManPacked(i) THEN SetDependency DEPENDENCY_ZLIB
            SetDependency DEPENDENCY_EMBED
        NEXT i
    ELSE
        eflFF = FREEFILE
        OPEN "O", #eflFF, tmpdir$ + "embedded.cpp"
        'write required header stuff
        PRINT #eflFF, "#include <stdint.h>"
        PRINT #eflFF, "#include <string.h>"
        PRINT #eflFF, "#include "; AddQuotes$("../c/common.h")
        PRINT #eflFF, "#include "; AddQuotes$("../c/libqb/include/compression.h")
        PRINT #eflFF, ""
        CLOSE #eflFF
        'append files converted into arrays
        '> embed only those $EMBED files, which are referenced by at least
        '  one _EMBEDDED$() call to avoid unnecessary bloat
        '> adjust dependency settings according to the process
        '> record each file as it was read, for the manifest
        EmbedManCount = 0
        FOR i = 0 TO eflUB
            IF embedFileList$(eflFile, i) <> "" AND embedFileList$(eflUsed, i) = "yes" THEN
                eflPacked = ConvertFileToCArray%(embedFileList$(eflFile, i), embedFileList$(eflHand, i), eflSize, eflHash)
                IF eflPacked THEN SetDependency DEPENDENCY_ZLIB
                SetDependency DEPENDENCY_EMBED
                eflTime = FileModTime&&(embedFileList$(eflFile, i))
                EmbedManAdd embedFileList$(eflHand, i), embedFileList$(eflFile, i), eflSize, eflTime, eflHash, eflPacked
            END IF
        NEXT i
        '-----
        eflFF = FREEFILE
        OPEN "A", #eflFF, tmpdir$ + "embedded.cpp"
        'append the internal retrieval function for _EMBEDDED$()
        PRINT #eflFF, "qbs *func__embedded(qbs *handle)"
        PRINT #eflFF, "{"
        FOR i = 0 TO eflUB
            IF embedFileList$(eflFile, i) <> "" AND embedFileList$(eflUsed, i) = "yes" THEN
                PRINT #eflFF, "    if (qbs_equal(handle, qbs_new_txt("; AddQuotes$(embedFileList$(eflHand, i)); "))) {return GetArrayData_"; embedFileList$(eflHand, i); "();}"
            END IF
        NEXT i
        PRINT #eflFF, "    return qbs_new_txt("; MKI$(&H2222); ");"
        PRINT #eflFF, "}"
        PRINT #eflFF, ""
        CLOSE #eflFF
        'The manifest goes out after the file it describes, so that a QB64-PE killed
        'part way through this leaves one that predates embedded.cpp and the next
        'build regenerates. The manifest may cause extra work, never skipped work.
        EmbedManCppLen = FileLength&&(tmpdir$ + "embedded.cpp")
        EmbedManSave
    END IF
END SUB


' This function is related to $EMBED, it converts the given file into
' an C-Array incl. read back function and append it to the C/C++ source
' file "internal\temp\embedded.cpp".
'
' Inputs:  sourcefile spec, unique handle (case sensitive)
' Outputs: fileSize&& and fileHash$ describe the file as it was read, for the
'          manifest that lets the next build skip all of this. They cost
'          nothing here because the whole file is in hand already.
' Return: 0 = normal embed, 1 = packed embed (DEPENDENCY_ZLIB required)
FUNCTION ConvertFileToCArray% (file$, handle$, fileSize&&, fileHash$)
    '--- read file contents ---
    sff% = FREEFILE
    filedata$ = _READFILE$(file$)
    fileSize&& = LEN(filedata$)
    fileHash$ = _MD5$(filedata$)
    '--- try to compress ---
    compdata$ = _DEFLATE$(filedata$)
    IF LEN(compdata$) < (LEN(filedata$) * 0.8) THEN
        tmpfile$ = tmpdir$ + "embed.bin"
        _WRITEFILE tmpfile$, compdata$
        packed% = 1
        OPEN "B", #sff%, tmpfile$
    ELSE
        packed% = 0
        OPEN "B", #sff%, file$
    END IF
    '--- init variables ---
    fl& = LOF(sff%)
    cntL& = INT(fl& / 32)
    cntV& = INT(cntL& / 8180)
    cntB& = (fl& - (cntL& * 32))
    '--- create C-Array ---
    dff% = FREEFILE
    OPEN "A", #dff%, tmpdir$ + "embedded.cpp"
    '--- process LONGs ---
    tmpI$ = SPACE$(32)
    FOR vc& = 0 TO cntV&
        IF vc& = cntV& THEN numL& = (cntL& MOD 8180): ELSE numL& = 8180
        PRINT #dff%, "static const uint32_t "; handle$; "L"; _TOSTR$(vc&); "[] = {"
        PRINT #dff%, "    "; _TOSTR$(numL& * 8); ","
        FOR z& = 1 TO numL&
            GET #sff%, , tmpI$: offI% = 1
            tmpO$ = "    " + STRING$(88, ","): offO% = 5
            DO
                tmpL& = CVL(MID$(tmpI$, offI%, 4)): offI% = offI% + 4
                MID$(tmpO$, offO%, 10) = "0x" + RIGHT$("00000000" + HEX$(tmpL&), 8)
                offO% = offO% + 11
            LOOP UNTIL offO% > 92
            IF z& < numL& THEN PRINT #dff%, tmpO$: ELSE PRINT #dff%, LEFT$(tmpO$, 91)
        NEXT z&
        PRINT #dff%, "};"
        PRINT #dff%, ""
    NEXT vc&
    '--- process remaining BYTEs ---
    IF cntB& > 0 THEN
        PRINT #dff%, "static const uint8_t "; handle$; "B[] = {"
        PRINT #dff%, "    "; _TOSTR$(cntB&); ","
        PRINT #dff%, "    ";
        FOR x% = 1 TO cntB&
            GET #sff%, , tmpB%%
            PRINT #dff%, "0x" + RIGHT$("00" + HEX$(tmpB%%), 2);
            IF x% <> 16 THEN
                IF x% <> cntB& THEN PRINT #dff%, ",";
            ELSE
                IF x% <> cntB& THEN
                    PRINT #dff%, ","
                    PRINT #dff%, "    ";
                END IF
            END IF
        NEXT x%
        PRINT #dff%, ""
        PRINT #dff%, "};"
        PRINT #dff%, ""
    END IF
    '--- make a read function ---
    PRINT #dff%, "qbs *GetArrayData_"; handle$; "(void)"
    PRINT #dff%, "{"
    PRINT #dff%, "    qbs  *data = qbs_new("; _TOSTR$(fl&); ", 1);"
    PRINT #dff%, "    char *buff = (char*) data -> chr;"
    PRINT #dff%, ""
    FOR vc& = 0 TO cntV&
        PRINT #dff%, "    memcpy(buff, &"; handle$; "L"; _TOSTR$(vc&); "[1], "; handle$; "L"; _TOSTR$(vc&); "[0] << 2);"
        IF vc& < cntV& OR cntB& > 0 THEN
            PRINT #dff%, "    buff += ("; handle$; "L"; _TOSTR$(vc&); "[0] << 2);"
        END IF
    NEXT vc&
    IF cntB& > 0 THEN
        PRINT #dff%, "    memcpy(buff, &"; handle$; "B[1], "; handle$; "B[0]);"
    END IF
    PRINT #dff%, ""
    IF packed% THEN
        PRINT #dff%, "    return func__inflate(data, "; _TOSTR$(LEN(filedata$)); ", 1);"
    ELSE
        PRINT #dff%, "    return data;"
    END IF
    PRINT #dff%, "}"
    PRINT #dff%, ""
    '--- ending ---
    CLOSE #dff%
    CLOSE #sff%
    ConvertFileToCArray% = packed%
END FUNCTION


' The manifest file of this QB64-PE instance, or "" while there is no temp
' folder to put it in (in which case there is no manifest and the embed pass
' always runs).
FUNCTION EmbedManSpec$
    IF LEN(tmpdir$) THEN EmbedManSpec$ = tmpdir$ + "embeds.txt"
END FUNCTION


' Tells whether embedded.cpp is still exactly what this program's $EMBED
' files would produce.
'
' All of these have to hold: the manifest parses, the initial tag matches.
' the ordered list of used (handle, file) pairs is the one it records, every
' one of those files is unchanged, and embedded.cpp is still on disk at the
' recorded length.
'
' The list has to match in order because both the array definitions and the
' dispatcher arms are emitted in embedFileList$ order. "Used" means referenced
' by at least one _EMBEDDED$() call, which is not known until the program has
' been parsed, so a file that stopped being referenced counts as a change here
' just as a new one does.
FUNCTION EmbedManMatches% (listUB)
    DIM i&, n&, spec$, mt AS _INTEGER64, refresh%

    IF NOT EmbedManLoad% THEN EXIT FUNCTION

    n& = 0
    FOR i& = 0 TO listUB
        IF embedFileList$(eflFile, i&) <> "" AND embedFileList$(eflUsed, i&) = "yes" THEN
            IF n& >= EmbedManCount THEN EXIT FUNCTION
            IF EmbedManHand$(n&) <> embedFileList$(eflHand, i&) THEN EXIT FUNCTION
            IF EmbedManPath$(n&) <> embedFileList$(eflFile, i&) THEN EXIT FUNCTION
            n& = n& + 1
        END IF
    NEXT i&
    IF n& <> EmbedManCount THEN EXIT FUNCTION

    FOR i& = 0 TO EmbedManCount - 1
        spec$ = EmbedManPath$(i&)

        IF NOT _FILEEXISTS(spec$) THEN EXIT FUNCTION
        IF FileLength&&(spec$) <> EmbedManSize(i&) THEN EXIT FUNCTION

        mt = FileModTime&&(spec$)
        ' Matching size and matching stamp indicates the file is unchanged.
        IF mt = 0 _ORELSE mt <> EmbedManTime(i&) THEN

            ' In the event the mtime doesn't match (or is not present), we do
            ' hash comparison to see if the contents are still the same.
            IF _MD5$(_READFILE$(spec$)) <> EmbedManHash$(i&) THEN EXIT FUNCTION
            IF mt <> 0 THEN EmbedManTime(i&) = mt: refresh% = -1
        END IF
    NEXT i&

    IF NOT _FILEEXISTS(tmpdir$ + "embedded.cpp") THEN EXIT FUNCTION
    IF FileLength&&(tmpdir$ + "embedded.cpp") <> EmbedManCppLen THEN EXIT FUNCTION

    IF refresh% THEN EmbedManSave

    EmbedManMatches% = -1
END FUNCTION


SUB EmbedManAdd (handle$, fileSpec$, fileSize AS _INTEGER64, fileTime AS _INTEGER64, fileHash$, packed AS _BYTE)
    DIM idx&, newUb&

    idx& = EmbedManCount
    IF idx& > UBOUND(EmbedManHand$) THEN
        newUb& = idx& + 15
        REDIM _PRESERVE EmbedManHand$(newUb&)
        REDIM _PRESERVE EmbedManPath$(newUb&)
        REDIM _PRESERVE EmbedManHash$(newUb&)
        REDIM _PRESERVE EmbedManSize(newUb&) AS _INTEGER64
        REDIM _PRESERVE EmbedManTime(newUb&) AS _INTEGER64
        REDIM _PRESERVE EmbedManPacked(newUb&) AS _BYTE
    END IF
    EmbedManHand$(idx&) = handle$
    EmbedManPath$(idx&) = fileSpec$
    EmbedManHash$(idx&) = fileHash$
    EmbedManSize(idx&) = fileSize
    EmbedManTime(idx&) = fileTime
    EmbedManPacked(idx&) = packed
    EmbedManCount = idx& + 1
END SUB


' Loads the manifest left by a previous build.
'
' To get loaded, the QB64-PE version the manifest was created from has to
' match, and then every line has to be validly formatted.
FUNCTION EmbedManLoad%
    DIM ff%, spec$, ln$, fp&, f1$, f2$, f3$, f4$, f5$, rest$
    DIM sz AS _INTEGER64, mt AS _INTEGER64, pk AS _BYTE

    EmbedManCount = 0
    EmbedManCppLen = -1
    spec$ = EmbedManSpec$

    IF LEN(spec$) = 0 THEN EXIT FUNCTION
    IF NOT _FILEEXISTS(spec$) THEN EXIT FUNCTION

    ff% = FREEFILE
    OPEN spec$ FOR INPUT AS ff%
    IF NOT EOF(ff%) THEN
        LINE INPUT #ff%, ln$
        ln$ = EmbedManTrimEol$(ln$)

        IF LEFT$(ln$, LEN(EmbedManTag) + 1) = EmbedManTag + " " THEN
            fp& = LEN(EmbedManTag) + 2
            f1$ = EmbedManField$(ln$, fp&) 'length of embedded.cpp
            rest$ = MID$(ln$, fp&) '        version, which may contain anything

            IF EmbedManIsNumber%(f1$) _ANDALSO rest$ = Version$ THEN
                EmbedManCppLen = EmbedManNumber&&(f1$)
                DO UNTIL EOF(ff%)
                    LINE INPUT #ff%, ln$
                    ln$ = EmbedManTrimEol$(ln$)
                    fp& = 1
                    f1$ = EmbedManField$(ln$, fp&) 'size
                    f2$ = EmbedManField$(ln$, fp&) 'mtime
                    f3$ = EmbedManField$(ln$, fp&) 'hash
                    f4$ = EmbedManField$(ln$, fp&) 'packed
                    f5$ = EmbedManField$(ln$, fp&) 'handle
                    rest$ = MID$(ln$, fp&) '        file, which may contain spaces

                    IF EmbedManIsNumber%(f1$) = 0 _ORELSE EmbedManIsNumber%(f2$) = 0 _ORELSE LEN(f3$) = 0 _ORELSE LEN(f5$) = 0 _ORELSE LEN(rest$) = 0 _ORELSE (f4$ <> "0" _ANDALSO f4$ <> "1") THEN
                        EmbedManCount = 0
                        EmbedManCppLen = -1
                        EXIT DO
                    END IF

                    sz = EmbedManNumber&&(f1$)
                    mt = EmbedManNumber&&(f2$)
                    pk = 0: IF f4$ = "1" THEN pk = 1
                    EmbedManAdd f5$, rest$, sz, mt, f3$, pk
                LOOP
            END IF
        END IF
    END IF
    CLOSE ff%
    EmbedManLoad% = (EmbedManCppLen >= 0)
END FUNCTION


' Regenerate the manifest file
SUB EmbedManSave
    DIM ff%, spec$, i&

    spec$ = EmbedManSpec$
    IF LEN(spec$) = 0 THEN EXIT SUB
    ff% = FREEFILE
    OPEN spec$ FOR OUTPUT AS ff%
    PRINT #ff%, EmbedManTag + " " + _TOSTR$(EmbedManCppLen) + " " + Version$
    FOR i& = 0 TO EmbedManCount - 1
        PRINT #ff%, _TOSTR$(EmbedManSize(i&)) + " " + _TOSTR$(EmbedManTime(i&)) + " " + EmbedManHash$(i&) + " " + _TOSTR$(EmbedManPacked(i&)) + " " + EmbedManHand$(i&) + " " + EmbedManPath$(i&)
    NEXT i&
    CLOSE ff%
END SUB


' PRINT # writes a CRLF on every platform, this strips the CR if it's still
' present in the string.
FUNCTION EmbedManTrimEol$ (ln$)
    IF RIGHT$(ln$, 1) = CHR$(13) THEN
        EmbedManTrimEol$ = LEFT$(ln$, LEN(ln$) - 1)
    ELSE
        EmbedManTrimEol$ = ln$
    END IF
END FUNCTION

' Pulls the next space delimited field out of a manifest line, moving
' fldPos& past it. Returns "" once the line has run out.
FUNCTION EmbedManField$ (ln$, fldPos&)
    DIM sp&

    IF fldPos& > LEN(ln$) THEN EXIT FUNCTION
    sp& = INSTR(fldPos&, ln$, " ")
    IF sp& = 0 THEN
        EmbedManField$ = MID$(ln$, fldPos&)
        fldPos& = LEN(ln$) + 1
    ELSE
        EmbedManField$ = MID$(ln$, fldPos&, sp& - fldPos&)
        fldPos& = sp& + 1
    END IF
END FUNCTION


FUNCTION EmbedManIsNumber% (fld$)
    DIM i&

    IF LEN(fld$) = 0 _ORELSE LEN(fld$) > 19 THEN EXIT FUNCTION
    FOR i& = 1 TO LEN(fld$)
        IF ASC(fld$, i&) < 48 _ORELSE ASC(fld$, i&) > 57 THEN EXIT FUNCTION
    NEXT i&
    'equal-length strings of digits compare as the numbers do
    IF LEN(fld$) = 19 _ANDALSO fld$ > "9223372036854775807" THEN EXIT FUNCTION
    EmbedManIsNumber% = -1
END FUNCTION


' Reads a field EmbedManIsNumber%() has already accepted.
FUNCTION EmbedManNumber&& (fld$)
    DIM i&, v AS _INTEGER64

    FOR i& = 1 TO LEN(fld$)
        v = v * 10 + (ASC(fld$, i&) - 48)
    NEXT i&
    EmbedManNumber&& = v
END FUNCTION
