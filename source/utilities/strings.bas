'
' String manipulation functions
'
FUNCTION StrRemove$ (myString$, whatToRemove$) 'noncase sensitive
    DIM a$, b$
    DIM AS LONG i

    a$ = myString$
    b$ = LCASE$(whatToRemove$)
    i = INSTR(LCASE$(a$), b$)
    DO WHILE i
        a$ = LEFT$(a$, i - 1) + RIGHT$(a$, LEN(a$) - i - LEN(b$) + 1)
        i = INSTR(LCASE$(a$), b$)
    LOOP
    StrRemove$ = a$
END FUNCTION

FUNCTION StrReplace$ (myString$, find$, replaceWith$) 'noncase sensitive
    DIM a$, b$
    DIM AS LONG basei, i
    IF LEN(myString$) = 0 THEN EXIT FUNCTION
    a$ = myString$
    b$ = LCASE$(find$)
    basei = 1
    i = INSTR(basei, LCASE$(a$), b$)
    DO WHILE i
        a$ = LEFT$(a$, i - 1) + replaceWith$ + RIGHT$(a$, LEN(a$) - i - LEN(b$) + 1)
        basei = i + LEN(replaceWith$)
        i = INSTR(basei, LCASE$(a$), b$)
    LOOP
    StrReplace$ = a$
END FUNCTION

' Returns true if text has a certain enclosing pair like 'hello'
FUNCTION HasStringEnclosingPair%% (text AS STRING, pair AS STRING)
    IF LEN(text) > 1 AND LEN(pair) > 1 THEN
        HasStringEnclosingPair = ASC(pair, 1) = ASC(text, 1) AND ASC(pair, 2) = ASC(text, LEN(text))
    END IF
END FUNCTION

' Removes a string's enclosing pair if it is found. So, 'hello world' can be returned as just hello world
' pair - is the enclosing pair. E.g. "''", "()", "[]" etc.
FUNCTION RemoveStringEnclosingPair$ (text AS STRING, pair AS STRING)
    IF HasStringEnclosingPair(text, pair) THEN
        RemoveStringEnclosingPair = MID$(text, 2, LEN(text) - 2)
    ELSE
        RemoveStringEnclosingPair = text
    END IF
END FUNCTION

FUNCTION AddQuotes$ (s$)
    AddQuotes$ = CHR$(34) + s$ + CHR$(34)
END FUNCTION

'
' Convert a boolean value to 'True' or 'False'
'
FUNCTION BoolToTFString$ (b AS LONG)
    IF b THEN
        BoolToTFString$ = "True"
    ELSE
        BoolToTFString$ = "False"
    END IF
END FUNCTION

'
' Convert 'True' or 'False' to a boolean value
'
' Any string not 'True' or 'False' is returned as -2
'
FUNCTION TFStringToBool% (s AS STRING)
    DIM s2 AS STRING
    s2 = _TRIM$(UCASE$(s))

    IF s2 = "TRUE" THEN
        TFStringToBool% = -1
    ELSEIF s2 = "FALSE" THEN
        TFStringToBool% = 0
    ELSE
        TFStringToBool% = -2
    END IF
END FUNCTION

SUB WriteConfigSetting (section$, item$, value$)
    WriteSetting ConfigFile$, section$, item$, value$
END SUB

FUNCTION ReadConfigSetting (section$, item$, value$)
    value$ = ReadSetting$(ConfigFile$, section$, item$)
    ReadConfigSetting = (LEN(value$) > 0)
END FUNCTION

'
' Reads the bool setting at section:setting. If it is not there or invalid, writes the default value to it
'
FUNCTION ReadWriteBooleanSettingValue% (section AS STRING, setting AS STRING, default AS INTEGER)
    DIM checkResult AS INTEGER
    DIM value AS STRING
    DIM result AS INTEGER

    result = ReadConfigSetting(section, setting, value)

    checkResult = TFStringToBool%(value)

    IF checkResult = -2 THEN
        WriteConfigSetting section, setting, BoolToTFString$(default)
        ReadWriteBooleanSettingValue% = default
    ELSE
        ReadWriteBooleanSettingValue% = checkResult
    END IF
END FUNCTION

