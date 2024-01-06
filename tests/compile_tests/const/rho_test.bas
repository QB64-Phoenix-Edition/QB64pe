
CONST chunkSIZEOF% = 4 + 4

CONST CHformLEN% = 4
CONST formSIZEOF% = chunkSIZEOF% + CHformLEN% 'remove trailing % and it works

CONST CHthdrLEN% = 30 + 2 + 4
CONST thdrSIZEOF% = chunkSIZEOF% + CHthdrLEN% 'no error

CONST CHcsetLEN% = 16 + 264 + 2 + 2 + 2 + 2
CONST csetSIZEOF% = chunkSIZEOF% + CHcsetLEN% 'no error

CONST CHwposLEN% = 30 + 2 + 2
CONST wposSIZEOF% = chunkSIZEOF% + CHwposLEN% 'no error

CONST CHvarsLEN% = 4 + 2 + 2
CONST varsSIZEOF% = chunkSIZEOF% + CHvarsLEN% 'no error

CONST CHtlogLEN% = 30 + 12 + 80
CONST tlogSIZEOF% = chunkSIZEOF% + CHtlogLEN% 'no error
