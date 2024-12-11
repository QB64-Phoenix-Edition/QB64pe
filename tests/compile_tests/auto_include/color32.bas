'Test to prove auto-including of $COLOR:32 constants on demand.

$CONSOLE:ONLY
$COLOR:32

'Let's also try to re-assign a color CONST to a new CONST to prove
'availability of the color constants during pre-pass
CONST newGreen = Green

PRINT "Some $COLOR:32 (color32.bi) constants:"
PRINT "--------------------------------------"
PRINT "Red.....:"; Red
PRINT "Green...:"; newGreen
PRINT "Blue....:"; Blue

SYSTEM

