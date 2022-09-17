Option _Explicit
$Console
$ScreenHide
_Dest _Console

Dim As Long i, note, duration
Dim word As String
Dim As Double t, ct, d1, d2

Play "mb"

t = Timer
For i = 1 To 34
    Read note, duration, word
    Sound note, duration: Print word$;
Next
Print
ct = Timer
If t > ct Then t = t - 86400
d1 = ct - t

Restore
Play "mf"

t = Timer
For i = 1 To 34
    Read note, duration, word
    Sound note, duration: Print word$;
Next
Print
ct = Timer
If t > ct Then t = t - 86400
d2 = ct - t

If d2 - d1 > 5 Then
    Print "Foreground playback always takes longer to complete than background playback."
End If

System

Data 392,8,"My ",659,8,"Bon-",587,8,"nie ",523,8,"lies ",587,8,"O-",523,8,"Ver ",440,8,"the "
Data 392,8,"O-",330,32,"cean ",392,8,"My ",659,8,"Bon-",587,8,"nie ",523,8,"lies "
Data 523,8,"O-",494,8,"ver ",523,8,"the ",587,40,"sea ",392,8,"My ",659,8,"Bon-",587,8,"nie"
Data 523,8," lies ",587,8,"O-",523,8,"ver ",440,8,"the ",392,8,"O-",330,32,"cean ",392,8,"Oh "
Data 440,8,"bring ",587,8,"back ",523,8,"my ",494,8,"Bon-",440,8,"nie ",494,8,"to ",523,32,"me..!"
