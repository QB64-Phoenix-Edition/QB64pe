' Shared state for the translation-unit writer in split_units.bas. The parse loop
' in qb64pe.bas fills these in as it goes.

' Per SUB/FUNCTION bookkeeping for splitting the generated C++ into separate
' translation units, indexed by subfuncn. Grown alongside subfuncnlast and reset
' at the start of every compilation pass.
'
'   SrcFile - the $INCLUDE file the routine is declared in. Routines are grouped
'             into translation units by this, so that editing one .bi/.bm only
'             invalidates the unit built from it.
'   CcallId - MKL$ list of the DECLARE LIBRARY routines it calls, resolved in
'             closemain once every declaration has been seen.
CONST SubFuncMetaGrow = 256
REDIM SHARED SubFuncSrcFile(SubFuncMetaGrow) AS STRING
REDIM SHARED SubFuncCcallIds(SubFuncMetaGrow) AS STRING

' Roughly how many bytes of generated C++ to aim for per object file. Only a file
' much larger than this is split.
CONST SplitChunkBytes = 1000000

' Which of _COMPILEDATE$/_COMPILETIME$/_COMPILERVERSION$ the program calls.
DIM SHARED UsesCompileDate AS _BYTE
DIM SHARED UsesCompileTime AS _BYTE
DIM SHARED UsesCompilerVersion AS _BYTE
