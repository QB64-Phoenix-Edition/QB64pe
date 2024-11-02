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

'afterlastline.bm contains some functions,
'let's call some to prove the file is included
PRINT "Prove availability of afterlastline.bm functions:"
PRINT "-------------------------------------------------"
PRINT "Minimum of 5.0 and  4.999999:"; _MIN(5.0, 4.999999)
PRINT "Maximum of 5.000001 and -4.9:"; _MAX(5.000001, -4.9)
PRINT "Encoding a URL..............: "; _ENCODEURL$("http://w%w.example&.com/")

SYSTEM

