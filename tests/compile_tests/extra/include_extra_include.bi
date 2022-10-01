
' Include a file relative to the location of this file (which itself is included from elsewhere)

' Both relative formats should work

'$include:'./include_extra_target.bi'
'$include:'include_extra_target.bi'

' Absolute position relative to the compiler should work too

'$include:'./tests/compile_tests/extra/include_extra_target.bi'
'$include:'tests/compile_tests/extra/include_extra_target.bi'
