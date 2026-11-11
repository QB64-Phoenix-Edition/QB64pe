$CONSOLE:ONLY
OPTION _EXPLICIT

DIM AS LONG i, note, duration
DIM word AS STRING
DIM AS DOUBLE t, ct, d1, d2

PLAY "mb"

t = TIMER
FOR i = 1 TO 34
    READ note, duration, word
    SOUND note, duration: PRINT word$;
NEXT
PRINT
ct = TIMER
IF t > ct THEN t = t - 86400
d1 = ct - t

RESTORE
PLAY "mf"

t = TIMER
FOR i = 1 TO 34
    READ note, duration, word
    SOUND note, duration: PRINT word$;
NEXT
PRINT
ct = TIMER
IF t > ct THEN t = t - 86400
d2 = ct - t

IF (d2 - d1 > 5) _ORELSE (INSTR(_OS$, "LINUX") > 0 AND INSTR(_OS$, "MACOSX") = 0) THEN
    PRINT "Foreground playback always takes longer to complete than background playback."
END IF

SYSTEM

DATA 392,8,"My ",659,8,"Bon-",587,8,"nie ",523,8,"lies ",587,8,"O-",523,8,"Ver ",440,8,"the "
DATA 392,8,"O-",330,32,"cean ",392,8,"My ",659,8,"Bon-",587,8,"nie ",523,8,"lies "
DATA 523,8,"O-",494,8,"ver ",523,8,"the ",587,40,"sea ",392,8,"My ",659,8,"Bon-",587,8,"nie"
DATA 523,8," lies ",587,8,"O-",523,8,"ver ",440,8,"the ",392,8,"O-",330,32,"cean ",392,8,"Oh "
DATA 440,8,"bring ",587,8,"back ",523,8,"my ",494,8,"Bon-",440,8,"nie ",494,8,"to ",523,32,"me..!"