'
' Reads the string setting at section:setting. If it is not there or invalid, writes the default value to it
'
FUNCTION ReadWriteStringSettingValue$ (section AS STRING, setting AS STRING, default AS STRING)
    DIM value AS STRING
    DIM result AS INTEGER

    result = ReadConfigSetting(section, setting, value)

    IF result = 0 THEN
        WriteConfigSetting section, setting, default
        ReadWriteStringSettingValue$ = default
    ELSE
        ReadWriteStringSettingValue$ = value
    END IF
END FUNCTION

'
' Reads the integer setting at section:setting. If it is not there or invalid, writes the default value to it
'
' Verifies the value is positive and non-zero
'
FUNCTION ReadWriteLongSettingValue& (section AS STRING, setting AS STRING, default AS LONG)
    DIM value AS STRING
    DIM result AS INTEGER
    DIM checkResult AS LONG

    result = ReadConfigSetting(section, setting, value)

    checkResult = VAL(value)

    IF result = 0 OR checkResult <= 0 THEN
        WriteConfigSetting section, setting, str2$(default)
        ReadWriteLongSettingValue& = default
    ELSE
        ReadWriteLongSettingValue& = checkResult
    END IF
END FUNCTION

FUNCTION str2$ (v AS LONG)
    str2$ = _TRIM$(STR$(v))
END FUNCTION

FUNCTION str2u64$ (v~&&)
    str2u64$ = LTRIM$(RTRIM$(STR$(v~&&)))
END FUNCTION

FUNCTION str2i64$ (v&&)
    str2i64$ = LTRIM$(RTRIM$(STR$(v&&)))
END FUNCTION

'
' Build the CRC32 checksum of a string message
'
FUNCTION GetCRC32& (MsgData$)
STATIC lut~&()
'--- option _explicit requirements ---
DIM i%, crc~&, i&
'--- init lookup table on first call ---
$CHECKING:OFF
REDIM _PRESERVE lut~&(0 TO 255)
IF lut~&(1) = 0 THEN
    RESTORE GetCRC32_LookupTable
    FOR i% = 0 TO 255: READ lut~&(i%): NEXT i%
END IF
'--- compute CRC32 ---
crc~& = &HFFFFFFFF
FOR i& = 1 TO LEN(MsgData$)
    crc~& = ((crc~& \ 256) XOR lut~&((crc~& XOR ASC(MsgData$, i&)) AND 255))
