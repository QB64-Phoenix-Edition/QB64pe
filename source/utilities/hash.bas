
FUNCTION HashValue& (a$) 'returns the hash table value of a string
    '[5(first)][5(second)][5(last)][5(2nd-last)][3(length AND 7)][1(first char is underscore)]
    l = LEN(a$)
    IF l = 0 THEN EXIT FUNCTION 'an (invalid) NULL string equates to 0
    a = ASC(a$)
    IF a <> 95 THEN 'does not begin with underscore
        SELECT CASE l
            CASE 1
                HashValue& = hash1char(a) + 1048576
                EXIT FUNCTION
            CASE 2
                HashValue& = hash2char(CVI(a$)) + 2097152
                EXIT FUNCTION
            CASE 3
                HashValue& = hash2char(CVI(a$)) + hash1char(ASC(a$, 3)) * 1024 + 3145728
                EXIT FUNCTION
            CASE ELSE
                HashValue& = hash2char(CVI(a$)) + hash2char(ASC(a$, l) + ASC(a$, l - 1) * 256) * 1024 + (l AND 7) * 1048576
                EXIT FUNCTION
        END SELECT
    ELSE 'does begin with underscore
        SELECT CASE l
            CASE 1
                HashValue& = (1048576 + 8388608): EXIT FUNCTION 'note: underscore only is illegal in QB64 but supported by hash
            CASE 2
                HashValue& = hash1char(ASC(a$, 2)) + (2097152 + 8388608)
                EXIT FUNCTION
            CASE 3
                HashValue& = hash2char(ASC(a$, 2) + ASC(a$, 3) * 256) + (3145728 + 8388608)
                EXIT FUNCTION
            CASE 4
                HashValue& = hash2char((CVL(a$) AND &HFFFF00) \ 256) + hash1char(ASC(a$, 4)) * 1024 + (4194304 + 8388608)
                EXIT FUNCTION
            CASE ELSE
                HashValue& = hash2char((CVL(a$) AND &HFFFF00) \ 256) + hash2char(ASC(a$, l) + ASC(a$, l - 1) * 256) * 1024 + (l AND 7) * 1048576 + 8388608
                EXIT FUNCTION
        END SELECT
    END IF
END FUNCTION

SUB HashAdd (a$, flags, reference)

    'find the index to use
    IF HashListFreeLast > 0 THEN
        'take from free list
        i = HashListFree(HashListFreeLast)
        HashListFreeLast = HashListFreeLast - 1
    ELSE
        IF HashListNext > HashListSize THEN
            'double hash list size
            HashListSize = HashListSize * 2
            REDIM _PRESERVE HashList(1 TO HashListSize) AS HashListItem
            REDIM _PRESERVE HashListName(1 TO HashListSize) AS STRING * 256
        END IF
        i = HashListNext
        HashListNext = HashListNext + 1
    END IF

    'setup links to index
    x = HashValue(a$)
    i2 = HashTable(x)
    IF i2 THEN
        i3 = HashList(i2).LastItem
        HashList(i2).LastItem = i
        HashList(i3).NextItem = i
        HashList(i).PrevItem = i3
    ELSE
        HashTable(x) = i
        HashList(i).PrevItem = 0
        HashList(i).LastItem = i
    END IF
    HashList(i).NextItem = 0

    'set common hashlist values
    HashList(i).Flags = flags
    HashList(i).Reference = reference
    HashListName(i) = UCASE$(a$)

END SUB

FUNCTION HashFind (a$, searchflags, resultflags, resultreference)
    '(0,1,2)z=hashfind[rev]("RUMI",Hashflag_label,resflag,resref)
    '0=doesn't exist
    '1=found, no more items to scan
    '2=found, more items still to scan
    i = HashTable(HashValue(a$))
    IF i THEN
        ua$ = UCASE$(a$) + SPACE$(256 - LEN(a$))
        hashfind_next:
        f = HashList(i).Flags
        IF searchflags AND f THEN 'flags in common
            IF HashListName(i) = ua$ THEN
                resultflags = f
                resultreference = HashList(i).Reference
                i2 = HashList(i).NextItem
                IF i2 THEN
                    HashFind = 2
                    HashFind_NextListItem = i2
                    HashFind_Reverse = 0
                    HashFind_SearchFlags = searchflags
                    HashFind_Name = ua$
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                ELSE
                    HashFind = 1
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                END IF
            END IF
        END IF
        i = HashList(i).NextItem
        IF i THEN GOTO hashfind_next
    END IF
END FUNCTION

FUNCTION HashFindRev (a$, searchflags, resultflags, resultreference)
    '(0,1,2)z=hashfind[rev]("RUMI",Hashflag_label,resflag,resref)
    '0=doesn't exist
    '1=found, no more items to scan
    '2=found, more items still to scan
    i = HashTable(HashValue(a$))
    IF i THEN
        i = HashList(i).LastItem
        ua$ = UCASE$(a$) + SPACE$(256 - LEN(a$))
        hashfindrev_next:
        f = HashList(i).Flags
        IF searchflags AND f THEN 'flags in common
            IF HashListName(i) = ua$ THEN
                resultflags = f
                resultreference = HashList(i).Reference
                i2 = HashList(i).PrevItem
                IF i2 THEN
                    HashFindRev = 2
                    HashFind_NextListItem = i2
                    HashFind_Reverse = 1
                    HashFind_SearchFlags = searchflags
                    HashFind_Name = ua$
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                ELSE
                    HashFindRev = 1
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                END IF
            END IF
        END IF
        i = HashList(i).PrevItem
        IF i THEN GOTO hashfindrev_next
    END IF
