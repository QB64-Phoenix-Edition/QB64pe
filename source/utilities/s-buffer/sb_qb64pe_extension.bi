'--- This array holds the file names for the used buffers, the array is
'--- directly indexed using the buffer handles. This array is comparable
'--- to the "table of contents" of the harddrive.
'--- Avoid direct access, use the provided SUBs and FUNCTIONs.
'-----
REDIM SHARED SBufN(0 TO 99) AS STRING 'init for 100 buffers

