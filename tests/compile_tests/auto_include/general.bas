'Test to prove the general auto-include files
'beforefirstline.bi and afterlastline.bm are really included.

$CONSOLE:ONLY

'beforefirstline.bi contains lots of constants,
'we print some to prove the file is included
PRINT "Prove availability of beforfirstline.bi constants:"
PRINT "--------------------------------------------------"
PRINT "Base of natural logarithm:"; _E
PRINT "Seconds in day...........:"; _SECS_IN_DAY
PRINT "One Gigabyte.............:"; _ONE_GB
PRINT

'afterlastline.bm contains some support functions,
'let's call one to prove the file is included
PRINT "Prove availability of afterlastline.bm functions:"
PRINT "-------------------------------------------------"
PRINT "Encoding a URL...........: "; _ENCODEURL$("http://w%w.example&.com/")

SYSTEM