END FUNCTION

FUNCTION HashFindCont (resultflags, resultreference)
    '(0,1,2)z=hashfind[rev](resflag,resref)
    '0=no more items exist
    '1=found, no more items to scan
    '2=found, more items still to scan
    IF HashFind_Reverse THEN

        i = HashFind_NextListItem
        hashfindrevc_next:
        f = HashList(i).Flags
        IF HashFind_SearchFlags AND f THEN 'flags in common
            IF HashListName(i) = HashFind_Name THEN
                resultflags = f
                resultreference = HashList(i).Reference
                i2 = HashList(i).PrevItem
                IF i2 THEN
                    HashFindCont = 2
                    HashFind_NextListItem = i2
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                ELSE
                    HashFindCont = 1
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                END IF
            END IF
        END IF
        i = HashList(i).PrevItem
        IF i THEN GOTO hashfindrevc_next
        EXIT FUNCTION

    ELSE

        i = HashFind_NextListItem
        hashfindc_next:
        f = HashList(i).Flags
        IF HashFind_SearchFlags AND f THEN 'flags in common
            IF HashListName(i) = HashFind_Name THEN
                resultflags = f
                resultreference = HashList(i).Reference
                i2 = HashList(i).NextItem
                IF i2 THEN
                    HashFindCont = 2
                    HashFind_NextListItem = i2
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                ELSE
                    HashFindCont = 1
                    HashRemove_LastFound = i
                    EXIT FUNCTION
                END IF
            END IF
        END IF
        i = HashList(i).NextItem
        IF i THEN GOTO hashfindc_next
        EXIT FUNCTION

    END IF
END FUNCTION

SUB HashRemove

    i = HashRemove_LastFound

    'add to free list
    HashListFreeLast = HashListFreeLast + 1
    IF HashListFreeLast > HashListFreeSize THEN
        HashListFreeSize = HashListFreeSize * 2
        REDIM _PRESERVE HashListFree(1 TO HashListFreeSize) AS LONG
    END IF
    HashListFree(HashListFreeLast) = i

    'unlink
    i1 = HashList(i).PrevItem
    IF i1 THEN
        'not first item in list
        i2 = HashList(i).NextItem
        IF i2 THEN
            '(not first and) not last item
            HashList(i1).NextItem = i2
            HashList(i2).LastItem = i1
        ELSE
            'last item
            x = HashTable(HashValue(HashListName$(i)))
            HashList(x).LastItem = i1
            HashList(i1).NextItem = 0
        END IF
    ELSE
        'first item in list
        x = HashTable(HashValue(HashListName$(i)))
        i2 = HashList(i).NextItem
        IF i2 THEN
            '(first item but) not last item
            HashTable(x) = i2
            HashList(i2).PrevItem = 0
            HashList(i2).LastItem = HashList(i).LastItem
        ELSE
            '(first and) last item
            HashTable(x) = 0
        END IF
    END IF

END SUB

SUB HashDump 'used for debugging purposes
    fh = FREEFILE
    OPEN "hashdump.txt" FOR OUTPUT AS #fh
    b$ = "12345678901234567890123456789012}"
    FOR x = 0 TO 16777215
        IF HashTable(x) THEN

            PRINT #fh, "START HashTable("; x; "):"
            i = HashTable(x)

            'validate
            lasti = HashList(i).LastItem
            IF HashList(i).LastItem = 0 OR HashList(i).PrevItem <> 0 OR HashValue(HashListName(i)) <> x THEN GOTO corrupt

            PRINT #fh, "  HashList("; i; ").LastItem="; HashList(i).LastItem
            hashdumpnextitem:
            x$ = "  [" + STR$(i) + "]" + HashListName(i)

            f = HashList(i).Flags
            x$ = x$ + ",.Flags=" + STR$(f) + "{"
            FOR z = 1 TO 32
                ASC(b$, z) = (f AND 1) + 48
                f = f \ 2
            NEXT
            x$ = x$ + b$

            x$ = x$ + ",.Reference=" + STR$(HashList(i).Reference)

            PRINT #fh, x$

            'validate
            i1 = HashList(i).PrevItem
            i2 = HashList(i).NextItem
            IF i1 THEN
                IF HashList(i1).NextItem <> i THEN GOTO corrupt
            END IF
            IF i2 THEN
                IF HashList(i2).PrevItem <> i THEN GOTO corrupt
            END IF
            IF i2 = 0 THEN
                IF lasti <> i THEN GOTO corrupt
            END IF

            i = HashList(i).NextItem
            IF i THEN GOTO hashdumpnextitem

            PRINT #fh, "END HashTable("; x; ")"
        END IF
    NEXT
    CLOSE #fh

    EXIT SUB
    corrupt:
    PRINT #fh, "HASH TABLE CORRUPT!" 'should never happen
    CLOSE #fh

END SUB

SUB HashClear 'clear entire hash table

    HashListSize = 65536
    HashListNext = 1
    HashListFreeSize = 1024
    HashListFreeLast = 0
    REDIM HashList(1 TO HashListSize) AS HashListItem
    REDIM HashListName(1 TO HashListSize) AS STRING * 256
    REDIM HashListFree(1 TO HashListFreeSize) AS LONG
    REDIM HashTable(16777215) AS LONG '64MB lookup table with indexes to the hashlist

    HashFind_NextListItem = 0
    HashFind_Reverse = 0
    HashFind_SearchFlags = 0
    HashFind_Name = ""
    HashRemove_LastFound = 0

END SUB

