' This is the logic for splitting the generated C++ into separate translation
' units, enabling incremental rebuild where we only recompile the routines that
' changed. The files this writes - main_split.txt, qbx_gen.h, split.mk,
' objs.rsp and one tu_*.cpp per unit - are what the Makefile builds the program
' from when SplitObjectFiles is on.

' Record that the SUB/FUNCTION being compiled calls a DECLARE LIBRARY routine.
' This indicates it may require a C header and have to be pulled into the main
' compilation unit.
SUB NoteCcallFromSubFunc (libid AS LONG)
    IF subfuncn = 0 THEN EXIT SUB
    k$ = MKL$(libid)
    'Fixed-width records, so scan in steps of 4: INSTR would also match across a
    'record boundary.
    FOR i = 1 TO LEN(SubFuncCcallIds(subfuncn)) STEP 4
        IF MID$(SubFuncCcallIds(subfuncn), i, 4) = k$ THEN EXIT SUB
    NEXT
    SubFuncCcallIds(subfuncn) = SubFuncCcallIds(subfuncn) + k$
END SUB

' Write out a SUB/FUNCTION's generated C++ to disk and release the memory it's using.
'
' This should only be performed once the given SUB/FUNCTION is done being generated,
' as after this point it can no longer be modified. For larger programs doing
' this is important however because otherwise we have to hold the entire
' generated C++ in memory, that can be significant for larger programs.
'
' The buffers are "retired" rather than cleared, this means SBufN still names
' their files for PruneStaleGeneratedFiles and GetBufLen&() still reports their
' size for WriteSplitTranslationUnits.
SUB RetireSubFuncBuffers (n AS LONG)
    ' A command-line build only does one compilation so it has nothing to gain
    ' by keeping all the generated C++ in memory. Interactive use during the
    ' IDE will do lots of compiations in series during editing though, so we
    ' make it optional in that situation.
    IF idemode _ANDALSO NOT WriteFilesWhileTranslating THEN EXIT SUB

    ' Format mode doesn't actually write to disk, so throw the contents away
    ' rather than filling the temp folder with them.
    IF FormatMode THEN discard% = -1

    s$ = _TOSTR$(n)
    RetireBufferNamed tmpdir$ + "main" + s$ + ".txt", discard%
    RetireBufferNamed tmpdir$ + "data" + s$ + ".txt", discard%
    RetireBufferNamed tmpdir$ + "free" + s$ + ".txt", discard%
    RetireBufferNamed tmpdir$ + "ret" + s$ + ".txt", discard%
END SUB

