$CONSOLE:ONLY

' Include a file that itself includes another file. It does so in a relative
' fashion

Dim includeCount As Long

'$include:'../extra/include_extra_include.bi'
'$include:'./tests/compile_tests/extra/include_extra_include.bi'
'$include:'tests/compile_tests/extra/include_extra_include.bi'

SYSTEM
