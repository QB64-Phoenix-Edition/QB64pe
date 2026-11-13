'Array to handle $EMBED metacommand:
REDIM SHARED embedFileList$(3, 10)
CONST eflLine = 0, eflUsed = 1, eflFile = 2, eflHand = 3 '1st index IDs

' embeds.txt is the record of what embedded.cpp was last generated from, this
' allows a build to cheaply verify whether it needs to be regenerated. The
' current format is below:
'
'   qb64pe-embed-manifest 1 <length of embedded.cpp> <qb64pe version>
'   <size> <mtime> <hash> <packed> <handle> <file>
'
' The two fields that may contain spaces - the version and the file name - come
' last on their lines. The version is recorded because we don't guarentee this
' file will work across different QB64-PE versions.
CONST EmbedManTag = "qb64pe-embed-manifest 1"
REDIM SHARED EmbedManHand$(15), EmbedManPath$(15), EmbedManHash$(15)
REDIM SHARED EmbedManSize(15) AS _INTEGER64, EmbedManTime(15) AS _INTEGER64
REDIM SHARED EmbedManPacked(15) AS _BYTE
DIM SHARED EmbedManCount AS LONG, EmbedManCppLen AS _INTEGER64
