If _DirExists("internal") = 0 Then
    _ScreenShow
    Print "QB64 cannot locate the 'internal' folder"
    Print
    Print "Check that QB64 has been extracted properly."
    Print "For MacOSX, launch 'qb64_start.command' or enter './qb64' in Terminal."
    Print "For Linux, in the console enter './qb64'."
    Do
        _Limit 1
    Loop Until InKey$ <> ""
    System 1
End If

Dim Shared Cache_Folder As String
Cache_Folder$ = "internal\help"
If InStr(_OS$, "WIN") = 0 Then Cache_Folder$ = "internal/help"
If _DirExists(Cache_Folder$) = 0 Then MkDir Cache_Folder$



