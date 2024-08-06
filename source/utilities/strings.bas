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
    str2$ = LTRIM$(STR$(v))
END FUNCTION

FUNCTION str2u64$ (v~&&)
    str2u64$ = LTRIM$(STR$(v~&&))
END FUNCTION

FUNCTION str2i64$ (v&&)
    str2i64$ = LTRIM$(STR$(v&&))
END FUNCTION

