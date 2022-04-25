$If SETTINGS_BAS = UNDEFINED Then
    'Used for debugging the compiler's code (not the code it compiles) [for temporary/advanced usage]
    Const Debug = 0

    'All variables will be of type LONG unless explicitly defined
    DefLng A-Z

    'All arrays will be dynamically allocated so they can be REDIM-ed
    '$DYNAMIC

    'We need console access to support command-line compilation via the -x command line compile option
    $Console

    'Initially the "SCREEN" will be hidden, if the -x option is used it will never be created
    $ScreenHide
$End If

$Let SETTINGS_BAS = TRUE
