
TYPE ConstFunction
    nam AS STRING

    ArgCount AS INTEGER ' If positive, this is the number of argument this function accepts
END TYPE
REDIM SHARED ConstFuncs(1000) AS ConstFunction

TYPE ParseNum
    f AS _FLOAT
    i AS _INTEGER64
    ui AS _UNSIGNED _INTEGER64
    s AS STRING
    typ AS LONG
END TYPE

CONST CONST_EVAL_DEBUG = 0

Set_ConstFunctions

