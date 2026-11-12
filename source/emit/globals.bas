' Declaring the compiled program's globals.
'
' Everything at file scope goes out twice: the definition into global.txt,
' which only the main translation unit compiles, and an extern declaration of
' it into globalx.txt, which every other unit includes through qbx_gen.h. These
' helpers serve to make it easy to write to both at the same time.

' Emit a variable definition through defdatahandle, which points at GlobTxtBuf at
' module level but at a SUB's DataTxtBuf inside a SUB. Only the former is a program
' global, so only that one is mirrored into globalx.txt - a SUB local must not be.
SUB DeclareGlobalData (decl$)
    WriteBufLine defdatahandle, decl$
    IF defdatahandle = GlobTxtBuf THEN WriteBufLine GlobDeclBuf, "extern " + DeclWithoutInitializer$(decl$)
END SUB

' Emit a program-global variable definition and its extern.
SUB DeclareGlobalVar (decl$)
    WriteBufLine GlobTxtBuf, decl$
    WriteBufLine GlobDeclBuf, "extern " + DeclWithoutInitializer$(decl$)
END SUB

' Emit a generated helper function into global.txt and its prototype into
' globalx.txt. signature$ is the opening line, ending in "){".
SUB DeclareGlobalFunc (signature$)
    WriteBufLine GlobTxtBuf, signature$
    WriteBufLine GlobDeclBuf, LEFT$(signature$, LEN(signature$) - 1) + ";"
END SUB

' This is for things like static inline functions or macros which should be
' included the same into every file.
SUB DeclareGlobalVerbatim (text$)
    WriteBufLine GlobTxtBuf, text$
    WriteBufLine GlobDeclBuf, text$
END SUB

' This strips the initializer value from a variable declaration, Ex:
' "int32 console=1;" -> "int32 console;".
'
' The process is simply looking for the first `=` and removing everything after
' it. As a sanity check we make sure the `=` is not within a quoted section,
' which could indicate we found an incorrect one.
FUNCTION DeclWithoutInitializer$ (decl$)
    x = INSTR(decl$, "=")
    IF x THEN
        head$ = LEFT$(decl$, x - 1)
        IF INSTR(head$, CHR$(34)) OR INSTR(head$, "'") THEN Give_Error "Internal declaration has a literal before its initializer: " + decl$: EXIT FUNCTION
        DeclWithoutInitializer$ = head$ + ";"
    ELSE
        DeclWithoutInitializer$ = decl$
    END IF
END FUNCTION

' Add a declaration to global.txt once code generation has finished. The nm pass
' runs that late, so the buffer's cursor has to be put back at the end first.
' Writing the file directly instead would leave the buffer and the file out of
' step, and the file would then be rewritten on every build.
SUB AppendGlobalLine (decl$)
    DIM h AS LONG
    h = OpenBuffer&("A", tmpdir$ + "global.txt")
    WriteBufLine h, decl$
END SUB
