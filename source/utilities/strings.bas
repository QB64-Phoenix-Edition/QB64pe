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
IF lut~&(1) = 0 THEN GOSUB GetCRC32_LookupTable
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
lut~&(0) = &H00000000: lut~&(1) = &H77073096: lut~&(2) = &HEE0E612C: lut~&(3) = &H990951BA
lut~&(4) = &H076DC419: lut~&(5) = &H706AF48F: lut~&(6) = &HE963A535: lut~&(7) = &H9E6495A3
lut~&(8) = &H0EDB8832: lut~&(9) = &H79DCB8A4: lut~&(10) = &HE0D5E91E: lut~&(11) = &H97D2D988
lut~&(12) = &H09B64C2B: lut~&(13) = &H7EB17CBD: lut~&(14) = &HE7B82D07: lut~&(15) = &H90BF1D91
lut~&(16) = &H1DB71064: lut~&(17) = &H6AB020F2: lut~&(18) = &HF3B97148: lut~&(19) = &H84BE41DE
lut~&(20) = &H1ADAD47D: lut~&(21) = &H6DDDE4EB: lut~&(22) = &HF4D4B551: lut~&(23) = &H83D385C7
lut~&(24) = &H136C9856: lut~&(25) = &H646BA8C0: lut~&(26) = &HFD62F97A: lut~&(27) = &H8A65C9EC
lut~&(28) = &H14015C4F: lut~&(29) = &H63066CD9: lut~&(30) = &HFA0F3D63: lut~&(31) = &H8D080DF5
lut~&(32) = &H3B6E20C8: lut~&(33) = &H4C69105E: lut~&(34) = &HD56041E4: lut~&(35) = &HA2677172
lut~&(36) = &H3C03E4D1: lut~&(37) = &H4B04D447: lut~&(38) = &HD20D85FD: lut~&(39) = &HA50AB56B
lut~&(40) = &H35B5A8FA: lut~&(41) = &H42B2986C: lut~&(42) = &HDBBBC9D6: lut~&(43) = &HACBCF940
lut~&(44) = &H32D86CE3: lut~&(45) = &H45DF5C75: lut~&(46) = &HDCD60DCF: lut~&(47) = &HABD13D59
lut~&(48) = &H26D930AC: lut~&(49) = &H51DE003A: lut~&(50) = &HC8D75180: lut~&(51) = &HBFD06116
lut~&(52) = &H21B4F4B5: lut~&(53) = &H56B3C423: lut~&(54) = &HCFBA9599: lut~&(55) = &HB8BDA50F
lut~&(56) = &H2802B89E: lut~&(57) = &H5F058808: lut~&(58) = &HC60CD9B2: lut~&(59) = &HB10BE924
lut~&(60) = &H2F6F7C87: lut~&(61) = &H58684C11: lut~&(62) = &HC1611DAB: lut~&(63) = &HB6662D3D
lut~&(64) = &H76DC4190: lut~&(65) = &H01DB7106: lut~&(66) = &H98D220BC: lut~&(67) = &HEFD5102A
lut~&(68) = &H71B18589: lut~&(69) = &H06B6B51F: lut~&(70) = &H9FBFE4A5: lut~&(71) = &HE8B8D433
lut~&(72) = &H7807C9A2: lut~&(73) = &H0F00F934: lut~&(74) = &H9609A88E: lut~&(75) = &HE10E9818
lut~&(76) = &H7F6A0DBB: lut~&(77) = &H086D3D2D: lut~&(78) = &H91646C97: lut~&(79) = &HE6635C01
lut~&(80) = &H6B6B51F4: lut~&(81) = &H1C6C6162: lut~&(82) = &H856530D8: lut~&(83) = &HF262004E
lut~&(84) = &H6C0695ED: lut~&(85) = &H1B01A57B: lut~&(86) = &H8208F4C1: lut~&(87) = &HF50FC457
lut~&(88) = &H65B0D9C6: lut~&(89) = &H12B7E950: lut~&(90) = &H8BBEB8EA: lut~&(91) = &HFCB9887C
lut~&(92) = &H62DD1DDF: lut~&(93) = &H15DA2D49: lut~&(94) = &H8CD37CF3: lut~&(95) = &HFBD44C65
lut~&(96) = &H4DB26158: lut~&(97) = &H3AB551CE: lut~&(98) = &HA3BC0074: lut~&(99) = &HD4BB30E2
lut~&(100) = &H4ADFA541: lut~&(101) = &H3DD895D7: lut~&(102) = &HA4D1C46D: lut~&(103) = &HD3D6F4FB
lut~&(104) = &H4369E96A: lut~&(105) = &H346ED9FC: lut~&(106) = &HAD678846: lut~&(107) = &HDA60B8D0
lut~&(108) = &H44042D73: lut~&(109) = &H33031DE5: lut~&(110) = &HAA0A4C5F: lut~&(111) = &HDD0D7CC9
lut~&(112) = &H5005713C: lut~&(113) = &H270241AA: lut~&(114) = &HBE0B1010: lut~&(115) = &HC90C2086
lut~&(116) = &H5768B525: lut~&(117) = &H206F85B3: lut~&(118) = &HB966D409: lut~&(119) = &HCE61E49F
lut~&(120) = &H5EDEF90E: lut~&(121) = &H29D9C998: lut~&(122) = &HB0D09822: lut~&(123) = &HC7D7A8B4
lut~&(124) = &H59B33D17: lut~&(125) = &H2EB40D81: lut~&(126) = &HB7BD5C3B: lut~&(127) = &HC0BA6CAD
lut~&(128) = &HEDB88320: lut~&(129) = &H9ABFB3B6: lut~&(130) = &H03B6E20C: lut~&(131) = &H74B1D29A
lut~&(132) = &HEAD54739: lut~&(133) = &H9DD277AF: lut~&(134) = &H04DB2615: lut~&(135) = &H73DC1683
lut~&(136) = &HE3630B12: lut~&(137) = &H94643B84: lut~&(138) = &H0D6D6A3E: lut~&(139) = &H7A6A5AA8
lut~&(140) = &HE40ECF0B: lut~&(141) = &H9309FF9D: lut~&(142) = &H0A00AE27: lut~&(143) = &H7D079EB1
lut~&(144) = &HF00F9344: lut~&(145) = &H8708A3D2: lut~&(146) = &H1E01F268: lut~&(147) = &H6906C2FE
lut~&(148) = &HF762575D: lut~&(149) = &H806567CB: lut~&(150) = &H196C3671: lut~&(151) = &H6E6B06E7
lut~&(152) = &HFED41B76: lut~&(153) = &H89D32BE0: lut~&(154) = &H10DA7A5A: lut~&(155) = &H67DD4ACC
lut~&(156) = &HF9B9DF6F: lut~&(157) = &H8EBEEFF9: lut~&(158) = &H17B7BE43: lut~&(159) = &H60B08ED5
lut~&(160) = &HD6D6A3E8: lut~&(161) = &HA1D1937E: lut~&(162) = &H38D8C2C4: lut~&(163) = &H4FDFF252
lut~&(164) = &HD1BB67F1: lut~&(165) = &HA6BC5767: lut~&(166) = &H3FB506DD: lut~&(167) = &H48B2364B
lut~&(168) = &HD80D2BDA: lut~&(169) = &HAF0A1B4C: lut~&(170) = &H36034AF6: lut~&(171) = &H41047A60
lut~&(172) = &HDF60EFC3: lut~&(173) = &HA867DF55: lut~&(174) = &H316E8EEF: lut~&(175) = &H4669BE79
lut~&(176) = &HCB61B38C: lut~&(177) = &HBC66831A: lut~&(178) = &H256FD2A0: lut~&(179) = &H5268E236
lut~&(180) = &HCC0C7795: lut~&(181) = &HBB0B4703: lut~&(182) = &H220216B9: lut~&(183) = &H5505262F
lut~&(184) = &HC5BA3BBE: lut~&(185) = &HB2BD0B28: lut~&(186) = &H2BB45A92: lut~&(187) = &H5CB36A04
lut~&(188) = &HC2D7FFA7: lut~&(189) = &HB5D0CF31: lut~&(190) = &H2CD99E8B: lut~&(191) = &H5BDEAE1D
lut~&(192) = &H9B64C2B0: lut~&(193) = &HEC63F226: lut~&(194) = &H756AA39C: lut~&(195) = &H026D930A
lut~&(196) = &H9C0906A9: lut~&(197) = &HEB0E363F: lut~&(198) = &H72076785: lut~&(199) = &H05005713
lut~&(200) = &H95BF4A82: lut~&(201) = &HE2B87A14: lut~&(202) = &H7BB12BAE: lut~&(203) = &H0CB61B38
lut~&(204) = &H92D28E9B: lut~&(205) = &HE5D5BE0D: lut~&(206) = &H7CDCEFB7: lut~&(207) = &H0BDBDF21
lut~&(208) = &H86D3D2D4: lut~&(209) = &HF1D4E242: lut~&(210) = &H68DDB3F8: lut~&(211) = &H1FDA836E
lut~&(212) = &H81BE16CD: lut~&(213) = &HF6B9265B: lut~&(214) = &H6FB077E1: lut~&(215) = &H18B74777
lut~&(216) = &H88085AE6: lut~&(217) = &HFF0F6A70: lut~&(218) = &H66063BCA: lut~&(219) = &H11010B5C
lut~&(220) = &H8F659EFF: lut~&(221) = &HF862AE69: lut~&(222) = &H616BFFD3: lut~&(223) = &H166CCF45
lut~&(224) = &HA00AE278: lut~&(225) = &HD70DD2EE: lut~&(226) = &H4E048354: lut~&(227) = &H3903B3C2
lut~&(228) = &HA7672661: lut~&(229) = &HD06016F7: lut~&(230) = &H4969474D: lut~&(231) = &H3E6E77DB
lut~&(232) = &HAED16A4A: lut~&(233) = &HD9D65ADC: lut~&(234) = &H40DF0B66: lut~&(235) = &H37D83BF0
lut~&(236) = &HA9BCAE53: lut~&(237) = &HDEBB9EC5: lut~&(238) = &H47B2CF7F: lut~&(239) = &H30B5FFE9
lut~&(240) = &HBDBDF21C: lut~&(241) = &HCABAC28A: lut~&(242) = &H53B39330: lut~&(243) = &H24B4A3A6
lut~&(244) = &HBAD03605: lut~&(245) = &HCDD70693: lut~&(246) = &H54DE5729: lut~&(247) = &H23D967BF
lut~&(248) = &HB3667A2E: lut~&(249) = &HC4614AB8: lut~&(250) = &H5D681B02: lut~&(251) = &H2A6F2B94
lut~&(252) = &HB40BBE37: lut~&(253) = &HC30C8EA1: lut~&(254) = &H5A05DF1B: lut~&(255) = &H2D02EF8D
RETURN
END FUNCTION

