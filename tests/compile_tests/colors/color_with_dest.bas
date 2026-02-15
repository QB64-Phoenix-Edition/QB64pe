$CONSOLE:ONLY

'COLOR works screen mode sensitive, so we need to check all
'to make sure the detination is properly reset by each branch.
DIM img&(0 TO 13)
img&(0) = _NEWIMAGE(25, 40, 0)
img&(1) = _NEWIMAGE(25, 40, 1)
img&(2) = _NEWIMAGE(25, 40, 2)

'Modes 3-6 not supported (we use it for 256/32 modes)
img&(4) = _NEWIMAGE(25, 40, 256)
img&(5) = _NEWIMAGE(25, 40, 32)

img&(7) = _NEWIMAGE(25, 40, 7)
img&(8) = _NEWIMAGE(25, 40, 8)
img&(9) = _NEWIMAGE(25, 40, 9)
img&(10) = _NEWIMAGE(25, 40, 10)
img&(11) = _NEWIMAGE(25, 40, 11)
img&(12) = _NEWIMAGE(25, 40, 12)
img&(13) = _NEWIMAGE(25, 40, 13)

FOR i% = 0 TO 13
    SELECT CASE i%
        CASE 3, 6: _CONTINUE 'skip
        CASE ELSE
            PRINT
            PRINT "SCREEN Mode"; _IIF(i% = 4, 256, _IIF(i% = 5, 32, i%))
            PRINT "---------------"
            PRINT "_DEST before = "; _DEST
            COLOR 1, 0, , img&(i%)
            PRINT "_DEST after  = "; _DEST
            _FREEIMAGE img&(i%)
    END SELECT
NEXT i%

SYSTEM

