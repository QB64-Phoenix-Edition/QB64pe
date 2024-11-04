$CONSOLE:ONLY

smi! = -3.402823E+38
sma! = 3.402823E+38
dmi# = -1.797693134862315D+308
dma# = 1.797693134862315D+308
fmi## = -1.189731495357231765F+4932
fma## = 1.189731495357231765F+4932
PRINT
PRINT "("; _STR$(smi!); ")"
PRINT "("; _STR$(sma!); ")"
PRINT
PRINT "("; _STR$(dmi#); ")"
PRINT "("; _STR$(dma#); ")"
PRINT
PRINT "("; _STR$(fmi##); ")"
PRINT "("; _STR$(fma##); ")"
PRINT
PRINT "("; STR$(fmi##); ")"
PRINT "("; STR$(fma##); ")"

SYSTEM

