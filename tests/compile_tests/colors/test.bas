$CONSOLE:ONLY

PRINT "RGB functions forward"
PRINT "Grey, max alpha: "; HEX$(_RGB32(127))
PRINT "Grey, half alpha: "; HEX$(_RGB32(127, 127))
PRINT "Grey, max alpha: "; HEX$(_RGB32(127, 127, 127))
PRINT "Grey, half alpha: "; HEX$(_RGB32(127, 127, 127, 127))

c~& = _RGBA32(80, 128, 160, 32)
PRINT
PRINT "RGB functions backward _RGBA32(80, 128, 160, 32)"
PRINT "Red..:"; _RED32(c~&)
PRINT "Green:"; _GREEN32(c~&)
PRINT "Blue.:"; _BLUE32(c~&)
PRINT "Alpha:"; _ALPHA32(c~&)

PRINT
PRINT "HSB functions forward"
PRINT "Red, max alpha: "; HEX$(_HSB32(0, 100, 100))
PRINT "Red, half alpha: "; HEX$(_HSBA32(0, 100, 100, 50))
PRINT "Midgreen, max alpha: "; HEX$(_HSB32(120, 75, 75))
PRINT "Midgreen, 3/4 alpha: "; HEX$(_HSBA32(120, 75, 75, 75))

c~& = _HSBA32(90, 75, 65, 55)
PRINT
PRINT "HSB functions backward _HSBA32(90, 75, 65, 55)"
PRINT "Hue.......:"; CINT(_HUE32(c~&))
PRINT "Saturation:"; CINT(_SATURATION32(c~&))
PRINT "Brightness:"; CINT(_BRIGHTNESS32(c~&))
PRINT "Alpha.....:"; CINT(_ALPHA32(c~&) / 255 * 100)

SYSTEM

