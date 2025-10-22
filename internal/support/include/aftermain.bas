'==========================================================================
' This file is automatically included in each compiled program right after
' the main program section.
'----------
' The purpose of this file is, for the auto-include logic, to inject an
' implicit END instruction first, before including other "IncAfterMain"
' library files from the $USELIBRARY metacommand, which could contain
' further GOSUB routines or error handlers. It is to avoid the program
' can run unimpededly into such library code, just in case the user has
' no explicit END or SYSTEM at the end of his main code.
' !!! There should be nothing else in this file !!!
'----------
' IMPORTANT
'   The use of this file is exclusively reserved to QB64-PE itself,
'   do NOT add your own personal stuff here !!
'==========================================================================

$INCLUDEONCE

END

