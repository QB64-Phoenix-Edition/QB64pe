'formatting is default at the beginning,
'so the TYPE should be reformatted
TYPE test
 one   AS LONG
 two   AS LONG
 three AS LONG
 four  AS LONG
END TYPE

'the following include will leave formatting off, but
'that should not affect the main code, hence the TYPE
'after the include should still be reformatted
'$include: 'format-off.bm'
TYPE test2
 one   AS LONG
 two   AS LONG
 three AS LONG
 four  AS LONG
END TYPE

'$format:off
'now formatting is off, so the DATAs should remain as we
'manually aligned it, even the include in between, which
'switches formatting on again, should not change that
DATA 0,   0,   0
DATA 0,   0,   0
'$include: 'format-on.bm'
DATA 0,   0,   0
DATA 0,   0,   0

'$format:on
'now formatting is on again, so the next DATAs will change
DATA 0,   0,   0
DATA 0,   0,   0
DATA 0,   0,   0

'$format:off
'$format:off
'$format:off
'multiple same switches don't hurt, there's no nesting behavior,
'it's simply off and the TYPE should remain aligned as is
TYPE test3
 one   AS LONG
 two   AS LONG
 three AS LONG
 four  AS LONG
END TYPE

'$format:on
'formatting is back on with a single call, even we had 3x off above,
'once again there's no nesting, the TYPE gets formatted
TYPE test4
 one   AS LONG
 two   AS LONG
 three AS LONG
 four  AS LONG
END TYPE

'$format:on
'no problem here either, formatting is on already but a 2nd switch
'doesn't hurt, the DATAs get formatted
DATA 0,   0,   0
DATA 0,   0,   0
DATA 0,   0,   0

