$CONSOLE:ONLY

Option _Explicit

' VAL(..., numericalType) must carry the requested return type into
' overloaded runtime functions such as _TOSTR$.
Print _ToStr$(Val("16777217", Single))
Print _ToStr$(Val("1E100", Double))
Print _ToStr$(Val("1E100", _Float))

System