NEXT i&
$CHECKING:ON
'--- set result & cleanup ---
GetCRC32& = (crc~& XOR &HFFFFFFFF)
EXIT FUNCTION
'--------------------
GetCRC32_LookupTable:
DATA &H00000000,&H77073096,&HEE0E612C,&H990951BA,&H076DC419,&H706AF48F,&HE963A535,&H9E6495A3
DATA &H0EDB8832,&H79DCB8A4,&HE0D5E91E,&H97D2D988,&H09B64C2B,&H7EB17CBD,&HE7B82D07,&H90BF1D91
DATA &H1DB71064,&H6AB020F2,&HF3B97148,&H84BE41DE,&H1ADAD47D,&H6DDDE4EB,&HF4D4B551,&H83D385C7
DATA &H136C9856,&H646BA8C0,&HFD62F97A,&H8A65C9EC,&H14015C4F,&H63066CD9,&HFA0F3D63,&H8D080DF5
DATA &H3B6E20C8,&H4C69105E,&HD56041E4,&HA2677172,&H3C03E4D1,&H4B04D447,&HD20D85FD,&HA50AB56B
DATA &H35B5A8FA,&H42B2986C,&HDBBBC9D6,&HACBCF940,&H32D86CE3,&H45DF5C75,&HDCD60DCF,&HABD13D59
DATA &H26D930AC,&H51DE003A,&HC8D75180,&HBFD06116,&H21B4F4B5,&H56B3C423,&HCFBA9599,&HB8BDA50F
DATA &H2802B89E,&H5F058808,&HC60CD9B2,&HB10BE924,&H2F6F7C87,&H58684C11,&HC1611DAB,&HB6662D3D
DATA &H76DC4190,&H01DB7106,&H98D220BC,&HEFD5102A,&H71B18589,&H06B6B51F,&H9FBFE4A5,&HE8B8D433
DATA &H7807C9A2,&H0F00F934,&H9609A88E,&HE10E9818,&H7F6A0DBB,&H086D3D2D,&H91646C97,&HE6635C01
DATA &H6B6B51F4,&H1C6C6162,&H856530D8,&HF262004E,&H6C0695ED,&H1B01A57B,&H8208F4C1,&HF50FC457
DATA &H65B0D9C6,&H12B7E950,&H8BBEB8EA,&HFCB9887C,&H62DD1DDF,&H15DA2D49,&H8CD37CF3,&HFBD44C65
DATA &H4DB26158,&H3AB551CE,&HA3BC0074,&HD4BB30E2,&H4ADFA541,&H3DD895D7,&HA4D1C46D,&HD3D6F4FB
DATA &H4369E96A,&H346ED9FC,&HAD678846,&HDA60B8D0,&H44042D73,&H33031DE5,&HAA0A4C5F,&HDD0D7CC9
DATA &H5005713C,&H270241AA,&HBE0B1010,&HC90C2086,&H5768B525,&H206F85B3,&HB966D409,&HCE61E49F
DATA &H5EDEF90E,&H29D9C998,&HB0D09822,&HC7D7A8B4,&H59B33D17,&H2EB40D81,&HB7BD5C3B,&HC0BA6CAD
DATA &HEDB88320,&H9ABFB3B6,&H03B6E20C,&H74B1D29A,&HEAD54739,&H9DD277AF,&H04DB2615,&H73DC1683
DATA &HE3630B12,&H94643B84,&H0D6D6A3E,&H7A6A5AA8,&HE40ECF0B,&H9309FF9D,&H0A00AE27,&H7D079EB1
DATA &HF00F9344,&H8708A3D2,&H1E01F268,&H6906C2FE,&HF762575D,&H806567CB,&H196C3671,&H6E6B06E7
DATA &HFED41B76,&H89D32BE0,&H10DA7A5A,&H67DD4ACC,&HF9B9DF6F,&H8EBEEFF9,&H17B7BE43,&H60B08ED5
DATA &HD6D6A3E8,&HA1D1937E,&H38D8C2C4,&H4FDFF252,&HD1BB67F1,&HA6BC5767,&H3FB506DD,&H48B2364B
DATA &HD80D2BDA,&HAF0A1B4C,&H36034AF6,&H41047A60,&HDF60EFC3,&HA867DF55,&H316E8EEF,&H4669BE79
DATA &HCB61B38C,&HBC66831A,&H256FD2A0,&H5268E236,&HCC0C7795,&HBB0B4703,&H220216B9,&H5505262F
DATA &HC5BA3BBE,&HB2BD0B28,&H2BB45A92,&H5CB36A04,&HC2D7FFA7,&HB5D0CF31,&H2CD99E8B,&H5BDEAE1D
DATA &H9B64C2B0,&HEC63F226,&H756AA39C,&H026D930A,&H9C0906A9,&HEB0E363F,&H72076785,&H05005713
DATA &H95BF4A82,&HE2B87A14,&H7BB12BAE,&H0CB61B38,&H92D28E9B,&HE5D5BE0D,&H7CDCEFB7,&H0BDBDF21
DATA &H86D3D2D4,&HF1D4E242,&H68DDB3F8,&H1FDA836E,&H81BE16CD,&HF6B9265B,&H6FB077E1,&H18B74777
DATA &H88085AE6,&HFF0F6A70,&H66063BCA,&H11010B5C,&H8F659EFF,&HF862AE69,&H616BFFD3,&H166CCF45
DATA &HA00AE278,&HD70DD2EE,&H4E048354,&H3903B3C2,&HA7672661,&HD06016F7,&H4969474D,&H3E6E77DB
DATA &HAED16A4A,&HD9D65ADC,&H40DF0B66,&H37D83BF0,&HA9BCAE53,&HDEBB9EC5,&H47B2CF7F,&H30B5FFE9
DATA &HBDBDF21C,&HCABAC28A,&H53B39330,&H24B4A3A6,&HBAD03605,&HCDD70693,&H54DE5729,&H23D967BF
DATA &HB3667A2E,&HC4614AB8,&H5D681B02,&H2A6F2B94,&HB40BBE37,&HC30C8EA1,&H5A05DF1B,&H2D02EF8D
END FUNCTION

