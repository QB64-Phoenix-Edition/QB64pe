'$INCLUDE:'.\type_definitions.bas'

$If OPENGL_GLOBAL_BAS = UNDEFINED Then
    ReDim Shared GL_COMMANDS(2000) As GL_idstruct
    Dim Shared GL_HELPER_CODE As String
    Dim Shared GL_COMMANDS_LAST
    ReDim Shared GL_DEFINES(2000) As String 'average ~600 entries
    ReDim Shared GL_DEFINES_VALUE(2000) As _Integer64
    Dim Shared GL_DEFINES_LAST
    Dim Shared GL_KIT: GL_KIT = 0
$End If
$Let OPENGL_GLOBAL_BAS = TRUE
