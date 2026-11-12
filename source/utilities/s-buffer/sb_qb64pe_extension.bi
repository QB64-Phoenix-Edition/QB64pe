'--- This array holds the file names for the used buffers, the array is
'--- directly indexed using the buffer handles. This array is comparable
'--- to the "table of contents" of the harddrive.
'--- Avoid direct access, use the provided SUBs and FUNCTIONs.
'-----
REDIM SHARED SBufN(0 TO 99) AS STRING 'init for 100 buffers
REDIM SHARED SBufCppLineStatus(0 TO 99) AS _Byte ' Tracks whether to emit a #line, used for buffers writing C++ code

'--- This tracks which buffers have been retired: written out and erased from
'--- memory, but still tracked within the library.
'-----
' A retired buffer keeps its name, its hash table entry and its recorded length,
' so its file still counts as one this build produced and GetBufLen&() still
' reports its size, but its contents are gone. See RetireBuffer().
REDIM SHARED SBufRetired(0 TO 99) AS _Byte

'--- Hash table over SBufN(), so that OpenBuffer&() can find an already open
'--- buffer without comparing its name against every other open buffer.
'-----
' SBufNHashTable() holds "handle + 1" of the first buffer in each bucket (0 when
' the bucket is empty), SBufNHashNext() is indexed by "handle" and chains the
' rest of the bucket.
CONST SBufNHashMask = 65535
REDIM SHARED SBufNHashTable(SBufNHashMask) AS LONG
REDIM SHARED SBufNHashNext(0 TO 99) AS LONG

'--- Bytes compared at a time when a buffer is checked against its file.
'-----
CONST BufCompareChunkSize = 65536

'--- Cache of the last "#line" file name resolved by AddCppLine
'-----
DIM SHARED AddCppLine_suffix AS STRING, AddCppLine_name AS STRING
DIM SHARED AddCppLine_source AS STRING, AddCppLine_base AS STRING
DIM SHARED AddCppLine_internal AS LONG, AddCppLine_absolute AS LONG

