'Old QuickBasic "quick load" format
FUNCTION BinaryFormatCheck% (pathToCheck$, pathSepToCheck$, fileToCheck$)

    file$ = pathToCheck$ + pathSepToCheck$ + fileToCheck$

    fh = FREEFILE
    OPEN file$ FOR BINARY AS #fh
    a$ = SPACE$(LOF(fh))
    GET #fh, 1, a$
    IF INSTR(a$, CHR$(0)) = 0 THEN CLOSE #fh: EXIT FUNCTION 'not a binary file
    a$ = ""
    GET #fh, 1, Format%
    GET #fh, , Version%
    CLOSE #fh

    SELECT CASE Format%
        CASE 2300 'VBDOS
            result = idemessagebox("Invalid format", "VBDOS binary format not supported.", "")
            BinaryFormatCheck% = 1
        CASE 764 'QBX 7.1
            result = idemessagebox("Invalid format", "QBX 7.1 binary format not supported.", "")
            BinaryFormatCheck% = 1
        CASE 252 'QuickBASIC 4.5
            IF INSTR(_OS$, "WIN") THEN
                convertUtility$ = "internal\utilities\QB45BIN.exe"
            ELSE
                convertUtility$ = "./internal/utilities/QB45BIN"
            END IF
            IF _FILEEXISTS(convertUtility$) THEN
                what$ = ideyesnobox("Binary format", "QuickBASIC 4.5 binary format detected. Convert to plain text?")
                IF what$ = "Y" THEN
                    ConvertIt:
                    IF FileHasExtension(file$) THEN
                        FOR i = LEN(file$) TO 1 STEP -1
                            IF ASC(file$, i) = 46 THEN
                                'keep previous extension
                                ofile$ = LEFT$(file$, i - 1) + " (converted)" + MID$(file$, i)
                                EXIT FOR
                            END IF
                        NEXT
                    ELSE
                        ofile$ = file$ + " (converted).bas"
                    END IF

                    SCREEN , , 3, 0
                    dummy = DarkenFGBG(1)
                    clearStatusWindow 0
                    COLOR 15, 1
                    _PRINTSTRING (2, idewy - 3), "Converting...          "
                    PCOPY 3, 0

                    convertLine$ = convertUtility$ + " " + QuotedFilename$(file$) + " -o " + QuotedFilename$(ofile$)
                    SHELL _HIDE convertLine$

                    clearStatusWindow 0
                    dummy = DarkenFGBG(0)
                    PCOPY 3, 0

                    IF _FILEEXISTS(ofile$) = 0 THEN
                        result = idemessagebox("Binary format", "Conversion failed.", "")
                        BinaryFormatCheck% = 2 'conversion failed
                    ELSE
                        pathToCheck$ = getfilepath$(ofile$)
                        IF LEN(pathToCheck$) THEN
                            fileToCheck$ = MID$(ofile$, LEN(pathToCheck$) + 1)
                            pathToCheck$ = LEFT$(pathToCheck$, LEN(pathToCheck$) - 1) 'remove path separator
                        ELSE
                            fileToCheck$ = ofile$
                        END IF
                    END IF
                ELSE
                    BinaryFormatCheck% = 1
                END IF
            ELSE
                IF _FILEEXISTS("internal/support/converter/QB45BIN.bas") = 0 THEN
                    result = idemessagebox("Binary format", "Conversion utility not found. Cannot open QuickBASIC 4.5 binary format.", "")
                    BinaryFormatCheck% = 1
                    EXIT FUNCTION
                END IF
                what$ = ideyesnobox("Binary format", "QuickBASIC 4.5 binary format detected. Convert to plain text?")
                IF what$ = "Y" THEN
                    'Compile the utility first, then convert the file
                    IF _DIREXISTS("./internal/utilities") = 0 THEN MKDIR "./internal/utilities"
                    PCOPY 3, 0
                    SCREEN , , 3, 0
                    dummy = DarkenFGBG(1)
                    clearStatusWindow 0
                    COLOR 15, 1
                    _PRINTSTRING (2, idewy - 3), "Preparing to convert..."
                    PCOPY 3, 0
                    IF INSTR(_OS$, "WIN") THEN
                        SHELL _HIDE "qb64pe -x internal/support/converter/QB45BIN.bas -o internal/utilities/QB45BIN"
                    ELSE
                        SHELL _HIDE "./qb64pe -x ./internal/support/converter/QB45BIN.bas -o ./internal/utilities/QB45BIN"
                    END IF
                    IF _FILEEXISTS(convertUtility$) THEN GOTO ConvertIt
                    clearStatusWindow 0
                    dummy = DarkenFGBG(0)
                    PCOPY 3, 0
                    result = idemessagebox("Binary format", "Error launching conversion utility.", "")
                END IF
                BinaryFormatCheck% = 1
            END IF
    END SELECT
END FUNCTION

FUNCTION OfferNoprefixConversion% (file$)
    what$ = ideyesnobox("$NOPREFIX", "This program uses the $NOPREFIX directive which is unsupported.\n\nQB64PE can automatically convert this file and any included files to\nremove $NOPREFIX. Backups of all files will be made.\n\nConvert this program?")
    IF what$ <> "Y" THEN EXIT FUNCTION

    SCREEN , , 3, 0
    dummy = DarkenFGBG(1)
    COLOR 15, 1
    _PRINTSTRING (2, idewy - 3), "Converting...          "
    PCOPY 3, 0

    IF INSTR(_OS$, "WIN") THEN
        convertUtility$ = "internal\utilities\AddPREFIX.exe"
    ELSE
        convertUtility$ = "./internal/utilities/AddPREFIX"
    END IF
    IF NOT _FILEEXISTS(convertUtility$) THEN
        IF _DIREXISTS("./internal/utilities") = 0 THEN MKDIR "./internal/utilities"
        IF INSTR(_OS$, "WIN") THEN
            SHELL _HIDE "qb64pe -x internal/support/converter/AddPREFIX.bas -o " + convertUtility$
        ELSE
            SHELL _HIDE "./qb64pe -x ./internal/support/converter/AddPREFIX.bas -o " + convertUtility$
        END IF
    END IF

    convertLine$ = convertUtility$ + " " + QuotedFilename$(file$)
    IF _SHELLHIDE(convertLine$) = 0 _ANDALSO OpenFile$(file$) <> "C" THEN
        OfferNoprefixConversion% = -1
    ELSE
        clearStatusWindow 0
        dummy = DarkenFGBG(0)
        PCOPY 3, 0
        SCREEN , , 3, 0
        result = idemessagebox("$NOPREFIX", "Error running conversion utility.", "")
    END IF
END FUNCTION