' This groups the program's SUB/FUNCTIONs into translation units and writes the
' files the Makefile needs to build them. The general concept is that we want to
' achieve a few things:
'
' 1. Reasonable size of translation units. Compilation will be slower if there's
'    lots of very small files to compile.
'
' 2. Avoiding creating large translation units, as they then have to get
'    recompiled every time something in them changes.
'
' 3. Retain the QB64-PE source locality of the generated C++. Editing a couple
'    SUB/FUNCTIONs in one QB64-PE source file should not trigger several different
'    translation units to get recompiled.
'
' To achieve that we group the generated C++ via their original QB64-PE
' source files, and then also split that source if it gets larger than
' SplitChunkBytes.
'
' Routines that call a DECLARE LIBRARY routine we emitted no declaration for
' stay with the main module since that's the only unit that includes the user's
' C headers.
SUB WriteSplitTranslationUnits
    CONST BinHashMask = 1023

    ' Each SUB/FUNCTION is assigned to a 'bin', each bin is a group of SUB/FUNCTIONs
    ' that will be compiled together.
    '
    ' Routines go into the same bin if the originate from the same source file.
    DIM binOf(subfuncnlast) AS LONG, pinned(subfuncnlast) AS _BYTE
    DIM binName$(subfuncnlast), binBytes(subfuncnlast) AS LONG
    DIM sizes(subfuncnlast) AS LONG
    DIM binRoutines(subfuncnlast) AS LONG, binStart(subfuncnlast + 1) AS LONG
    DIM binFill(subfuncnlast) AS LONG, byBin(subfuncnlast) AS LONG

    ' Source file name -> bin, as a chained hash table.
    DIM binHash(BinHashMask) AS LONG, binHashNext(subfuncnlast) AS LONG

    ' Determine which routines need to be pinned into the main module code.
    pinnedCount = 0
    FOR i = 1 TO subfuncnlast
        ids$ = SubFuncCcallIds(i)
        FOR k = 1 TO LEN(ids$) STEP 4
            IF ids(CVL(MID$(ids$, k, 4))).libnodecl THEN
                pinned(i) = 1
                pinnedCount = pinnedCount + 1
                EXIT FOR
            END IF
        NEXT
    NEXT

    ' Group by declaring file, preserving first-seen order so that unit
    ' numbering is stable as long as the set of files is
    binCount = 0
    FOR i = 1 TO subfuncnlast
        IF pinned(i) = 0 THEN
            sizes(i) = GetBufLen&(SBufNFind&(tmpdir$ + "main" + _TOSTR$(i) + ".txt"))

            ' Find the hash entry for this source file, creating it if it doesn't exist
            f$ = SubFuncSrcFile(i)
            h = SBufNHash&(f$) AND BinHashMask
            b = 0
            j = binHash(h)
            DO WHILE j
                IF binName$(j) = f$ THEN b = j: EXIT DO
                j = binHashNext(j)
            LOOP
            IF b = 0 THEN
                binCount = binCount + 1
                b = binCount
                binName$(b) = f$
                binBytes(b) = 0
                binRoutines(b) = 0
                binHashNext(b) = binHash(h)
                binHash(h) = b
            END IF
            binOf(i) = b
            binBytes(b) = binBytes(b) + sizes(i)
            binRoutines(b) = binRoutines(b) + 1
        END IF
    NEXT

    ' We now each SUB/FUNCTION assigned to a bin but we can't easily loop over
    ' each bin's SUB/FUNCTIONs to produce the compilation file.
    '
    ' To fix that we sort them into a new 'byBin' array - each entry holds the index
    ' to one SUB/FUNCTION, but it's sorted first by bin, so indexes 1-A
    ' contains the first bin's SUB/FUNCTIONs, A-B contains the second bin's
    ' SUB/FUNCTIONs, B-C contains the third bin, etc.
    '
    ' 'binStart()' is indexed by bin and indicates the first index in 'byBin'
    ' for that particular bin. The last index for that bin is the one before
    ' 'binStart()' for the next bin.
    slot = 1
    FOR b = 1 TO binCount
        binStart(b) = slot
        binFill(b) = slot
        slot = slot + binRoutines(b)
    NEXT
    binStart(binCount + 1) = slot
    FOR i = 1 TO subfuncnlast
        b = binOf(i)
        IF b THEN
            byBin(binFill(b)) = i
            binFill(b) = binFill(b) + 1
        END IF
    NEXT

    ' main_split.txt is the main module's unit, it includes the module-level code and any 
    ' routines pinned to it
    mainsplitbuf = OpenBuffer&("O", tmpdir$ + "main_split.txt")
    WriteBufLine mainsplitbuf, "#include " + CHR$(34) + "main0.txt" + CHR$(34)
    FOR i = 1 TO subfuncnlast
        IF pinned(i) THEN WriteBufLine mainsplitbuf, "#include " + CHR$(34) + "main" + _TOSTR$(i) + ".txt" + CHR$(34)
    NEXT

    ' This header includes all the headers needed by every split compilation file.
    genbuf = OpenBuffer&("O", tmpdir$ + "qbx_gen.h")
    WriteBufLine genbuf, "#pragma once"
    WriteBufLine genbuf, "#include " + CHR$(34) + "../c/qbx.h" + CHR$(34)
    WriteBufLine genbuf, "#include " + CHR$(34) + "globalx.txt" + CHR$(34)
    WriteBufLine genbuf, "#include " + CHR$(34) + "regsf.txt" + CHR$(34)

    ' Open the file that will hold all the Makefile logic for compiling the
    ' separate files. We add to this as we go.
    mkbuf = OpenBuffer&("O", tmpdir$ + "split.mk")
    WriteBufLine mkbuf, "# Generated by QB64-PE. Translation units of the compiled program."
    WriteBufLine mkbuf, "QB_SPLIT_SRCS :="

    ' Backslashes would need to be escaped, but it's fine to use forward
    ' slashes on every platform since this file is just read by gcc which
    ' accepts them.
    rspdir$ = StrReplace$(tmpdir$, "\", "/")
    rspbuf = OpenBuffer&("O", tmpdir$ + "objs.rsp")

    ' The compilation-info functions get a unit of their own because they get
    ' recompiled on every rebuild.
    IF UsesCompileDate OR UsesCompileTime OR UsesCompilerVersion THEN
        compinfobuf = OpenBuffer&("O", tmpdir$ + "compinfo.cpp")
        WriteBufLine compinfobuf, "#include " + CHR$(34) + "qbx_gen.h" + CHR$(34)
        WriteCompilationInfoFuncs compinfobuf
        WriteBufLine mkbuf, "QB_SPLIT_SRCS += $(PATH_INTERNAL_TEMP)/compinfo.cpp"
        WriteBufLine rspbuf, rspdir$ + "compinfo.o"

        ' If the user wants Compilation date or time then we use FORCE to tell
        ' make it should always get rebuilt, even if nothing changes, since the
        ' date/time might be different.
        IF UsesCompileDate OR UsesCompileTime THEN
            WriteBufLine mkbuf, "$(PATH_INTERNAL_TEMP)/compinfo.o: FORCE"
        END IF
    END IF

    unitCount = 0
    FOR b = 1 TO binCount
        ' SplitChunkBytes informs us of many total separate source files this bin will produce
        IF binBytes(b) >= SplitChunkBytes * 2 THEN
            chunks = (binBytes(b) + SplitChunkBytes - 1) \ SplitChunkBytes
        ELSE
            chunks = 1
        END IF
        soFar = 0
        chunk = 1
        unitbuf = 0

        ' Iterate over the SUB/FUNCTIONs in this bin
        FOR r = binStart(b) TO binStart(b + 1) - 1
            i = byBin(r)

            ' Start a new source file - either at the start of this bin, or
            ' when a new chunk is started
            IF unitbuf = 0 THEN
                unitCount = unitCount + 1
                u$ = SplitUnitName$(binName$(b), chunk)
                unitbuf = OpenBuffer&("O", tmpdir$ + u$ + ".cpp")
                WriteBufLine unitbuf, "#include " + CHR$(34) + "qbx_gen.h" + CHR$(34)
                WriteBufLine mkbuf, "QB_SPLIT_SRCS += $(PATH_INTERNAL_TEMP)/" + u$ + ".cpp"
                WriteBufLine rspbuf, rspdir$ + u$ + ".o"
            END IF
            WriteBufLine unitbuf, "#include " + CHR$(34) + "main" + _TOSTR$(i) + ".txt" + CHR$(34)

            ' Here we're listing all the file dependencies for make so that it
            ' knows to rebuild it if any of the files it includes get changed.
            n$ = _TOSTR$(i)
            WriteBufLine mkbuf, "$(PATH_INTERNAL_TEMP)/" + u$ + ".o: $(PATH_INTERNAL_TEMP)/main" + n$ + ".txt $(PATH_INTERNAL_TEMP)/data" + n$ + ".txt $(PATH_INTERNAL_TEMP)/free" + n$ + ".txt $(PATH_INTERNAL_TEMP)/ret" + n$ + ".txt"

            soFar = soFar + sizes(i)
            IF chunks > 1 AND chunk < chunks AND soFar >= binBytes(b) \ chunks THEN
                chunk = chunk + 1
                soFar = 0
                unitbuf = 0
            END IF
        NEXT
    NEXT

END SUB

' A translation unit's file name, derived from the source file its routines come
' from. The hash should ensure its uniqueness (given the relatively low number
' of units that would be typical even for larger programs), but we also include
' a readable portion for when viewing the temp folder directly.
FUNCTION SplitUnitName$ (srcFile$, chunk AS LONG)
    n$ = LCASE$(StrReplace$(srcFile$, "\", "/"))
    h~& = 2166136261
    FOR i = 1 TO LEN(n$)
        h~& = h~& XOR ASC(n$, i)
        h~& = h~& * 16777619
    NEXT
    b$ = n$
    x = _INSTRREV(b$, "/"): IF x THEN b$ = MID$(b$, x + 1)
    x = INSTR(b$, "."): IF x THEN b$ = LEFT$(b$, x - 1)
    t$ = ""
    FOR i = 1 TO LEN(b$)
        IF i > 24 THEN EXIT FOR
        a = ASC(b$, i)
        IF alphanumeric(a) THEN t$ = t$ + CHR$(a) ELSE t$ = t$ + "_"
    NEXT
    SplitUnitName$ = "tu_" + RIGHT$("00000000" + LCASE$(HEX$(h~&)), 8) + "_" + t$ + "_c" + _TOSTR$(chunk)
END FUNCTION

' Deletes generated files left in this instance's temp folder by an earlier build
' that this one did not produce. Lots of source changes such as removing
' SUB/FUNCTIONs, renaming files, etc. change the names or reduce the number of
' generated C++ files. Without cleaning them up they would just keep growing
' indefinitely, in particular the tu_*.cpp files which have largely random
' names.
'
' SBufN() is the record of every file this build produced, so anything that is
' not in it can be removed.
SUB PruneStaleGeneratedFiles
    REDIM names$(0 TO 255)

    ' We make a copy of _FILES$() since we're around to delete some of them,
    ' doing so while iterating the directory could give undefined results.
    f$ = _FILES$(tmpdir$ + "*")
    DO WHILE LEN(f$)
        IF n > UBOUND(names$) THEN REDIM _PRESERVE names$(0 TO UBOUND(names$) * 2 + 1)
        names$(n) = f$
        n = n + 1
        f$ = _FILES$
    LOOP

    FOR i = 0 TO n - 1
        f$ = names$(i)

        ' We avoid touching anything that QB64-PE didn't clearly create
        IF IsGeneratedFileName%(f$) THEN
            IF SBufNFind&(tmpdir$ + f$) < 0 THEN
                KILL tmpdir$ + f$
                ' For deleted source files we don't want to leave their .o
                ' around, since nothing else is going to clean it up.
                IF LCASE$(RIGHT$(f$, 4)) = ".cpp" THEN
                    o$ = tmpdir$ + LEFT$(f$, LEN(f$) - 4) + ".o"
                    IF _FILEEXISTS(o$) THEN KILL o$
                END IF
            END IF
        END IF
    NEXT
END SUB

' When doing a second compilation in the same process we need to forget the
' existing buffers so that their names are no longer in the list.
'
' The next compilation may not produce the same set of files (Ex. tu_ files may
' get renamed, there might be less SUB/FUNCTIONs, etc.), clearing this list out
' ensure PruneStaleGeneratedFiles can correctly remove any files no longer needed.
SUB ForgetGeneratedFileBuffers
    FOR buf& = 0 TO UBOUND(SBufN)
        IF LEN(SBufN(buf&)) THEN
            IF LEFT$(SBufN(buf&), LEN(tmpdir$)) = tmpdir$ THEN
                IF IsGeneratedFileName%(MID$(SBufN(buf&), LEN(tmpdir$) + 1)) THEN ClearBufferAt buf&
            END IF
        END IF
    NEXT
END SUB

' This indicates whether the given file is one that QB64-PE has generated (based
' on the variety of file names it can produce). We use this for both cleaning up
' the internal file buffers, and also cleaning up the files on disk, without
' touching anything that QB64-PE doesn't make during compilation.
'
' Note that this does not match _everything_, only the file names that have
' generated names and could pile up in the temp folder.
FUNCTION IsGeneratedFileName% (f$)
    n$ = LCASE$(f$)

    'One per translation unit, when SplitObjectFiles is on.
    IF SplitUnitFileName%(n$) THEN IsGeneratedFileName% = -1: EXIT FUNCTION

    IF RIGHT$(n$, 4) <> ".txt" THEN EXIT FUNCTION
    n$ = LEFT$(n$, LEN(n$) - 4)

    'One set per SUB/FUNCTION, numbered by routine.
    IF NumberedGeneratedFile%(n$, "main") THEN IsGeneratedFileName% = -1: EXIT FUNCTION
    IF NumberedGeneratedFile%(n$, "data") THEN IsGeneratedFileName% = -1: EXIT FUNCTION
    IF NumberedGeneratedFile%(n$, "free") THEN IsGeneratedFileName% = -1: EXIT FUNCTION
    IF NumberedGeneratedFile%(n$, "ret") THEN IsGeneratedFileName% = -1: EXIT FUNCTION

    'One per DECLARE LIBRARY routine whose linkage the nm pass had to spell out.
    IF NumberedGeneratedFile%(n$, "externtype") THEN IsGeneratedFileName% = -1: EXIT FUNCTION

    'One per CHAIN/RUN target.
    IF NumberedGeneratedFile%(n$, "chain") THEN IsGeneratedFileName% = -1: EXIT FUNCTION
    IF NumberedGeneratedFile%(n$, "inpchain") THEN IsGeneratedFileName% = -1: EXIT FUNCTION
END FUNCTION

' Tells whether a lower-cased file name is a translation unit, by the whole shape
' SplitUnitName$ gives one: tu_, the hash, a readable tail, the chunk number.
FUNCTION SplitUnitFileName% (n$)
    IF LEFT$(n$, 3) <> "tu_" THEN EXIT FUNCTION
    IF RIGHT$(n$, 4) <> ".cpp" THEN EXIT FUNCTION

    b$ = MID$(n$, 4, LEN(n$) - 7)
    IF LEN(b$) < 9 THEN EXIT FUNCTION
    IF MID$(b$, 9, 1) <> "_" THEN EXIT FUNCTION
    FOR i = 1 TO 8
        IF INSTR("0123456789abcdef", MID$(b$, i, 1)) = 0 THEN EXIT FUNCTION
    NEXT

    x = _INSTRREV(b$, "_c")
    IF x <= 9 THEN EXIT FUNCTION
    SplitUnitFileName% = NumberedGeneratedFile%(MID$(b$, x + 1), "c")
END FUNCTION

' Tells whether name$ is prefix$ followed by a number and nothing else, so that
' main0 matches "main" but main.txt, maindata and main_split do not.
FUNCTION NumberedGeneratedFile% (name$, prefix$)
    IF LEN(name$) <= LEN(prefix$) THEN EXIT FUNCTION
    IF LEFT$(name$, LEN(prefix$)) <> prefix$ THEN EXIT FUNCTION
    FOR i = LEN(prefix$) + 1 TO LEN(name$)
        a = ASC(name$, i)
        IF a < 48 OR a > 57 THEN EXIT FUNCTION
    NEXT
    NumberedGeneratedFile% = -1
END FUNCTION

' Emit the bodies of _COMPILEDATE$/_COMPILETIME$/_COMPILERVERSION$, and only
' the specific ones the program calls.
SUB WriteCompilationInfoFuncs (buf AS LONG)
    IF UsesCompileDate THEN
        WriteBufLine buf, "qbs *func__compdate() {"
        WriteBufLine buf, "return qbs_new_txt(__DATE__);"
        WriteBufLine buf, "}"
    END IF
    IF UsesCompileTime THEN
        WriteBufLine buf, "qbs *func__comptime() {"
        WriteBufLine buf, "return qbs_new_txt(__TIME__);"
        WriteBufLine buf, "}"
    END IF
    IF UsesCompilerVersion THEN
        WriteBufLine buf, "qbs *func__compvers() {"
        WriteBufLine buf, "return qbs_new_txt(" + CHR$(34) + "QB64-PE v" + Version$ + CHR$(34) + ");"
        WriteBufLine buf, "}"
    END IF
END SUB
