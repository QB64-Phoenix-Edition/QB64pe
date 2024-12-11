'Test to prove auto-including of $COLOR:0 constants on demand.

$CONSOLE:ONLY
$COLOR:0

'Let's also try to re-assign a color CONST to a new CONST to prove
'availability of the color constants during pre-pass
CONST newGreen = Green

PRINT "Some $COLOR:0 (color0.bi) constants:"
PRINT "------------------------------------"
PRINT "Red.....:"; Red
PRINT "Green...:"; newGreen
PRINT "Blue....:"; Blue

SYSTEM

