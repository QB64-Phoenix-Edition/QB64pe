



' Deep-copy support for descriptor-backed TYPE member arrays.
' Emit C code for all descriptor-backed member arrays inside one UDT element.
'Numeric/fixed-size descriptor payloads can be copied with memcpy, but variable
'strings and nested owner UDTs need ownership-aware qbs/descriptor cloning so the
'source and destination never share owned payloads.
SUB AppendDynUDTDescCopy (dstbase$, srcbase$, udt AS LONG, dstoff$, srcoff$, bytesperelement$, acc$, layout_mode AS LONG)
    IF udtxvariable(udt) THEN
        AppendDynUDTOwnSetAt "((uint8*)(" + dstbase$ + ")+(" + dstoff$ + "))", "((uint8*)(" + srcbase$ + ")+(" + srcoff$ + "))", udt, 0, 0, bytesperelement$, "0", "0", acc$, layout_mode
        EXIT SUB
    END IF

    ' udtxvariable propagates from every nested UDT into its parent at TYPE-definition
    ' time. Therefore the descriptor-only traversal below cannot encounter a nested
    ' owner UDT; any such graph was handled by the owner branch above.
    DIM elemnum AS LONG
    DIM nestedudt AS LONG
    DIM dynoffbytes AS LONG
    DIM memberelembytes AS LONG
    DIM elemvarstr AS LONG
    DIM copycode AS STRING
    DIM nesteddst$
    DIM nestedsrc$
    DIM inline_bytes AS LONG
    DIM loop_name AS STRING

    elemnum = udtxnext(udt)
    DO WHILE elemnum
        dynoffbytes = UDTDynMemberOffset&(elemnum) \ 8
        IF UDTMemberDynDesc%(elemnum) THEN
            memberelembytes = udt_dyn_array_elem_bytes(elemnum)
            elemvarstr = DynMemVarStr%(elemnum)
            nestedudt = 0
            IF (udtetype(elemnum) AND ISUDT) <> 0 THEN
                nestedudt = udtetype(elemnum) AND UDTMASK
            END IF

            ' The generated C block clones one live member-array descriptor. It validates
            ' the source descriptor, allocates a fresh destination descriptor/data block,
            ' copies or deep-clones payload elements, and only then releases the old
            ' destination descriptor so failed allocation cannot destroy the target.
            copycode = "{" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint **dyn_dst_slot=(ptrszint**)(((uint8*)(" + dstbase$ + "))+(" + dstoff$ + "+" + _TOSTR$(dynoffbytes) + "));" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint **dyn_src_slot=(ptrszint**)(((uint8*)(" + srcbase$ + "))+(" + srcoff$ + "+" + _TOSTR$(dynoffbytes) + "));" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint *dyn_src_desc=*dyn_src_slot;" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint *dyn_dst_desc=*dyn_dst_slot;" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_src_desc&&(dyn_src_desc[2]&1)&&(dyn_src_desc[3]>0)){" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint dyn_dims=dyn_src_desc[3];" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint dyn_slots=dyn_dims*4+4+1;" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint dyn_lock_index=dyn_dims*4+4;" + CHR$(13) + CHR$(10)
            copycode = copycode + "uint64 dyn_total=1;" + CHR$(13) + CHR$(10)
            copycode = copycode + "for(ptrszint dyn_i=1; dyn_i<=dyn_dims; dyn_i++){" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint dyn_arg=(dyn_dims-dyn_i)*4+4;" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_src_desc[dyn_arg+1]<0) error(257);" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_src_desc[dyn_arg+1]&&dyn_total>(18446744073709551615ull/(uint64)dyn_src_desc[dyn_arg+1])) error(257);" + CHR$(13) + CHR$(10)
            copycode = copycode + "dyn_total*=(uint64)dyn_src_desc[dyn_arg+1];" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint *dyn_new_desc=(ptrszint*)calloc((size_t)dyn_slots,ptrsz);" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (!dyn_new_desc) error(257);" + CHR$(13) + CHR$(10)
            copycode = copycode + "memcpy((void*)dyn_new_desc,(void*)dyn_src_desc,(size_t)(dyn_slots-1)*ptrsz);" + CHR$(13) + CHR$(10)
            copycode = copycode + "new_mem_lock();" + CHR$(13) + CHR$(10)
            copycode = copycode + "mem_lock_tmp->type=4;" + CHR$(13) + CHR$(10)
            copycode = copycode + "dyn_new_desc[dyn_lock_index]=(ptrszint)mem_lock_tmp;" + CHR$(13) + CHR$(10)
            copycode = copycode + "dyn_new_desc[0]=(ptrszint)nothingvalue;" + CHR$(13) + CHR$(10)
            copycode = copycode + "uint64 dyn_bytes=dyn_total*(uint64)" + _TOSTR$(memberelembytes) + ";" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_bytes&&dyn_src_desc[0]&&dyn_src_desc[0]!=(ptrszint)nothingvalue){" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_src_desc[2]&4){" + CHR$(13) + CHR$(10)
            copycode = copycode + "dyn_new_desc[0]=(ptrszint)cmem_dynamic_malloc((size_t)dyn_bytes);" + CHR$(13) + CHR$(10)
            copycode = copycode + "}else{" + CHR$(13) + CHR$(10)
            copycode = copycode + "dyn_new_desc[0]=(ptrszint)malloc((size_t)dyn_bytes);" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (!dyn_new_desc[0]) error(257);" + CHR$(13) + CHR$(10)
            IF elemvarstr THEN
                AppendDynStrInit "(void*)dyn_new_desc[0]", "(ptrszint)dyn_total", memberelembytes, copycode
                AppendDynStrSet "(void*)dyn_new_desc[0]", "(void*)dyn_src_desc[0]", "(ptrszint)dyn_total", memberelembytes, copycode
            ELSEIF nestedudt <> 0 AND UDTDynHasMemberArrays%(nestedudt, layout_mode) THEN
                copycode = copycode + "for(ptrszint dyn_elem_i=0; dyn_elem_i<(ptrszint)dyn_total; dyn_elem_i++){" + CHR$(13) + CHR$(10)
                AppendDynUDTOwnInitAt "(void*)dyn_new_desc[0]", nestedudt, 0, _TOSTR$(memberelembytes), "dyn_elem_i", copycode, layout_mode
                IF Error_Happened THEN EXIT SUB
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
                copycode = copycode + "for(ptrszint dyn_elem_i=0; dyn_elem_i<(ptrszint)dyn_total; dyn_elem_i++){" + CHR$(13) + CHR$(10)
                AppendDynUDTOwnSetAt "(void*)dyn_new_desc[0]", "(void*)dyn_src_desc[0]", nestedudt, 0, 0, _TOSTR$(memberelembytes), "dyn_elem_i", "dyn_elem_i", copycode, layout_mode
                IF Error_Happened THEN EXIT SUB
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
            ELSE
                copycode = copycode + "memcpy((void*)dyn_new_desc[0],(void*)dyn_src_desc[0],(size_t)dyn_bytes);" + CHR$(13) + CHR$(10)
            END IF
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_dst_desc){" + CHR$(13) + CHR$(10)
            IF elemvarstr THEN
                copycode = copycode + "if ((dyn_dst_desc[2]&1)&&dyn_dst_desc[0]&&dyn_dst_desc[0]!=(ptrszint)nothingvalue&&dyn_dst_desc[3]>0){" + CHR$(13) + CHR$(10)
                copycode = copycode + "uint64 dyn_dst_total=1;" + CHR$(13) + CHR$(10)
                copycode = copycode + "for(ptrszint dyn_dst_i=1; dyn_dst_i<=dyn_dst_desc[3]; dyn_dst_i++){ptrszint dyn_dst_arg=(dyn_dst_desc[3]-dyn_dst_i)*4+4; dyn_dst_total*=(uint64)dyn_dst_desc[dyn_dst_arg+1];}" + CHR$(13) + CHR$(10)
                AppendDynStrFree "(void*)dyn_dst_desc[0]", "(ptrszint)dyn_dst_total", memberelembytes, copycode
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
            END IF
            IF nestedudt <> 0 AND UDTDynHasMemberArrays%(nestedudt, layout_mode) THEN
                copycode = copycode + "if ((dyn_dst_desc[2]&1)&&dyn_dst_desc[0]&&dyn_dst_desc[0]!=(ptrszint)nothingvalue&&dyn_dst_desc[3]>0){" + CHR$(13) + CHR$(10)
                copycode = copycode + "uint64 dyn_dst_total=1;" + CHR$(13) + CHR$(10)
                copycode = copycode + "for(ptrszint dyn_dst_i=1; dyn_dst_i<=dyn_dst_desc[3]; dyn_dst_i++){ptrszint dyn_dst_arg=(dyn_dst_desc[3]-dyn_dst_i)*4+4; dyn_dst_total*=(uint64)dyn_dst_desc[dyn_dst_arg+1];}" + CHR$(13) + CHR$(10)
                copycode = copycode + "for(ptrszint dyn_dst_elem=0; dyn_dst_elem<(ptrszint)dyn_dst_total; dyn_dst_elem++){" + CHR$(13) + CHR$(10)
                AppendDynUDTOwnFreeAt "(void*)dyn_dst_desc[0]", nestedudt, 0, _TOSTR$(memberelembytes), "dyn_dst_elem", copycode, layout_mode
                IF Error_Happened THEN EXIT SUB
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
            END IF
            copycode = copycode + "if ((dyn_dst_desc[2]&1)&&dyn_dst_desc[0]&&dyn_dst_desc[0]!=(ptrszint)nothingvalue){" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_dst_desc[2]&4) cmem_dynamic_free((uint8*)dyn_dst_desc[0]); else free((void*)dyn_dst_desc[0]);" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_dst_desc[3]>0){" + CHR$(13) + CHR$(10)
            copycode = copycode + "ptrszint dyn_dst_lock_index=dyn_dst_desc[3]*4+4;" + CHR$(13) + CHR$(10)
            copycode = copycode + "if (dyn_dst_desc[dyn_dst_lock_index]) free_mem_lock((mem_lock*)dyn_dst_desc[dyn_dst_lock_index]);" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "free((void*)dyn_dst_desc);" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "*dyn_dst_slot=dyn_new_desc;" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            copycode = copycode + "}" + CHR$(13) + CHR$(10)
            acc$ = acc$ + copycode
        ELSEIF (udtetype(elemnum) AND ISUDT) <> 0 THEN
            nestedudt = udtetype(elemnum) AND UDTMASK
            IF UDTDynHasMemberArrays%(nestedudt, layout_mode) THEN
                IF udtearrayelements(elemnum) THEN
                    ' Inline arrays of nested descriptor-bearing UDTs must clone every element.
                    ' Emit one compact C++ runtime loop so generated source size depends on TYPE
                    ' shape, not on the declared element count. The member id is globally unique
                    ' in the UDT metadata and therefore gives nested loops a collision-free name.
                    inline_bytes = UDTDynInlineElemBytes&(elemnum, layout_mode)
                    loop_name = "dyn_desc_copy_i" + _TOSTR$(elemnum)
                    acc$ = acc$ + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + _TOSTR$(udtearrayelements(elemnum)) + ";" + loop_name + "++){" + CHR$(13) + CHR$(10)
                    nesteddst$ = "(" + dstoff$ + "+" + _TOSTR$(dynoffbytes) + "+" + loop_name + "*" + _TOSTR$(inline_bytes) + ")"
                    nestedsrc$ = "(" + srcoff$ + "+" + _TOSTR$(dynoffbytes) + "+" + loop_name + "*" + _TOSTR$(inline_bytes) + ")"
                    AppendDynUDTDescCopy dstbase$, srcbase$, nestedudt, nesteddst$, nestedsrc$, _TOSTR$(inline_bytes), acc$, layout_mode
                    IF Error_Happened THEN EXIT SUB
                    acc$ = acc$ + "}" + CHR$(13) + CHR$(10)
                ELSE
                    nesteddst$ = "(" + dstoff$ + "+" + _TOSTR$(dynoffbytes) + ")"
                    nestedsrc$ = "(" + srcoff$ + "+" + _TOSTR$(dynoffbytes) + ")"
                    AppendDynUDTDescCopy dstbase$, srcbase$, nestedudt, nesteddst$, nestedsrc$, _TOSTR$(UDTDynMemberSize&(elemnum) \ 8), acc$, layout_mode
                    IF Error_Happened THEN EXIT SUB
                END IF
            END IF
        END IF
        elemnum = udtenext(elemnum)
    LOOP
END SUB

' Parse one lower-bound/element-count pair from serialized TYPE member-array bounds metadata.
FUNCTION ParseNextUDTArrayDescriptorDim& (descriptor$, descriptor_position AS LONG, lower_bound AS LONG, element_count AS LONG)
    IF descriptor_position <= 0 THEN descriptor_position = 1
    IF descriptor_position > LEN(descriptor$) THEN EXIT FUNCTION

    next_separator = INSTR(descriptor_position, descriptor$, ";")
    IF next_separator THEN
        pair$ = MID$(descriptor$, descriptor_position, next_separator - descriptor_position)
        descriptor_position = next_separator + 1
    ELSE
        pair$ = MID$(descriptor$, descriptor_position)
        descriptor_position = LEN(descriptor$) + 1
    END IF

    comma_pos = INSTR(pair$, ",")
    IF comma_pos = 0 THEN EXIT FUNCTION

    lower_bound = VAL(LEFT$(pair$, comma_pos - 1))
    element_count = VAL(MID$(pair$, comma_pos + 1))
    IF element_count <= 0 THEN EXIT FUNCTION

    ParseNextUDTArrayDescriptorDim = -1
END FUNCTION


FUNCTION typevalue2symbol$ (t)

    IF t AND ISSTRING THEN
        IF t AND ISFIXEDLENGTH THEN Give_Error "Cannot convert expression type to symbol": EXIT FUNCTION
        typevalue2symbol$ = "$"
        EXIT FUNCTION
    END IF

    s$ = ""

    IF t AND ISUNSIGNED THEN s$ = "~"

    b = t AND UDTMASK

    IF t AND ISOFFSETINBITS THEN
        IF b > 1 THEN s$ = s$ + "`" + _TOSTR$(b) ELSE s$ = s$ + "`"
        typevalue2symbol$ = s$
        EXIT FUNCTION
    END IF

    IF t AND ISFLOAT THEN
        IF b = 32 THEN s$ = "!"
        IF b = 64 THEN s$ = "#"
        IF b = 256 THEN s$ = "##"
        typevalue2symbol$ = s$
        EXIT FUNCTION
    END IF

    IF b = 8 THEN s$ = s$ + "%%"
    IF b = 16 THEN s$ = s$ + "%"
    IF b = 32 THEN s$ = s$ + "&"
    IF b = 64 THEN s$ = s$ + "&&"
    typevalue2symbol$ = s$

END FUNCTION

FUNCTION id2fulltypename$
    t = id.t
    IF t = 0 THEN t = id.arraytype
    size = id.tsize
    bits = t AND UDTMASK
    IF t AND ISUDT THEN
        a$ = RTRIM$(udtxcname(t AND UDTMASK))
        id2fulltypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISSTRING THEN
        IF t AND ISFIXEDLENGTH THEN a$ = "STRING * " + _TOSTR$(size) ELSE a$ = "STRING"
        id2fulltypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISOFFSETINBITS THEN
        IF bits > 1 THEN a$ = "_BIT * " + _TOSTR$(bits) ELSE a$ = "_BIT"
        IF t AND ISUNSIGNED THEN a$ = "_UNSIGNED " + a$
        id2fulltypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISFLOAT THEN
        IF bits = 32 THEN a$ = "SINGLE"
        IF bits = 64 THEN a$ = "DOUBLE"
        IF bits = 256 THEN a$ = "_FLOAT"
    ELSE 'integer-based
        IF bits = 8 THEN a$ = "_BYTE"
        IF bits = 16 THEN a$ = "INTEGER"
        IF bits = 32 THEN a$ = "LONG"
        IF bits = 64 THEN a$ = "_INTEGER64"
        IF t AND ISUNSIGNED THEN a$ = "_UNSIGNED " + a$
    END IF
    IF t AND ISOFFSET THEN
        a$ = "_OFFSET"
        IF t AND ISUNSIGNED THEN a$ = "_UNSIGNED " + a$
    END IF
    id2fulltypename$ = a$
END FUNCTION

FUNCTION id2shorttypename$
    t = id.t
    IF t = 0 THEN t = id.arraytype
    size = id.tsize
    bits = t AND UDTMASK
    IF t AND ISUDT THEN
        a$ = RTRIM$(udtxcname(t AND UDTMASK))
        id2shorttypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISSTRING THEN
        IF t AND ISFIXEDLENGTH THEN a$ = "STRING" + _TOSTR$(size) ELSE a$ = "STRING"
        id2shorttypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISOFFSETINBITS THEN
        IF t AND ISUNSIGNED THEN a$ = "_U" ELSE a$ = "_"
        IF bits > 1 THEN a$ = a$ + "BIT" + _TOSTR$(bits) ELSE a$ = a$ + "BIT1"
        id2shorttypename$ = a$: EXIT FUNCTION
    END IF
    IF t AND ISFLOAT THEN
        IF bits = 32 THEN a$ = "SINGLE"
        IF bits = 64 THEN a$ = "DOUBLE"
        IF bits = 256 THEN a$ = "_FLOAT"
    ELSE 'integer-based
        IF bits = 8 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "_UBYTE" ELSE a$ = "_BYTE"
        END IF
        IF bits = 16 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "UINTEGER" ELSE a$ = "INTEGER"
        END IF
        IF bits = 32 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "ULONG" ELSE a$ = "LONG"
        END IF
        IF bits = 64 THEN
            IF (t AND ISUNSIGNED) THEN a$ = "_UINTEGER64" ELSE a$ = "_INTEGER64"
        END IF
    END IF
    id2shorttypename$ = a$
END FUNCTION

FUNCTION symbol2fulltypename$ (s2$)
    'note: accepts both symbols and type names
    s$ = s2$

    IF LEFT$(s$, 1) = "~" THEN
        u = 1
        IF LEN(typ$) = 1 THEN Give_Error "Expected ~...": EXIT FUNCTION
        s$ = RIGHT$(s$, LEN(s$) - 1)
        u$ = "_UNSIGNED "
    END IF

    IF s$ = "%%" THEN t$ = u$ + "_BYTE": GOTO gotsym2typ
    IF s$ = "%" THEN t$ = u$ + "INTEGER": GOTO gotsym2typ
    IF s$ = "&" THEN t$ = u$ + "LONG": GOTO gotsym2typ
    IF s$ = "&&" THEN t$ = u$ + "_INTEGER64": GOTO gotsym2typ
    IF s$ = "%&" THEN t$ = u$ + "_OFFSET": GOTO gotsym2typ

    IF LEFT$(s$, 1) = "`" THEN
        IF LEN(s$) = 1 THEN
            t$ = u$ + "_BIT * 1"
            GOTO gotsym2typ
        END IF
        n$ = RIGHT$(s$, LEN(s$) - 1)
        IF isuinteger(n$) = 0 THEN Give_Error "Expected number after symbol `": EXIT FUNCTION
        t$ = u$ + "_BIT * " + n$
        GOTO gotsym2typ
    END IF

    IF u = 1 THEN Give_Error "Expected type symbol after ~": EXIT FUNCTION

    IF s$ = "!" THEN t$ = "SINGLE": GOTO gotsym2typ
    IF s$ = "#" THEN t$ = "DOUBLE": GOTO gotsym2typ
    IF s$ = "##" THEN t$ = "_FLOAT": GOTO gotsym2typ
    IF s$ = "$" THEN t$ = "STRING": GOTO gotsym2typ

    IF LEFT$(s$, 1) = "$" THEN
        n$ = RIGHT$(s$, LEN(s$) - 1)
        IF isuinteger(n$) = 0 THEN Give_Error "Expected number after symbol $": EXIT FUNCTION
        t$ = "STRING * " + n$
        GOTO gotsym2typ
    END IF

    t$ = s$

    gotsym2typ:

    IF RIGHT$(" " + t$, 5) = " _BIT" THEN t$ = t$ + " * 1" 'clarify (_UNSIGNED) _BIT as (_UNSIGNED) _BIT * 1

    FOR i = 1 TO LEN(t$)
        IF ASC(t$, i) = ASC(sp) THEN ASC(t$, i) = 32
    NEXT

    symbol2fulltypename$ = t$

END FUNCTION

FUNCTION symboltype (s$) 'returns type or 0(not a valid symbol)
    'note: sets symboltype_size for fixed length strings
    'created: 2011 (fast & comprehensive)
    IF LEN(s$) = 0 THEN EXIT FUNCTION
    'treat common cases first
    a = ASC(s$)
    l = LEN(s$)
    IF a = 37 THEN '%
        IF l = 1 THEN symboltype = 16: EXIT FUNCTION
        IF l > 2 THEN EXIT FUNCTION
        IF ASC(s$, 2) = 37 THEN symboltype = 8: EXIT FUNCTION
        IF ASC(s$, 2) = 38 THEN symboltype = OFFSETTYPE - ISPOINTER: EXIT FUNCTION '%&
        EXIT FUNCTION
    END IF
    IF a = 38 THEN '&
        IF l = 1 THEN symboltype = 32: EXIT FUNCTION
        IF l > 2 THEN EXIT FUNCTION
        IF ASC(s$, 2) = 38 THEN symboltype = 64: EXIT FUNCTION
        EXIT FUNCTION
    END IF
    IF a = 33 THEN '!
        IF l = 1 THEN symboltype = 32 + ISFLOAT: EXIT FUNCTION
        EXIT FUNCTION
    END IF
    IF a = 35 THEN '#
        IF l = 1 THEN symboltype = 64 + ISFLOAT: EXIT FUNCTION
        IF l > 2 THEN EXIT FUNCTION
        IF ASC(s$, 2) = 35 THEN symboltype = 64 + ISFLOAT: EXIT FUNCTION
        EXIT FUNCTION
    END IF
    IF a = 36 THEN '$
        IF l = 1 THEN symboltype = ISSTRING: EXIT FUNCTION
        IF isuinteger(RIGHT$(s$, l - 1)) THEN
            IF l >= (1 + 10) THEN
                IF l > (1 + 10) THEN EXIT FUNCTION
                IF s$ > "$2147483647" THEN EXIT FUNCTION
            END IF
            symboltype_size = VAL(RIGHT$(s$, l - 1))
            symboltype = ISSTRING + ISFIXEDLENGTH
            EXIT FUNCTION
        END IF
        EXIT FUNCTION
    END IF
    IF a = 96 THEN '`
        IF l = 1 THEN symboltype = 1 + ISOFFSETINBITS: EXIT FUNCTION
        IF isuinteger(RIGHT$(s$, l - 1)) THEN
            IF l > 3 THEN EXIT FUNCTION
            n = VAL(RIGHT$(s$, l - 1))
            IF n > 64 THEN EXIT FUNCTION
            symboltype = n + ISOFFSETINBITS: EXIT FUNCTION
        END IF
        EXIT FUNCTION
    END IF
    IF a = 126 THEN '~
        IF l = 1 THEN EXIT FUNCTION
        a = ASC(s$, 2)
        IF a = 37 THEN '%
            IF l = 2 THEN symboltype = 16 + ISUNSIGNED: EXIT FUNCTION
            IF l > 3 THEN EXIT FUNCTION
            IF ASC(s$, 3) = 37 THEN symboltype = 8 + ISUNSIGNED: EXIT FUNCTION
            IF ASC(s$, 3) = 38 THEN symboltype = UOFFSETTYPE - ISPOINTER: EXIT FUNCTION '~%&
            EXIT FUNCTION
        END IF
        IF a = 38 THEN '&
            IF l = 2 THEN symboltype = 32 + ISUNSIGNED: EXIT FUNCTION
            IF l > 3 THEN EXIT FUNCTION
            IF ASC(s$, 3) = 38 THEN symboltype = 64 + ISUNSIGNED: EXIT FUNCTION
            EXIT FUNCTION
        END IF
        IF a = 96 THEN '`
            IF l = 2 THEN symboltype = 1 + ISOFFSETINBITS + ISUNSIGNED: EXIT FUNCTION
            IF isuinteger(RIGHT$(s$, l - 2)) THEN
                IF l > 4 THEN EXIT FUNCTION
                n = VAL(RIGHT$(s$, l - 2))
                IF n > 64 THEN EXIT FUNCTION
                symboltype = n + ISOFFSETINBITS + ISUNSIGNED: EXIT FUNCTION
            END IF
            EXIT FUNCTION
        END IF
    END IF '~
END FUNCTION

FUNCTION typ2ctyp$ (t AS LONG, tstr AS STRING)
    ctyp$ = ""
    'typ can be passed as either: (the unused value is ignored)
    'i. as a typ value in t
    'ii. as a typ symbol (eg. "~%") in tstr
    'iii. as a typ name (eg. _UNSIGNED INTEGER) in tstr
    IF tstr$ = "" THEN
        IF (t AND ISARRAY) THEN EXIT FUNCTION 'cannot return array types
        IF (t AND ISSTRING) THEN typ2ctyp$ = "qbs": EXIT FUNCTION
        b = t AND UDTMASK
        IF (t AND ISUDT) THEN typ2ctyp$ = "void": EXIT FUNCTION
        IF (t AND ISOFFSETINBITS) THEN
            IF b <= 32 THEN ctyp$ = "int32" ELSE ctyp$ = "int64"
            IF (t AND ISUNSIGNED) THEN ctyp$ = "u" + ctyp$
            typ2ctyp$ = ctyp$: EXIT FUNCTION
        END IF
        IF (t AND ISFLOAT) THEN
            IF b = 32 THEN ctyp$ = "float"
            IF b = 64 THEN ctyp$ = "double"
            IF b = 256 THEN ctyp$ = "long double"
        ELSE
            IF b = 8 THEN ctyp$ = "int8"
            IF b = 16 THEN ctyp$ = "int16"
            IF b = 32 THEN ctyp$ = "int32"
            IF b = 64 THEN ctyp$ = "int64"
            IF t AND ISOFFSET THEN ctyp$ = "ptrszint"
            IF (t AND ISUNSIGNED) THEN ctyp$ = "u" + ctyp$
        END IF
        IF t AND ISOFFSET THEN
            ctyp$ = "ptrszint": IF (t AND ISUNSIGNED) THEN ctyp$ = "uptrszint"
        END IF
        typ2ctyp$ = ctyp$: EXIT FUNCTION
    END IF

    ts$ = tstr$
    'is ts$ a symbol?
    IF ts$ = "$" THEN ctyp$ = "qbs"
    IF ts$ = "!" THEN ctyp$ = "float"
    IF ts$ = "#" THEN ctyp$ = "double"
    IF ts$ = "##" THEN ctyp$ = "long double"
    IF LEFT$(ts$, 1) = "~" THEN unsgn = 1: ts$ = RIGHT$(ts$, LEN(ts$) - 1)
    IF LEFT$(ts$, 1) = "`" THEN
        n$ = RIGHT$(ts$, LEN(ts$) - 1)
        b = 1
        IF n$ <> "" THEN
            IF isuinteger(n$) = 0 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
            b = VAL(n$)
            IF b > 64 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
        END IF
        IF b <= 32 THEN ctyp$ = "int32" ELSE ctyp$ = "int64"
        IF unsgn THEN ctyp$ = "u" + ctyp$
        typ2ctyp$ = ctyp$: EXIT FUNCTION
    END IF
    IF ts$ = "%&" THEN
        typ2ctyp$ = "ptrszint": IF (t AND ISUNSIGNED) THEN typ2ctyp$ = "uptrszint"
        EXIT FUNCTION
    END IF
    IF ts$ = "%%" THEN ctyp$ = "int8"
    IF ts$ = "%" THEN ctyp$ = "int16"
    IF ts$ = "&" THEN ctyp$ = "int32"
    IF ts$ = "&&" THEN ctyp$ = "int64"
    IF ctyp$ <> "" THEN
        IF unsgn THEN ctyp$ = "u" + ctyp$
        typ2ctyp$ = ctyp$: EXIT FUNCTION
    END IF
    'is tstr$ a named type? (eg. 'LONG')
    s$ = type2symbol$(tstr$)
    IF Error_Happened THEN EXIT FUNCTION
    IF LEN(s$) THEN
        typ2ctyp$ = typ2ctyp$(0, s$)
        IF Error_Happened THEN EXIT FUNCTION
        EXIT FUNCTION
    END IF

    Give_Error "Invalid type": EXIT FUNCTION

END FUNCTION

FUNCTION type2symbol$ (typ$)
    t$ = typ$
    FOR i = 1 TO LEN(t$)
        IF MID$(t$, i, 1) = sp THEN MID$(t$, i, 1) = " "
    NEXT
    e$ = "Cannot convert type (" + typ$ + ") to symbol"

    t2$ = "SINGLE": s$ = "!": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "DOUBLE": s$ = "#": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_FLOAT": s$ = "##": IF t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED _BYTE": s$ = "~%%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_BYTE": s$ = "%%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED INTEGER": s$ = "~%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "INTEGER": s$ = "%": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED LONG": s$ = "~&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "LONG": s$ = "&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED _INTEGER64": s$ = "~&&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_INTEGER64": s$ = "&&": IF t$ = t2$ THEN GOTO t2sfound

    t2$ = "_UNSIGNED _OFFSET": s$ = "~%&": IF t$ = t2$ THEN GOTO t2sfound
    t2$ = "_OFFSET": s$ = "%&": IF t$ = t2$ THEN GOTO t2sfound

    ' These can have a length after them, so LEFT$() is used
    t2$ = "STRING": s$ = "$": IF LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "_UNSIGNED _BIT": s$ = "~`1": IF LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound
    t2$ = "_BIT": s$ = "`1": IF LEFT$(t$, LEN(t2$)) = t2$ THEN GOTO t2sfound

    Give_Error e$: EXIT FUNCTION
    t2sfound:
    type2symbol$ = s$
    IF LEN(t2$) <> LEN(t$) THEN
        IF s$ <> "$" AND s$ <> "~`1" AND s$ <> "`1" THEN Give_Error e$: EXIT FUNCTION
        t$ = RIGHT$(t$, LEN(t$) - LEN(t2$))
        IF LEFT$(t$, 3) <> " * " THEN Give_Error e$: EXIT FUNCTION
        t$ = RIGHT$(t$, LEN(t$) - 3)
        IF isuinteger(t$) = 0 THEN Give_Error e$: EXIT FUNCTION
        v = VAL(t$)
        IF v = 0 THEN Give_Error e$: EXIT FUNCTION
        IF s$ <> "$" AND v > 64 THEN Give_Error e$: EXIT FUNCTION
        IF s$ = "$" THEN
            s$ = s$ + _TOSTR$(v)
        ELSE
            s$ = LEFT$(s$, LEN(s$) - 1) + _TOSTR$(v)
        END IF
        type2symbol$ = s$
    END IF
END FUNCTION

'Strips away bits/indentifiers which make locating a variables source difficult
FUNCTION typecomp (typ)
    typ2 = typ
    IF (typ2 AND ISINCONVENTIONALMEMORY) THEN typ2 = typ2 - ISINCONVENTIONALMEMORY
    typecomp = typ2
END FUNCTION

FUNCTION typname2typ& (t2$)
    typname2typsize = 0 'the default

    t$ = t2$

    'symbol?
    ts$ = t$
    IF ts$ = "$" THEN typname2typ& = STRINGTYPE: EXIT FUNCTION
    IF ts$ = "!" THEN typname2typ& = SINGLETYPE: EXIT FUNCTION
    IF ts$ = "#" THEN typname2typ& = DOUBLETYPE: EXIT FUNCTION
    IF ts$ = "##" THEN typname2typ& = FLOATTYPE: EXIT FUNCTION

    'fixed length string?
    IF LEFT$(ts$, 1) = "$" THEN
        n$ = RIGHT$(ts$, LEN(ts$) - 1)
        IF isuinteger(n$) = 0 THEN Give_Error "Invalid index after STRING * type": EXIT FUNCTION
        b = VAL(n$)
        IF b = 0 THEN Give_Error "Invalid index after STRING * type": EXIT FUNCTION
        typname2typsize = b
        typname2typ& = STRINGTYPE + ISFIXEDLENGTH
        EXIT FUNCTION
    END IF

    'unsigned?
    IF LEFT$(ts$, 1) = "~" THEN unsgn = 1: ts$ = RIGHT$(ts$, LEN(ts$) - 1)

    'bit-type?
    IF LEFT$(ts$, 1) = "`" THEN
        n$ = RIGHT$(ts$, LEN(ts$) - 1)
        b = 1
        IF n$ <> "" THEN
            IF isuinteger(n$) = 0 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
            b = VAL(n$)
            IF b > 64 THEN Give_Error "Invalid index after _BIT type": EXIT FUNCTION
        END IF
        IF unsgn THEN typname2typ& = UBITTYPE + (b - 1) ELSE typname2typ& = BITTYPE + (b - 1)
        EXIT FUNCTION
    END IF

    t = 0
    IF ts$ = "%%" THEN t = BYTETYPE
    IF ts$ = "%" THEN t = INTEGERTYPE
    IF ts$ = "&" THEN t = LONGTYPE
    IF ts$ = "&&" THEN t = INTEGER64TYPE
    IF ts$ = "%&" THEN t = OFFSETTYPE

    IF t THEN
        IF unsgn THEN t = t + ISUNSIGNED
        typname2typ& = t: EXIT FUNCTION
    END IF
    'not a valid symbol

    'type name?
    FOR i = 1 TO LEN(t$)
        IF MID$(t$, i, 1) = sp THEN MID$(t$, i, 1) = " "
    NEXT
    IF t$ = "STRING" THEN typname2typ& = STRINGTYPE: EXIT FUNCTION

    IF LEFT$(t$, 9) = "STRING * " THEN

        n$ = RIGHT$(t$, LEN(t$) - 9)

        'constant check 2011
        hashfound = 0
        hashname$ = n$
        hashchkflags = HASHFLAG_CONSTANT
        hashres = HashFindRev(hashname$, hashchkflags, hashresflags, hashresref)
        DO WHILE hashres
            IF constsubfunc(hashresref) = subfuncn OR constsubfunc(hashresref) = 0 THEN
                IF constdefined(hashresref) THEN
                    hashfound = 1
                    EXIT DO
                END IF
            END IF
            IF hashres <> 1 THEN hashres = HashFindCont(hashresflags, hashresref) ELSE hashres = 0
        LOOP
        IF hashfound THEN
            i2 = hashresref
            t = consttype(i2)
            IF t AND ISSTRING THEN Give_Error "Expected STRING * numeric-constant": EXIT FUNCTION
            'convert value to general formats
            IF t AND ISFLOAT THEN
                v## = constfloat(i2)
                v&& = v##
                v~&& = v&&
            ELSE
                IF t AND ISUNSIGNED THEN
                    v~&& = constuinteger(i2)
                    v&& = v~&&
                    v## = v&&
                ELSE
                    v&& = constinteger(i2)
                    v## = v&&
                    v~&& = v&&
                END IF
            END IF
            IF v&& < 1 OR v&& > 9999999999 THEN Give_Error "STRING * out-of-range constant": EXIT FUNCTION
            b = v&&
            GOTO constantlenstr
        END IF

        IF isuinteger(n$) = 0 OR LEN(n$) > 10 THEN Give_Error "Invalid number/constant after STRING * type": EXIT FUNCTION
        b = VAL(n$)
        IF b = 0 OR LEN(n$) > 10 THEN Give_Error "Invalid number after STRING * type": EXIT FUNCTION
        constantlenstr:
        typname2typsize = b
        typname2typ& = STRINGTYPE + ISFIXEDLENGTH
        EXIT FUNCTION
    END IF

    IF t$ = "SINGLE" THEN typname2typ& = SINGLETYPE: EXIT FUNCTION
    IF t$ = "DOUBLE" THEN typname2typ& = DOUBLETYPE: EXIT FUNCTION
    IF t$ = "_FLOAT" THEN typname2typ& = FLOATTYPE: EXIT FUNCTION
    IF LEFT$(t$, 10) = "_UNSIGNED " THEN
        u = 1
        t$ = MID$(t$, INSTR(t$, CHR$(32)) + 1)
    END IF
    IF LEFT$(t$, 4) = "_BIT" THEN
        IF t$ = "_BIT" THEN
            IF u THEN typname2typ& = UBITTYPE ELSE typname2typ& = BITTYPE
            EXIT FUNCTION
        END IF

        IF LEFT$(t$, 7) <> "_BIT * " THEN Give_Error "Expected _BIT * number": EXIT FUNCTION

        IF LEFT$(t$, 4) = "_BIT" THEN
            n$ = RIGHT$(t$, LEN(t$) - 7)
        ELSE
            n$ = RIGHT$(t$, LEN(t$) - 6)
        END IF

        IF isuinteger(n$) = 0 THEN Give_Error "Invalid size after _BIT *": EXIT FUNCTION
        b = VAL(n$)
        IF b = 0 OR b > 64 THEN Give_Error "Invalid size after _BIT *": EXIT FUNCTION
        t = BITTYPE - 1 + b: IF u THEN t = t + ISUNSIGNED
        typname2typ& = t
        EXIT FUNCTION
    END IF

    t = 0
    IF t$ = "_BYTE" THEN t = BYTETYPE
    IF t$ = "INTEGER" THEN t = INTEGERTYPE
    IF t$ = "LONG" THEN t = LONGTYPE
    IF t$ = "_INTEGER64" THEN t = INTEGER64TYPE
    IF t$ = "_OFFSET" THEN t = OFFSETTYPE
    IF t THEN
        IF u THEN t = t + ISUNSIGNED
        typname2typ& = t
        EXIT FUNCTION
    END IF
    IF u THEN EXIT FUNCTION '_UNSIGNED (nothing)

    'UDT?
    FOR i = 1 TO lasttype
        IF t$ = RTRIM$(udtxname(i)) THEN
            typname2typ& = ISUDT + ISPOINTER + i
            EXIT FUNCTION
        END IF
    NEXT

    'return 0 (failed)
END FUNCTION

FUNCTION removesymbol$ (varname$)
    i = INSTR(varname$, "~"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "`"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "%"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "&"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "!"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "#"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "$"): IF i THEN GOTO foundsymbol
    EXIT FUNCTION
    foundsymbol:
    IF i = 1 THEN Give_Error "Expected variable name before symbol": EXIT FUNCTION
    symbol$ = RIGHT$(varname$, LEN(varname$) - i + 1)
    IF symboltype(symbol$) = 0 THEN Give_Error "Invalid symbol": EXIT FUNCTION
    removesymbol$ = symbol$
    varname$ = LEFT$(varname$, i - 1)
END FUNCTION

'
' Does not report an error if the symbol is invalid or varname is blank
'
FUNCTION tryRemoveSymbol$ (varname$)
    i = INSTR(varname$, "~"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "`"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "%"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "&"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "!"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "#"): IF i THEN GOTO foundsymbol
    i = INSTR(varname$, "$"): IF i THEN GOTO foundsymbol
    EXIT FUNCTION
    foundsymbol:
    symbol$ = RIGHT$(varname$, LEN(varname$) - i + 1)
    IF symboltype(symbol$) = 0 THEN EXIT FUNCTION
    tryRemoveSymbol$ = symbol$
    varname$ = LEFT$(varname$, i - 1)
END FUNCTION

SUB increaseUDTArrays
    ' The compiler keeps legacy inline size/offset metadata and, when the TYPE graph
    ' contains an explicit _Dynamic member array, one canonical descriptor-layout table.
    ' That choice is derived from TYPE declarations; DIM, REDIM and call sites never change it.
    x = UBOUND(udtxname)
    REDIM _PRESERVE udtxname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtxcname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtxsize(x + 1000) AS LONG
    REDIM _PRESERVE udtxfdynsize(x + 1000) AS LONG
    REDIM _PRESERVE udtxcanonmode(x + 1000) AS INTEGER
    REDIM _PRESERVE udtxnext(x + 1000) AS LONG
    REDIM _PRESERVE udtxvariable(x + 1000) AS INTEGER 'true if the udt contains variable length elements
    'elements
    REDIM _PRESERVE udtename(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtecname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtesize(x + 1000) AS LONG
    REDIM _PRESERVE udtefdynoffset(x + 1000) AS LONG
    REDIM _PRESERVE udtefdynsize(x + 1000) AS LONG
    REDIM _PRESERVE udtetype(x + 1000) AS LONG
    REDIM _PRESERVE udtetypesize(x + 1000) AS LONG
    REDIM _PRESERVE udtearrayelements(x + 1000) AS LONG
    REDIM _PRESERVE udtearraybase(x + 1000) AS LONG
    REDIM _PRESERVE udtearraydims(x + 1000) AS LONG
    ' udtearraydesc stores declaration-time bounds for every TYPE member array as
    ' serialized numeric lower/count pairs. For _Dynamic members these are the initial bounds.
    REDIM _PRESERVE udtearraydesc(x + 1000) AS STRING
    ' udtearrayfieldmode: 0 = unmarked inline member array,
    ' 1 = explicit _Static inline member array, 2 = explicit _Dynamic descriptor-backed member array.
    ' Unmarked compile-time arrays remain inline for compatibility, regardless of whether
    ' the parent UDT scalar/array is declared with DIM or REDIM.
    REDIM _PRESERVE udtearrayfieldmode(x + 1000) AS LONG
    REDIM _PRESERVE udtenext(x + 1000) AS LONG
END SUB

' Build the canonical descriptor-aware layout for a TYPE graph containing explicit _Dynamic arrays.
' The legacy inline layout remains in udtxsize/udtesize and is not modified here.
SUB BuildUDTDynLayout (udt_index AS LONG)
    BuildUDTDynLayoutM udt_index, 1

    udtxcanonmode(udt_index) = 0
    IF UDTDynHasMemberArrays%(udt_index, 1) THEN udtxcanonmode(udt_index) = 1
END SUB

SUB BuildUDTDynLayoutM (udt_index AS LONG, layout_mode AS LONG)
    ' Each explicit _Dynamic member occupies one pointer-sized descriptor slot in the
    ' parent UDT layout. Unmarked and _Static members remain inline. If an inline nested
    ' UDT graph contains explicit _Dynamic members, its canonical descriptor-layout size
    ' is used recursively so every stored offset and stride remains valid.
    DIM member_id AS LONG
    DIM nested_udt AS LONG
    DIM dynbits AS LONG
    DIM ptrbits AS LONG
    DIM membersize AS LONG
    DIM memberalign AS LONG
    DIM layoutalign AS LONG

    member_id = udtxnext(udt_index)
    dynbits = 0
    ptrbits = OFFSETTYPE AND UDTMASK
    layoutalign = 8

    DO WHILE member_id
        memberalign = 8
        IF UDTMemberDynDesc%(member_id) THEN
            memberalign = ptrbits
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN memberalign = ptrbits
        END IF

        ' Descriptor slots are dereferenced as ptrszint** in generated C. Align
        ' their offsets, and the start of nested descriptor-layout UDT values, so those
        ' accesses are valid C++ rather than architecture-dependent unaligned
        ' pointer dereferences. dynbits and ptrbits are measured in bits.
        IF memberalign > 8 THEN
            IF dynbits MOD memberalign THEN dynbits = dynbits + memberalign - (dynbits MOD memberalign)
            layoutalign = ptrbits
        END IF

        udtefdynoffset(member_id) = dynbits

        IF UDTMemberDynDesc%(member_id) THEN
            ' Descriptor-backed members occupy only a pointer-sized slot in the
            ' parent layout. Descriptor headers and payloads are allocated later
            ' by the initialization helpers.
            membersize = ptrbits
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    membersize = udtearrayelements(member_id) * UDTDynLayoutSize&(nested_udt)
                ELSE
                    membersize = udtesize(member_id)
                END IF
            ELSE
                membersize = udtesize(member_id)
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                membersize = UDTDynLayoutSize&(nested_udt)
            ELSE
                membersize = udtesize(member_id)
            END IF
        ELSE
            membersize = udtesize(member_id)
        END IF

        udtefdynsize(member_id) = membersize

        dynbits = dynbits + membersize
        member_id = udtenext(member_id)
    LOOP

    ' Keep every element of a descriptor-layout UDT array correctly aligned. It is
    ' not enough to align the first descriptor offset: without tail padding the
    ' next UDT element could begin between pointer boundaries.
    IF layoutalign > 8 THEN
        IF dynbits MOD layoutalign THEN dynbits = dynbits + layoutalign - (dynbits MOD layoutalign)
    END IF

    udtxfdynsize(udt_index) = dynbits
END SUB

' Return true when this UDT graph contains at least one explicit _Dynamic member array.
' Descriptor init/free/clone paths use this as their recursion gate. This result alone does
' not prove that every non-descriptor member is safe to copy with raw memcpy.
FUNCTION UDTDynHasMemberArrays% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG

    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id) THEN
            UDTDynHasMemberArrays% = -1
            EXIT FUNCTION
        END IF
        IF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                UDTDynHasMemberArrays% = -1
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' Validate the subset of member-array/owner combinations supported by the
' descriptor implementation. This keeps parser/runtime behavior predictable:
' unsupported ownership graphs are rejected before any C code is generated.
FUNCTION UDTDynMembersOK% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG
    member_id = udtxnext(udt_index)
    UDTDynMembersOK% = -1

    DO WHILE member_id
        IF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISSTRING) <> 0 THEN
                IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                    ' A variable-length STRING member array owns qbs* slots. Explicit _Dynamic
                    ' stores those slots behind a live descriptor; unmarked and _Static members
                    ' stay inline. The storage form is never inferred from DIM, REDIM or a call site.
                    IF UDTMemberDynDesc%(member_id) = 0 THEN
                        ' Inline variable-length STRING arrays in owner layouts are managed as
                        ' qbs* slots; the owner lifecycle helpers initialize and free each
                        ' element instead of treating the member as one scalar string.
                        IF UDTDynOwnerOK%(udt_index, layout_mode) = 0 THEN
                            UDTDynMembersOK% = 0
                            EXIT FUNCTION
                        END IF
                        IF Error_Happened THEN
                            UDTDynMembersOK% = 0
                            EXIT FUNCTION
                        END IF
                    END IF
                END IF
            END IF
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                ' An explicit _Dynamic UDT member array whose element owns variable strings
                ' needs recursive owner helpers. An unmarked or _Static member array remains
                ' inline; only its nested element layout becomes descriptor-aware when required.
                IF UDTMemberDynDesc%(member_id) AND udtxvariable(nested_udt) THEN
                    IF UDTDynOwnerOK%(nested_udt, layout_mode) = 0 THEN
                        UDTDynMembersOK% = 0
                        EXIT FUNCTION
                    END IF
                ELSE
                    IF UDTDynMembersOK%(nested_udt, layout_mode) = 0 THEN
                        UDTDynMembersOK% = 0
                        EXIT FUNCTION
                    END IF
                END IF
            END IF
        ' A scalar variable-length STRING in a descriptor-layout TYPE graph requires the
        ' owner-aware lifecycle path so its qbs* slot is initialized, copied and freed safely.
        ELSEIF (udtetype(member_id) AND ISSTRING) <> 0 THEN
            IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                IF UDTDynOwnerOK%(udt_index, layout_mode) = 0 THEN UDTDynMembersOK% = 0
                EXIT FUNCTION
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            IF UDTDynMembersOK%(udtetype(member_id) AND UDTMASK, layout_mode) = 0 THEN
                UDTDynMembersOK% = 0
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' True for descriptor-backed _Dynamic AS STRING member arrays. The payload
' is an array of qbs* slots, not raw character bytes; callers use this to select
' qbs init/free/copy and to keep _MEM/PUT/GET paths rejected elsewhere.
FUNCTION DynMemVarStr% (member_id AS LONG)
    IF member_id <= 0 THEN EXIT FUNCTION
    IF udtearrayelements(member_id) = 0 THEN EXIT FUNCTION
    IF udtearrayfieldmode(member_id) <> 2 THEN EXIT FUNCTION
    IF (udtetype(member_id) AND ISSTRING) = 0 THEN EXIT FUNCTION
    IF (udtetype(member_id) AND ISFIXEDLENGTH) <> 0 THEN EXIT FUNCTION
    DynMemVarStr% = -1
END FUNCTION

' True for descriptor-backed _Dynamic AS UDT where the element UDT owns
' variable-length strings. Such payloads require owner-aware recursive handling
' instead of memcpy, even though the descriptor header itself is fixed-size.
FUNCTION DynMemUDTVarStr% (member_id AS LONG)
    DIM nested_udt AS LONG
    IF member_id <= 0 THEN EXIT FUNCTION
    IF udtearrayelements(member_id) = 0 THEN EXIT FUNCTION
    IF udtearrayfieldmode(member_id) <> 2 THEN EXIT FUNCTION
    IF (udtetype(member_id) AND ISUDT) = 0 THEN EXIT FUNCTION
    nested_udt = udtetype(member_id) AND UDTMASK
    IF udtxvariable(nested_udt) = 0 THEN EXIT FUNCTION
    DynMemUDTVarStr% = -1
END FUNCTION

' Return true if this layout contains any descriptor payload that owns variable
' string storage directly or through nested UDT elements. This is a safety gate
' for operations that cannot expose or serialize qbs*/descriptor ownership.
FUNCTION UDTDynHasVarDesc% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG

    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id) THEN
            seen_dyn = -1
            IF DynMemVarStr%(member_id) THEN
                UDTDynHasVarDesc% = -1
                EXIT FUNCTION
            END IF
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                IF udtxvariable(nested_udt) THEN
                    UDTDynHasVarDesc% = -1
                    EXIT FUNCTION
                END IF
                IF UDTDynHasVarDesc%(nested_udt, layout_mode) THEN
                    UDTDynHasVarDesc% = -1
                    EXIT FUNCTION
                END IF
            END IF
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF UDTDynHasVarDesc%(nested_udt, layout_mode) THEN
                UDTDynHasVarDesc% = -1
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' Validate a descriptor-layout UDT graph that owns qbs* strings and/or member-array
' descriptors. Its lifecycle must initialize, free and deep-copy owned fields instead
' of treating the complete UDT value as an opaque byte block.
FUNCTION UDTDynOwnerOK% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG
    DIM seen_dyn AS LONG

    UDTDynOwnerOK% = -1
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id) THEN
            seen_dyn = -1
            ' Every explicit _Dynamic member needs descriptor lifecycle handling. STRING
            ' payloads and owner-UDT payloads additionally need deep qbs*/nested-owner handling;
            ' _MEM and PUT/GET remain rejected when variable-string ownership is present.
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                ' A descriptor-backed _Dynamic AS UDT element may itself be an owner layout.
                ' The recursive lifecycle helpers initialize, free and copy scalar variable
                ' STRINGs, _Dynamic AS STRING payloads and nested descriptor-owned members.
                IF UDTDynOwnerOK%(nested_udt, layout_mode) = 0 THEN
                    UDTDynOwnerOK% = 0
                    EXIT FUNCTION
                END IF
            END IF
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISSTRING) <> 0 THEN
                IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                    'Inline fixed-bound variable-length STRING arrays are owner-managed
                    'qbs* slots. They are not fixed-length strings, but they are now
                    'initialized/freed/copied slot-by-slot in the owner helpers.
                END IF
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            ' Scalar variable strings are allowed before, after, or between descriptor-backed
            ' member arrays. Unsupported descriptor-owning member types are still rejected
            ' when their own _Dynamic member is visited.
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF UDTDynOwnerOK%(nested_udt, layout_mode) = 0 THEN
                UDTDynOwnerOK% = 0
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION


' Descriptor payload helper for _Dynamic AS STRING. The payload stores qbs*
' slots at elem_bytes stride, so each element needs qbs_new/qbs_free/qbs_set
' rather than raw byte initialization or memcpy.
SUB AppendDynStrInit (base_expr AS STRING, total_expr AS STRING, elem_bytes AS LONG, acc AS STRING)
    DIM cr AS STRING
    cr = CHR$(13) + CHR$(10)
    acc = acc + cr + "for(ptrszint dyn_udt_str_i=0; dyn_udt_str_i<(" + total_expr + "); dyn_udt_str_i++){"
    acc = acc + cr + "*(qbs**)(((uint8*)(" + base_expr + "))+dyn_udt_str_i*" + LTRIM$(STR$(elem_bytes)) + ")=qbs_new(0,0);"
    acc = acc + cr + "}"
END SUB

SUB AppendDynStrFree (base_expr AS STRING, total_expr AS STRING, elem_bytes AS LONG, acc AS STRING)
    DIM cr AS STRING
    cr = CHR$(13) + CHR$(10)
    acc = acc + cr + "for(ptrszint dyn_udt_str_i=0; dyn_udt_str_i<(" + total_expr + "); dyn_udt_str_i++){"
    acc = acc + cr + "qbs **dyn_udt_str_slot=(qbs**)(((uint8*)(" + base_expr + "))+dyn_udt_str_i*" + LTRIM$(STR$(elem_bytes)) + ");"
    acc = acc + cr + "if (*dyn_udt_str_slot) qbs_free(*dyn_udt_str_slot);"
    acc = acc + cr + "}"
END SUB

SUB AppendDynStrSet (dst_expr AS STRING, src_expr AS STRING, total_expr AS STRING, elem_bytes AS LONG, acc AS STRING)
    DIM cr AS STRING
    cr = CHR$(13) + CHR$(10)
    acc = acc + cr + "for(ptrszint dyn_udt_str_i=0; dyn_udt_str_i<(" + total_expr + "); dyn_udt_str_i++){"
    acc = acc + cr + "qbs *dyn_udt_str_src=*(qbs**)(((uint8*)(" + src_expr + "))+dyn_udt_str_i*" + LTRIM$(STR$(elem_bytes)) + ");"
    acc = acc + cr + "qbs **dyn_udt_str_dst_slot=(qbs**)(((uint8*)(" + dst_expr + "))+dyn_udt_str_i*" + LTRIM$(STR$(elem_bytes)) + ");"
    acc = acc + cr + "if (!*dyn_udt_str_dst_slot) *dyn_udt_str_dst_slot=qbs_new(0,0);"
    acc = acc + cr + "if (dyn_udt_str_src) qbs_set(*dyn_udt_str_dst_slot,dyn_udt_str_src);"
    acc = acc + cr + "}"
END SUB


' Generate C code to initialize one owner-layout UDT element. Keep this established
' entry point because qb64pe.bas and other TYPE helpers call it directly. The shared
' lifecycle walker below owns the traversal; mode 1 performs initialization.
SUB AppendDynUDTOwnInitAt (base_expr AS STRING, udt_index AS LONG, root_offset AS LONG, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynUDTOwnLifeEx base_expr, udt_index, _TOSTR$(root_offset), elem_bytes, index_expr, acc, layout_mode, 0, 1
END SUB

' Generate C code to release one owner-layout UDT element while preserving the existing
' public helper signature. Mode 2 follows the exact same ownership graph as initialization.
SUB AppendDynUDTOwnFreeAt (base_expr AS STRING, udt_index AS LONG, root_offset AS LONG, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynUDTOwnLifeEx base_expr, udt_index, _TOSTR$(root_offset), elem_bytes, index_expr, acc, layout_mode, 0, 2
END SUB

' Shared expression-aware owner-layout lifecycle walker.
'
' op_mode=1 initializes descriptor payloads and inline/scalar qbs* owners.
' op_mode=2 frees the same descriptor payloads and qbs* owners.
'
' Descriptor traversal is invoked only once for each containing UDT element, exactly as in
' the former separate init/free walkers. Nested owner UDTs recurse through this same walker,
' which keeps init and free structurally symmetric without changing ownership semantics.
'
' Fixed member arrays are emitted as compact C++ runtime loops. Never replace these loops
' with BASIC-time FOR expansion over udtearrayelements(): generated C++ size and executable
' size must depend on TYPE shape, not on the declared number of array elements.
SUB AppendDynUDTOwnLifeEx (base_expr AS STRING, udt_index AS LONG, root_expr AS STRING, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG, op_mode AS LONG)
    DIM member_id AS LONG
    DIM elem_step AS LONG
    DIM nested_udt AS LONG
    DIM desc_done AS LONG
    DIM cr AS STRING
    DIM member_expr AS STRING
    DIM item_expr AS STRING
    DIM loop_name AS STRING
    DIM count_text AS STRING
    DIM stride_text AS STRING
    DIM slot_expr AS STRING

    cr = CHR$(13) + CHR$(10)
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_expr = "(" + root_expr + "+" + _TOSTR$(UDTDynMemberOffset&(member_id) \ 8) + ")"

        IF UDTMemberDynDesc%(member_id) THEN
            IF desc_done = 0 THEN
                SELECT CASE op_mode
                    CASE 1
                        AppendDynUDTDescInitEx base_expr, udt_index, root_expr, elem_bytes, index_expr, acc, layout_mode, loop_depth
                    CASE 2
                        AppendDynUDTDescFreeEx base_expr, udt_index, root_expr, elem_bytes, index_expr, acc, layout_mode, loop_depth
                END SELECT
                IF Error_Happened THEN EXIT SUB
                desc_done = -1
            END IF

        ELSEIF udtearrayelements(member_id) THEN
            elem_step = UDTDynInlineElemBytes&(member_id, layout_mode)
            count_text = _TOSTR$(udtearrayelements(member_id))
            stride_text = _TOSTR$(elem_step)

            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                elem_step = udt_array_member_bytes(member_id)
                stride_text = _TOSTR$(elem_step)
                loop_name = "dyn_own_i" + _TOSTR$(loop_depth)
                acc = acc + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                slot_expr = "*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+(" + item_expr + "))"

                SELECT CASE op_mode
                    CASE 1
                        acc = acc + cr + slot_expr + "=qbs_new(0,0);"
                    CASE 2
                        acc = acc + cr + "qbs_free(" + slot_expr + ");"
                END SELECT

                acc = acc + cr + "}"

            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    loop_name = "dyn_own_i" + _TOSTR$(loop_depth)
                    acc = acc + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                    item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                    AppendDynUDTOwnLifeEx base_expr, nested_udt, item_expr, elem_bytes, index_expr, acc, layout_mode, loop_depth + 1, op_mode
                    IF Error_Happened THEN EXIT SUB
                    acc = acc + cr + "}"
                END IF
            END IF

        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            slot_expr = "*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+(" + member_expr + "))"
            SELECT CASE op_mode
                CASE 1
                    acc = acc + cr + slot_expr + "=qbs_new(0,0);"
                CASE 2
                    acc = acc + cr + "qbs_free(" + slot_expr + ");"
            END SELECT

        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                AppendDynUDTOwnLifeEx base_expr, nested_udt, member_expr, elem_bytes, index_expr, acc, layout_mode, loop_depth, op_mode
                IF Error_Happened THEN EXIT SUB
            END IF
        END IF

        member_id = udtenext(member_id)
    LOOP
END SUB

' Generate C code for assignment into an already-live owner-layout UDT element.
' Existing destination descriptors are erased before cloning from source; qbs*
' scalar/inline members use qbs_set, and nested owner UDTs recurse.
SUB AppendDynUDTOwnSetAt (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root AS LONG, src_root AS LONG, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynUDTOwnSetEx dst_expr, src_expr, udt_index, _TOSTR$(dst_root), _TOSTR$(src_root), elem_bytes, dst_index, src_index, acc, layout_mode, 0
END SUB

' Expression-aware assignment into an already-live owner-layout UDT element.
SUB AppendDynUDTOwnSetEx (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root_expr AS STRING, src_root_expr AS STRING, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG)
    AppendDynUDTOwnCopyEx dst_expr, src_expr, udt_index, dst_root_expr, src_root_expr, elem_bytes, dst_index, src_index, acc, layout_mode, loop_depth, 1
END SUB

' Clone an owner-layout UDT element into zero-filled destination storage.
' Unlike AppendDynUDTOwnSetAt, this helper must not erase destination members first.
' It is used by whole-array assignment replace-copy: the new parent payload is
' calloc/zeroed, then each source element is deep-cloned into the clean slot.
' This avoids the unsafe init -> erase -> clone pattern for nested owner graphs.
SUB AppendDynUDTOwnCloneAt (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root AS LONG, src_root AS LONG, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynUDTOwnCloneEx dst_expr, src_expr, udt_index, _TOSTR$(dst_root), _TOSTR$(src_root), elem_bytes, dst_index, src_index, acc, layout_mode, 0
END SUB

' Expression-aware clone into zero-filled owner storage. Unlike SetEx this does not
' erase destination descriptors first.
SUB AppendDynUDTOwnCloneEx (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root_expr AS STRING, src_root_expr AS STRING, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG)
    AppendDynUDTOwnCopyEx dst_expr, src_expr, udt_index, dst_root_expr, src_root_expr, elem_bytes, dst_index, src_index, acc, layout_mode, loop_depth, 2
END SUB

' Shared owner-layout deep-copy traversal.
' copy_mode=1 assigns into an already-live destination and therefore erases each
' descriptor-backed member before cloning it. copy_mode=2 clones into zero-filled
' destination storage and must not erase first. Scalar/inline qbs members use qbs_set
' in both modes. Fixed member arrays always emit compact C++ runtime loops.
SUB AppendDynUDTOwnCopyEx (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root_expr AS STRING, src_root_expr AS STRING, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG, copy_mode AS LONG)
    DIM member_id AS LONG
    DIM elem_step AS LONG
    DIM nested_udt AS LONG
    DIM member_elem_bytes AS LONG
    DIM dst_slot AS STRING
    DIM src_slot AS STRING
    DIM cr AS STRING
    DIM dst_member_expr AS STRING
    DIM src_member_expr AS STRING
    DIM dst_item_expr AS STRING
    DIM src_item_expr AS STRING
    DIM loop_name AS STRING
    DIM loop_prefix AS STRING
    DIM count_text AS STRING
    DIM stride_text AS STRING

    cr = CHR$(13) + CHR$(10)
    IF copy_mode = 1 THEN loop_prefix = "dyn_ownset_i" ELSE loop_prefix = "dyn_clone_i"
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        dst_member_expr = "(" + dst_root_expr + "+" + _TOSTR$(UDTDynMemberOffset&(member_id) \ 8) + ")"
        src_member_expr = "(" + src_root_expr + "+" + _TOSTR$(UDTDynMemberOffset&(member_id) \ 8) + ")"
        IF UDTMemberDynDesc%(member_id) THEN
            member_elem_bytes = udt_dyn_array_elem_bytes(member_id)
            nested_udt = 0
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN nested_udt = udtetype(member_id) AND UDTMASK
            dst_slot = "((uint8*)" + dst_expr + "+" + elem_bytes + "*(" + dst_index + ")+(" + dst_member_expr + "))"
            src_slot = "((uint8*)" + src_expr + "+" + elem_bytes + "*(" + src_index + ")+(" + src_member_expr + "))"
            IF copy_mode = 1 THEN
                AppendDynMemberEraseEx dst_slot, member_elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
                IF Error_Happened THEN EXIT SUB
            END IF
            AppendDynMemberCloneAfterRaw src_slot, dst_slot, member_elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        ELSEIF udtearrayelements(member_id) THEN
            elem_step = udt_array_member_bytes(member_id)
            count_text = _TOSTR$(udtearrayelements(member_id))
            stride_text = _TOSTR$(elem_step)
            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                loop_name = loop_prefix + _TOSTR$(loop_depth)
                acc = acc + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                dst_item_expr = "(" + dst_member_expr + "+" + loop_name + "*" + stride_text + ")"
                src_item_expr = "(" + src_member_expr + "+" + loop_name + "*" + stride_text + ")"
                acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+(" + dst_item_expr + ")); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+(" + src_item_expr + ")); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}"
                acc = acc + cr + "}"
            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                loop_name = loop_prefix + _TOSTR$(loop_depth)
                acc = acc + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                dst_item_expr = "(" + dst_member_expr + "+" + loop_name + "*" + stride_text + ")"
                src_item_expr = "(" + src_member_expr + "+" + loop_name + "*" + stride_text + ")"
                IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    AppendDynUDTOwnCopyEx dst_expr, src_expr, nested_udt, dst_item_expr, src_item_expr, elem_bytes, dst_index, src_index, acc, layout_mode, loop_depth + 1, copy_mode
                    IF Error_Happened THEN EXIT SUB
                ELSE
                    acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+(" + dst_item_expr + "),((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+(" + src_item_expr + "),(size_t)" + stride_text + ");"
                END IF
                acc = acc + cr + "}"
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+(" + dst_member_expr + "),((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+(" + src_member_expr + "),(size_t)" + _TOSTR$(udtesize(member_id) \ 8) + ");"
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+(" + dst_member_expr + ")); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+(" + src_member_expr + ")); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}"
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                AppendDynUDTOwnCopyEx dst_expr, src_expr, nested_udt, dst_member_expr, src_member_expr, elem_bytes, dst_index, src_index, acc, layout_mode, loop_depth, copy_mode
                IF Error_Happened THEN EXIT SUB
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+(" + dst_member_expr + "),((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+(" + src_member_expr + "),(size_t)" + _TOSTR$(udtesize(member_id) \ 8) + ");"
            END IF
        ELSE
            acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+(" + dst_member_expr + "),((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+(" + src_member_expr + "),(size_t)" + _TOSTR$(udtesize(member_id) \ 8) + ");"
        END IF
        member_id = udtenext(member_id)
    LOOP
END SUB

' Entry point used by qb64pe.bas when an array of UDT elements needs descriptor
' initialization. Variable-string owner layouts go through the owner initializer;
' pure descriptor layouts can initialize descriptor slots directly.
SUB AppendDynUDTDescInit (n$, udt_index AS LONG, root_offset AS LONG, bytesperelement$, acc$, layout_mode AS LONG)
    IF udtxvariable(udt_index) THEN
        AppendDynUDTOwnInitAt "(void*)" + n$ + "[0]", udt_index, root_offset, bytesperelement$, "tmp_long", acc$, layout_mode
    ELSE
        AppendDynUDTDescInitAt n$ + "[0]", udt_index, root_offset, bytesperelement$, "tmp_long", acc$, layout_mode
    END IF
END SUB

' Generate initial C descriptor headers and payload allocations for explicit _Dynamic
' member arrays. Each new descriptor starts with the declaration-time dimension metadata
' and a mem lock.
SUB AppendDynUDTDescInitAt (data_expr AS STRING, udt_index AS LONG, root_offset AS LONG, bytesperelement AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynUDTDescInitEx data_expr, udt_index, _TOSTR$(root_offset), bytesperelement, index_expr, acc, layout_mode, 0
END SUB

' Expression-aware descriptor initializer. Inline arrays of nested descriptor-layout
' UDTs are traversed by generated runtime loops instead of compile-time unrolling.
SUB AppendDynUDTDescInitEx (data_expr AS STRING, udt_index AS LONG, root_expr AS STRING, bytesperelement AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG)
    AppendDynUDTDescLifeEx data_expr, udt_index, root_expr, bytesperelement, index_expr, acc, layout_mode, loop_depth, 1
END SUB

' Shared descriptor lifecycle traversal. Mode 1 creates explicit _Dynamic member
' descriptors; mode 2 releases them. Inline nested UDT arrays are traversed by one
' generated runtime loop regardless of their declared element count.
SUB AppendDynUDTDescLifeEx (data_expr AS STRING, udt_index AS LONG, root_expr AS STRING, bytesperelement AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG, op_mode AS LONG)
    DIM member_id AS LONG
    DIM elem_bytes AS LONG
    DIM dims_count AS LONG
    DIM desc_slots AS LONG
    DIM count_total AS LONG
    DIM desc_pos AS LONG
    DIM dim_idx AS LONG
    DIM lower_val AS LONG
    DIM count_val AS LONG
    DIM stride_val AS LONG
    DIM desc_idx AS LONG
    DIM member_ptr AS STRING
    DIM member_slot AS STRING
    DIM desc_var AS STRING
    DIM member_desc AS STRING
    DIM total_expr AS STRING
    DIM nested_udt AS LONG
    DIM elem_idx AS STRING
    DIM inline_bytes AS LONG
    DIM cr AS STRING
    DIM member_expr AS STRING
    DIM item_expr AS STRING
    DIM loop_name AS STRING
    DIM count_text AS STRING
    DIM stride_text AS STRING

    cr = CHR$(13) + CHR$(10)
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_expr = "(" + root_expr + "+" + _TOSTR$(UDTDynMemberOffset&(member_id) \ 8) + ")"

        IF UDTMemberDynDesc%(member_id) THEN
            elem_bytes = udt_dyn_array_elem_bytes(member_id)
            nested_udt = 0
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN nested_udt = udtetype(member_id) AND UDTMASK

            IF op_mode = 1 THEN
                dims_count = udtearraydims(member_id)
                desc_slots = 4 * dims_count + 4 + 1
                count_total = 1
                desc_pos = 1
                stride_val = 1
                member_desc = udtearraydesc(member_id)
                member_ptr = "((ptrszint**)((uint8*)(" + data_expr + ")+" + bytesperelement + "*(" + index_expr + ")+(" + member_expr + ")))"
                desc_var = "dyn_udt_desc"

                acc = acc + cr + "{"
                acc = acc + cr + "ptrszint **dyn_udt_slot=" + member_ptr + ";"
                acc = acc + cr + "ptrszint *dyn_udt_desc=(ptrszint*)calloc((size_t)" + _TOSTR$(desc_slots) + ",ptrsz);"
                acc = acc + cr + "if (!dyn_udt_desc) error(257);"
                acc = acc + cr + "*dyn_udt_slot=dyn_udt_desc;"
                acc = acc + cr + "new_mem_lock();"
                acc = acc + cr + "mem_lock_tmp->type=4;"
                acc = acc + cr + desc_var + "[" + _TOSTR$(desc_slots - 1) + "]=(ptrszint)mem_lock_tmp;"
                acc = acc + cr + desc_var + "[1]=0;"
                acc = acc + cr + desc_var + "[2]=1;"
                acc = acc + cr + desc_var + "[3]=" + _TOSTR$(dims_count) + ";"

                FOR dim_idx = 1 TO dims_count
                    IF ParseNextUDTArrayDescriptorDim(member_desc, desc_pos, lower_val, count_val) = 0 THEN
                        Give_Error "Cannot process the declared bounds of this TYPE member array"
                        EXIT SUB
                    END IF
                    desc_idx = (dims_count - dim_idx) * 4 + 4
                    acc = acc + cr + desc_var + "[" + _TOSTR$(desc_idx) + "]=" + _TOSTR$(lower_val) + ";"
                    acc = acc + cr + desc_var + "[" + _TOSTR$(desc_idx + 1) + "]=" + _TOSTR$(count_val) + ";"
                    acc = acc + cr + desc_var + "[" + _TOSTR$(desc_idx + 2) + "]=" + _TOSTR$(stride_val) + ";"
                    acc = acc + cr + desc_var + "[" + _TOSTR$(desc_idx + 3) + "]=0;"
                    stride_val = stride_val * count_val
                    count_total = count_total * count_val
                NEXT

                IF count_total <> udtearrayelements(member_id) THEN
                    Give_Error "Cannot process the declared bounds of this TYPE member array"
                    EXIT SUB
                END IF

                acc = acc + cr + desc_var + "[0]=(ptrszint)calloc((size_t)(" + _TOSTR$(count_total) + "*" + _TOSTR$(elem_bytes) + "),1);"
                acc = acc + cr + "if (!" + desc_var + "[0]) error(257);"
                total_expr = _TOSTR$(count_total)

                IF DynMemVarStr%(member_id) THEN
                    AppendDynStrInit "(void*)" + desc_var + "[0]", total_expr, elem_bytes, acc
                END IF

                IF nested_udt <> 0 THEN
                    IF DynMemUDTVarStr%(member_id) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                        elem_idx = "dyn_udt_own_m" + _TOSTR$(member_id)
                        acc = acc + cr + "for(ptrszint " + elem_idx + "=0;" + elem_idx + "<" + total_expr + ";" + elem_idx + "++){"
                        AppendDynUDTOwnInitAt "(void*)" + desc_var + "[0]", nested_udt, 0, _TOSTR$(elem_bytes), elem_idx, acc, layout_mode
                        IF Error_Happened THEN EXIT SUB
                        acc = acc + cr + "}"
                    END IF
                END IF

                acc = acc + cr + "}"
            ELSE
                member_slot = "((uint8*)(" + data_expr + ")+" + bytesperelement + "*(" + index_expr + ")+(" + member_expr + "))"
                AppendDynMemberEraseEx member_slot, elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
                IF Error_Happened THEN EXIT SUB
            END IF
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    inline_bytes = UDTDynInlineElemBytes&(member_id, layout_mode)
                    count_text = _TOSTR$(udtearrayelements(member_id))
                    stride_text = _TOSTR$(inline_bytes)
                    loop_name = "dyn_desc_i" + _TOSTR$(loop_depth)
                    acc = acc + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                    item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                    AppendDynUDTDescLifeEx data_expr, nested_udt, item_expr, bytesperelement, index_expr, acc, layout_mode, loop_depth + 1, op_mode
                    IF Error_Happened THEN EXIT SUB
                    acc = acc + cr + "}"
                END IF
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            AppendDynUDTDescLifeEx data_expr, udtetype(member_id) AND UDTMASK, member_expr, bytesperelement, index_expr, acc, layout_mode, loop_depth, op_mode
            IF Error_Happened THEN EXIT SUB
        END IF

        member_id = udtenext(member_id)
    LOOP
END SUB

' Entry point for freeing descriptor-backed member arrays inside UDT storage.
' Owner layouts need recursive owner cleanup; pure descriptor layouts can walk
' descriptor slots directly.
SUB AppendDynUDTDescFree (base_ptr AS STRING, udt_index AS LONG, root_offset AS LONG, bytesperelement AS STRING, acc AS STRING, layout_mode AS LONG)
    IF udtxvariable(udt_index) THEN
        AppendDynUDTOwnFreeAt "(void*)" + base_ptr, udt_index, root_offset, bytesperelement, "tmp_long", acc, layout_mode
    ELSE
        AppendDynUDTDescFreeAt base_ptr, udt_index, root_offset, bytesperelement, "tmp_long", acc, layout_mode
    END IF
END SUB

' Generate C code that frees descriptor payloads reachable from one UDT element,
' including nested inline UDT arrays whose element layout contains descriptors.
SUB AppendDynUDTDescFreeAt (data_expr AS STRING, udt_index AS LONG, root_offset AS LONG, bytesperelement AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynUDTDescFreeEx data_expr, udt_index, _TOSTR$(root_offset), bytesperelement, index_expr, acc, layout_mode, 0
END SUB

' Expression-aware descriptor cleanup mirror of AppendDynUDTDescInitEx.
SUB AppendDynUDTDescFreeEx (data_expr AS STRING, udt_index AS LONG, root_expr AS STRING, bytesperelement AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG, loop_depth AS LONG)
    AppendDynUDTDescLifeEx data_expr, udt_index, root_expr, bytesperelement, index_expr, acc, layout_mode, loop_depth, 2
END SUB

SUB AppendDynMemberErase (desc_slot AS STRING, acc AS STRING, layout_mode AS LONG)
    AppendDynMemberEraseEx desc_slot, 0, 0, 0, acc, layout_mode
END SUB

SUB AppendDynMemberEraseTyped (desc_slot AS STRING, elem_bytes AS LONG, elem_udt AS LONG, elem_varstr AS LONG, acc AS STRING, layout_mode AS LONG)
    AppendDynMemberEraseEx desc_slot, elem_bytes, elem_udt, elem_varstr, acc, layout_mode
END SUB

' Generate C code to erase one descriptor-backed member array. The helper first
' releases element-owned qbs*/nested descriptors when needed, then frees payload,
' mem lock and descriptor header, and finally clears the parent slot.
SUB AppendDynMemberEraseEx (desc_slot AS STRING, elem_bytes AS LONG, elem_udt AS LONG, elem_varstr AS LONG, acc AS STRING, layout_mode AS LONG)
    DIM cr AS STRING
    DIM elem_idx AS STRING

    cr = CHR$(13) + CHR$(10)
    elem_idx = "dyn_udt_erase_i"

    acc = acc + cr + "{"
    acc = acc + cr + "ptrszint **dyn_udt_slot=(ptrszint**)(" + desc_slot + ");"
    acc = acc + cr + "ptrszint *dyn_udt_old=*dyn_udt_slot;"
    ' After the new descriptor has been populated, release the old descriptor and
    ' any element-owned payload that was not transferred. The parent slot is not
    ' updated until the cleanup path has been emitted.
    acc = acc + cr + "if (dyn_udt_old){"
    IF elem_varstr THEN
        acc = acc + cr + "if (dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue && dyn_udt_old[3]>0){"
        acc = acc + cr + "ptrszint dyn_udt_erase_total=1;"
        acc = acc + cr + "for(ptrszint dyn_udt_erase_dim=0;dyn_udt_erase_dim<dyn_udt_old[3];dyn_udt_erase_dim++){dyn_udt_erase_total*=dyn_udt_old[4+dyn_udt_erase_dim*4+1];}"
        AppendDynStrFree "(void*)dyn_udt_old[0]", "dyn_udt_erase_total", elem_bytes, acc
        acc = acc + cr + "}"
    ELSEIF elem_udt <> 0 THEN
        IF udtxvariable(elem_udt) OR UDTDynHasMemberArrays%(elem_udt, layout_mode) THEN
            acc = acc + cr + "if (dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue && dyn_udt_old[3]>0){"
            acc = acc + cr + "ptrszint dyn_udt_erase_total=1;"
            acc = acc + cr + "for(ptrszint dyn_udt_erase_dim=0;dyn_udt_erase_dim<dyn_udt_old[3];dyn_udt_erase_dim++){dyn_udt_erase_total*=dyn_udt_old[4+dyn_udt_erase_dim*4+1];}"
            acc = acc + cr + "for(ptrszint " + elem_idx + "=0;" + elem_idx + "<dyn_udt_erase_total;" + elem_idx + "++){"
            AppendDynUDTOwnFreeAt "(void*)dyn_udt_old[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), elem_idx, acc, layout_mode
            IF Error_Happened THEN EXIT SUB
            acc = acc + cr + "}"
            acc = acc + cr + "}"
        END IF
    END IF
    acc = acc + cr + "if (dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue) free((void*)dyn_udt_old[0]);"
    acc = acc + cr + "if (dyn_udt_old[3]>0){ptrszint dyn_udt_lock_index=4*dyn_udt_old[3]+4; if (dyn_udt_old[dyn_udt_lock_index]) free_mem_lock((mem_lock*)dyn_udt_old[dyn_udt_lock_index]);}"
    acc = acc + cr + "free((void*)dyn_udt_old);"
    acc = acc + cr + "*dyn_udt_slot=NULL;"
    acc = acc + cr + "}"
    acc = acc + cr + "}"
END SUB

' Generate C code to deep-clone one descriptor-backed member array from an
' already-existing source descriptor into a clean destination slot. The generated
' symbol prefix includes element size/type tags so multiple clones can appear in
' the same emitted C scope without name collisions.
SUB AppendDynMemberCloneAfterRaw (src_slot AS STRING, dst_slot AS STRING, elem_bytes AS LONG, elem_udt AS LONG, elem_varstr AS LONG, acc AS STRING, layout_mode AS LONG)
    DIM cr AS STRING
    DIM pfx AS STRING
    DIM udt_tag AS STRING
    DIM var_tag AS STRING

    cr = CHR$(13) + CHR$(10)
    IF elem_udt < 0 THEN
        udt_tag = "m" + LTRIM$(STR$(-elem_udt))
    ELSE
        udt_tag = LTRIM$(STR$(elem_udt))
    END IF
    IF elem_varstr < 0 THEN
        var_tag = "m" + LTRIM$(STR$(-elem_varstr))
    ELSE
        var_tag = LTRIM$(STR$(elem_varstr))
    END IF
    pfx = "dyn_udt_c" + LTRIM$(STR$(elem_bytes)) + "_" + udt_tag + "_" + var_tag + "_"

    acc = acc + cr + "{"
    acc = acc + cr + "ptrszint *" + pfx + "src=*((ptrszint**)(" + src_slot + "));"
    acc = acc + cr + "ptrszint **" + pfx + "dst_slot=(ptrszint**)(" + dst_slot + ");"
    acc = acc + cr + "*" + pfx + "dst_slot=NULL;"
    acc = acc + cr + "if (" + pfx + "src && " + pfx + "src[3]>0){"
    acc = acc + cr + "ptrszint " + pfx + "dims=" + pfx + "src[3];"
    acc = acc + cr + "ptrszint " + pfx + "slots=4*" + pfx + "dims+4+1;"
    acc = acc + cr + "ptrszint *" + pfx + "dst=(ptrszint*)calloc((size_t)" + pfx + "slots,ptrsz);"
    acc = acc + cr + "if (!" + pfx + "dst) error(257);"
    acc = acc + cr + "memcpy((void*)" + pfx + "dst,(void*)" + pfx + "src,(size_t)((" + pfx + "slots-1)*ptrsz));"
    acc = acc + cr + "new_mem_lock();"
    acc = acc + cr + "mem_lock_tmp->type=4;"
    acc = acc + cr + pfx + "dst[" + pfx + "slots-1]=(ptrszint)mem_lock_tmp;"
    acc = acc + cr + "ptrszint " + pfx + "total=1;"
    acc = acc + cr + "for(ptrszint " + pfx + "dim=0;" + pfx + "dim<" + pfx + "dims;" + pfx + "dim++){"
    acc = acc + cr + pfx + "total*=" + pfx + "src[4+" + pfx + "dim*4+1];"
    acc = acc + cr + "}"
    acc = acc + cr + pfx + "dst[0]=(ptrszint)calloc((size_t)(" + pfx + "total*" + LTRIM$(STR$(elem_bytes)) + "),1);"
    acc = acc + cr + "if (!" + pfx + "dst[0]) error(257);"
    IF elem_varstr THEN
        AppendDynStrInit "(void*)" + pfx + "dst[0]", pfx + "total", elem_bytes, acc
        acc = acc + cr + "if (" + pfx + "src[0] && " + pfx + "src[0]!=(ptrszint)nothingvalue && " + pfx + "total){"
        AppendDynStrSet "(void*)" + pfx + "dst[0]", "(void*)" + pfx + "src[0]", pfx + "total", elem_bytes, acc
        acc = acc + cr + "}"
    ELSEIF elem_udt <> 0 AND (udtxvariable(elem_udt) OR UDTDynHasMemberArrays%(elem_udt, layout_mode)) THEN
        ' Clone into zero-filled owner elements. The descriptor payload was allocated with
        ' calloc above, so every destination element is clean storage. Use CloneAt rather
        ' than SetAt here: Clone preserves that contract and avoids generating erase blocks
        ' for descriptors that cannot yet be live. Scalar qbs* slots are still created lazily.
        acc = acc + cr + "if (" + pfx + "src[0] && " + pfx + "src[0]!=(ptrszint)nothingvalue && " + pfx + "total){"
        acc = acc + cr + "for(ptrszint " + pfx + "own_i=0; " + pfx + "own_i<" + pfx + "total; " + pfx + "own_i++){"
        AppendDynUDTOwnCloneAt "(void*)" + pfx + "dst[0]", "(void*)" + pfx + "src[0]", elem_udt, 0, 0, LTRIM$(STR$(elem_bytes)), pfx + "own_i", pfx + "own_i", acc, layout_mode
        IF Error_Happened THEN EXIT SUB
        acc = acc + cr + "}"
        acc = acc + cr + "}else{"
        acc = acc + cr + "for(ptrszint " + pfx + "own_i=0; " + pfx + "own_i<" + pfx + "total; " + pfx + "own_i++){"
        AppendDynUDTOwnInitAt "(void*)" + pfx + "dst[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), pfx + "own_i", acc, layout_mode
        IF Error_Happened THEN EXIT SUB
        acc = acc + cr + "}"
        acc = acc + cr + "}"
    ELSE
        acc = acc + cr + "if (" + pfx + "src[0] && " + pfx + "src[0]!=(ptrszint)nothingvalue && " + pfx + "total) memcpy((void*)" + pfx + "dst[0],(void*)" + pfx + "src[0],(size_t)(" + pfx + "total*" + LTRIM$(STR$(elem_bytes)) + "));"
    END IF
    acc = acc + cr + "*" + pfx + "dst_slot=" + pfx + "dst;"
    acc = acc + cr + "}"
    acc = acc + cr + "}"
END SUB


SUB AppendDynMemberRedim (prep_prefix AS STRING, total_dims AS LONG, desc_slot AS STRING, elem_bytes AS LONG, elem_udt AS LONG, elem_varstr AS LONG, redim_kind AS LONG, acc AS STRING, layout_mode AS LONG)
    ' redim_kind follows the ordinary REDIM parser: 1 = REDIM,
    ' 2 = REDIM _PRESERVE, 3 = REDIM _RETAIN. The member-array syntax is the
    ' same as ordinary dynamic arrays; only the storage target is a descriptor
    ' slot inside an owning UDT element.
    'The bounds were evaluated in qb64pe.bas by BuildDynMemberBoundsPrep(),
    'using the same expression path as ordinary dynamic arrays. This routine only
    'consumes the prepared descriptor and performs allocation/preserve/retain.
    DIM cr AS STRING
    DIM desc_name AS STRING
    DIM desc_slots AS LONG
    DIM desc_index AS LONG
    DIM dim_index AS LONG
    DIM retain_dim AS LONG

    cr = CHR$(13) + CHR$(10)
    IF total_dims <= 0 THEN Give_Error "Array bounds missing": EXIT SUB

    desc_name = prep_prefix + "_desc"
    desc_slots = 4 * total_dims + 4 + 1

    acc = acc + cr + "{"
    acc = acc + cr + "ptrszint **dyn_udt_slot=(ptrszint**)(" + desc_slot + ");"
    acc = acc + cr + "ptrszint *dyn_udt_old=*dyn_udt_slot;"
    acc = acc + cr + "ptrszint *dyn_udt_new=(ptrszint*)calloc((size_t)" + LTRIM$(STR$(desc_slots)) + ",ptrsz);"
    acc = acc + cr + "if (!dyn_udt_new) error(257);"
    acc = acc + cr + "new_mem_lock();"
    acc = acc + cr + "mem_lock_tmp->type=4;"
    acc = acc + cr + "dyn_udt_new[" + LTRIM$(STR$(desc_slots - 1)) + "]=(ptrszint)mem_lock_tmp;"
    acc = acc + cr + "dyn_udt_new[1]=0;"
    acc = acc + cr + "dyn_udt_new[2]=1;"
    acc = acc + cr + "dyn_udt_new[3]=" + LTRIM$(STR$(total_dims)) + ";"
    acc = acc + cr + "ptrszint dyn_udt_total=1;"

    FOR dim_index = 1 TO total_dims
        desc_index = (total_dims - dim_index) * 4 + 4
        acc = acc + cr + "dyn_udt_new[" + LTRIM$(STR$(desc_index)) + "]=" + desc_name + "[" + LTRIM$(STR$(desc_index)) + "];"
        acc = acc + cr + "dyn_udt_new[" + LTRIM$(STR$(desc_index + 1)) + "]=" + desc_name + "[" + LTRIM$(STR$(desc_index + 1)) + "];"
        acc = acc + cr + "dyn_udt_new[" + LTRIM$(STR$(desc_index + 2)) + "]=" + desc_name + "[" + LTRIM$(STR$(desc_index + 2)) + "];"
        acc = acc + cr + "dyn_udt_new[" + LTRIM$(STR$(desc_index + 3)) + "]=0;"
        acc = acc + cr + "dyn_udt_total*=dyn_udt_new[" + LTRIM$(STR$(desc_index + 1)) + "];"
    NEXT

    ' Allocate the new payload before copying from the old descriptor. Element-owned
    ' strings or nested owner UDTs are initialized immediately so later qbs_set or
    ' owner assignment writes into valid destination slots.
    acc = acc + cr + "dyn_udt_new[0]=(ptrszint)calloc((size_t)(dyn_udt_total*" + LTRIM$(STR$(elem_bytes)) + "),1);"
    acc = acc + cr + "if (!dyn_udt_new[0]) error(257);"

    IF elem_varstr THEN
        AppendDynStrInit "(void*)dyn_udt_new[0]", "dyn_udt_total", elem_bytes, acc
    ELSEIF elem_udt <> 0 AND (udtxvariable(elem_udt) OR UDTDynHasMemberArrays%(elem_udt, layout_mode)) THEN
        IF UDTDynHasMemberArrays%(elem_udt, layout_mode) THEN acc = acc + cr + "unsigned char *dyn_udt_moved=NULL;"
        acc = acc + cr + "for(ptrszint dyn_udt_own_init_i=0; dyn_udt_own_init_i<dyn_udt_total; dyn_udt_own_init_i++){"
        AppendDynUDTOwnInitAt "(void*)dyn_udt_new[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_own_init_i", acc, layout_mode
        IF Error_Happened THEN EXIT SUB
        acc = acc + cr + "}"
    END IF

    ' REDIM _PRESERVE keeps the leading linear prefix, matching ordinary QB64PE
    ' dynamic-array preserve semantics. It does not preserve by coordinates.
    IF redim_kind = 2 THEN
        acc = acc + cr + "if (dyn_udt_old && dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue && dyn_udt_old[3]>0){"
        acc = acc + cr + "ptrszint dyn_udt_old_total=1;"
        acc = acc + cr + "for(ptrszint dyn_udt_dim=0;dyn_udt_dim<dyn_udt_old[3];dyn_udt_dim++){"
        acc = acc + cr + "dyn_udt_old_total*=dyn_udt_old[4+dyn_udt_dim*4+1];"
        acc = acc + cr + "}"
        acc = acc + cr + "ptrszint dyn_udt_copy_total=(dyn_udt_old_total<dyn_udt_total)?dyn_udt_old_total:dyn_udt_total;"
        IF elem_udt <> 0 AND (udtxvariable(elem_udt) OR UDTDynHasMemberArrays%(elem_udt, layout_mode)) THEN
            acc = acc + cr + "for(ptrszint dyn_udt_copy_i=0; dyn_udt_copy_i<dyn_udt_copy_total; dyn_udt_copy_i++){"
            AppendDynUDTOwnSetAt "(void*)dyn_udt_new[0]", "(void*)dyn_udt_old[0]", elem_udt, 0, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_copy_i", "dyn_udt_copy_i", acc, layout_mode
            IF Error_Happened THEN EXIT SUB
            acc = acc + cr + "}"
        ELSEIF elem_varstr THEN
            acc = acc + cr + "if (dyn_udt_copy_total>0){"
            AppendDynStrSet "(void*)dyn_udt_new[0]", "(void*)dyn_udt_old[0]", "dyn_udt_copy_total", elem_bytes, acc
            acc = acc + cr + "}"
        ELSEIF elem_udt <> 0 AND UDTDynHasMemberArrays%(elem_udt, layout_mode) THEN
            acc = acc + cr + "if (dyn_udt_old_total>0){dyn_udt_moved=(unsigned char*)calloc((size_t)dyn_udt_old_total,1); if(!dyn_udt_moved) error(257);}"
            acc = acc + cr + "for(ptrszint dyn_udt_copy_i=0; dyn_udt_copy_i<dyn_udt_copy_total; dyn_udt_copy_i++){"
            AppendDynUDTDescFreeAt "(void*)dyn_udt_new[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_copy_i", acc, layout_mode
            IF Error_Happened THEN EXIT SUB
            acc = acc + cr + "memcpy(((uint8*)dyn_udt_new[0])+dyn_udt_copy_i*" + LTRIM$(STR$(elem_bytes)) + ",((uint8*)dyn_udt_old[0])+dyn_udt_copy_i*" + LTRIM$(STR$(elem_bytes)) + ",(size_t)" + LTRIM$(STR$(elem_bytes)) + ");"
            acc = acc + cr + "dyn_udt_moved[dyn_udt_copy_i]=1;"
            acc = acc + cr + "}"
        ELSE
            acc = acc + cr + "if (dyn_udt_copy_total>0) memcpy((void*)dyn_udt_new[0],(void*)dyn_udt_old[0],(size_t)(dyn_udt_copy_total*" + LTRIM$(STR$(elem_bytes)) + "));"
        END IF
        acc = acc + cr + "}"
    ' REDIM _RETAIN preserves the coordinate intersection of old and new bounds.
    ' This is the important path for multidimensional descriptor member arrays.
    ELSEIF redim_kind = 3 THEN
        acc = acc + cr + "if (dyn_udt_old && dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue && dyn_udt_old[3]==" + LTRIM$(STR$(total_dims)) + "){"
        IF elem_udt <> 0 AND UDTDynHasMemberArrays%(elem_udt, layout_mode) THEN
            acc = acc + cr + "ptrszint dyn_udt_old_total=1;"
            acc = acc + cr + "for(ptrszint dyn_udt_old_dim=0;dyn_udt_old_dim<dyn_udt_old[3];dyn_udt_old_dim++){dyn_udt_old_total*=dyn_udt_old[4+dyn_udt_old_dim*4+1];}"
            acc = acc + cr + "if (dyn_udt_old_total>0){dyn_udt_moved=(unsigned char*)calloc((size_t)dyn_udt_old_total,1); if(!dyn_udt_moved) error(257);}"
        END IF
        acc = acc + cr + "ptrszint dyn_udt_retain_lo[" + LTRIM$(STR$(total_dims)) + "];"
        acc = acc + cr + "ptrszint dyn_udt_retain_hi[" + LTRIM$(STR$(total_dims)) + "];"
        acc = acc + cr + "ptrszint dyn_udt_retain_idx[" + LTRIM$(STR$(total_dims)) + "];"
        acc = acc + cr + "ptrszint dyn_udt_retain_old_off;"
        acc = acc + cr + "ptrszint dyn_udt_retain_new_off;"
        acc = acc + cr + "ptrszint dyn_udt_retain_dim;"
        acc = acc + cr + "int dyn_udt_retain_any=1;"
        FOR retain_dim = 1 TO total_dims
            desc_index = (total_dims - retain_dim) * 4 + 4
            acc = acc + cr + "dyn_udt_retain_lo[" + LTRIM$(STR$(retain_dim - 1)) + "]=dyn_udt_new[" + LTRIM$(STR$(desc_index)) + "];"
            acc = acc + cr + "if (dyn_udt_old[" + LTRIM$(STR$(desc_index)) + "]>dyn_udt_retain_lo[" + LTRIM$(STR$(retain_dim - 1)) + "]) dyn_udt_retain_lo[" + LTRIM$(STR$(retain_dim - 1)) + "]=dyn_udt_old[" + LTRIM$(STR$(desc_index)) + "];"
            acc = acc + cr + "dyn_udt_retain_hi[" + LTRIM$(STR$(retain_dim - 1)) + "]=dyn_udt_new[" + LTRIM$(STR$(desc_index)) + "]+(dyn_udt_new[" + LTRIM$(STR$(desc_index + 1)) + "]-1);"
            acc = acc + cr + "if ((dyn_udt_old[" + LTRIM$(STR$(desc_index)) + "]+(dyn_udt_old[" + LTRIM$(STR$(desc_index + 1)) + "]-1))<dyn_udt_retain_hi[" + LTRIM$(STR$(retain_dim - 1)) + "]) dyn_udt_retain_hi[" + LTRIM$(STR$(retain_dim - 1)) + "]=dyn_udt_old[" + LTRIM$(STR$(desc_index)) + "]+(dyn_udt_old[" + LTRIM$(STR$(desc_index + 1)) + "]-1);"
            acc = acc + cr + "if (dyn_udt_retain_hi[" + LTRIM$(STR$(retain_dim - 1)) + "]<dyn_udt_retain_lo[" + LTRIM$(STR$(retain_dim - 1)) + "]) dyn_udt_retain_any=0;"
        NEXT
        acc = acc + cr + "if (dyn_udt_retain_any){"
        FOR retain_dim = 1 TO total_dims
            acc = acc + cr + "dyn_udt_retain_idx[" + LTRIM$(STR$(retain_dim - 1)) + "]=dyn_udt_retain_lo[" + LTRIM$(STR$(retain_dim - 1)) + "];"
        NEXT
        acc = acc + cr + "for(;;){"
        desc_index = (total_dims - 1) * 4 + 4
        acc = acc + cr + "dyn_udt_retain_old_off=dyn_udt_retain_idx[0]-dyn_udt_old[" + LTRIM$(STR$(desc_index)) + "];"
        acc = acc + cr + "dyn_udt_retain_new_off=dyn_udt_retain_idx[0]-dyn_udt_new[" + LTRIM$(STR$(desc_index)) + "];"
        FOR retain_dim = 2 TO total_dims
            desc_index = (total_dims - retain_dim) * 4 + 4
            acc = acc + cr + "dyn_udt_retain_old_off+=(dyn_udt_retain_idx[" + LTRIM$(STR$(retain_dim - 1)) + "]-dyn_udt_old[" + LTRIM$(STR$(desc_index)) + "])*dyn_udt_old[" + LTRIM$(STR$(desc_index + 2)) + "];"
            acc = acc + cr + "dyn_udt_retain_new_off+=(dyn_udt_retain_idx[" + LTRIM$(STR$(retain_dim - 1)) + "]-dyn_udt_new[" + LTRIM$(STR$(desc_index)) + "])*dyn_udt_new[" + LTRIM$(STR$(desc_index + 2)) + "];"
        NEXT
        IF elem_udt <> 0 AND (udtxvariable(elem_udt) OR UDTDynHasMemberArrays%(elem_udt, layout_mode)) THEN
            AppendDynUDTOwnSetAt "(void*)dyn_udt_new[0]", "(void*)dyn_udt_old[0]", elem_udt, 0, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_retain_new_off", "dyn_udt_retain_old_off", acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        ELSEIF elem_varstr THEN
            acc = acc + cr + "qbs_set(*(qbs**)(((uint8*)dyn_udt_new[0])+dyn_udt_retain_new_off*" + LTRIM$(STR$(elem_bytes)) + "),*(qbs**)(((uint8*)dyn_udt_old[0])+dyn_udt_retain_old_off*" + LTRIM$(STR$(elem_bytes)) + "));"
        ELSEIF elem_udt <> 0 AND UDTDynHasMemberArrays%(elem_udt, layout_mode) THEN
            AppendDynUDTDescFreeAt "(void*)dyn_udt_new[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_retain_new_off", acc, layout_mode
            IF Error_Happened THEN EXIT SUB
            acc = acc + cr + "memcpy(((uint8*)dyn_udt_new[0])+dyn_udt_retain_new_off*" + LTRIM$(STR$(elem_bytes)) + ",((uint8*)dyn_udt_old[0])+dyn_udt_retain_old_off*" + LTRIM$(STR$(elem_bytes)) + ",(size_t)" + LTRIM$(STR$(elem_bytes)) + ");"
            acc = acc + cr + "dyn_udt_moved[dyn_udt_retain_old_off]=1;"
        ELSE
            acc = acc + cr + "memcpy(((uint8*)dyn_udt_new[0])+dyn_udt_retain_new_off*" + LTRIM$(STR$(elem_bytes)) + ",((uint8*)dyn_udt_old[0])+dyn_udt_retain_old_off*" + LTRIM$(STR$(elem_bytes)) + ",(size_t)" + LTRIM$(STR$(elem_bytes)) + ");"
        END IF
        acc = acc + cr + "dyn_udt_retain_dim=0;"
        acc = acc + cr + "while (dyn_udt_retain_dim<" + LTRIM$(STR$(total_dims)) + "){"
        acc = acc + cr + "dyn_udt_retain_idx[dyn_udt_retain_dim]++;"
        acc = acc + cr + "if (dyn_udt_retain_idx[dyn_udt_retain_dim]<=dyn_udt_retain_hi[dyn_udt_retain_dim]) break;"
        acc = acc + cr + "dyn_udt_retain_idx[dyn_udt_retain_dim]=dyn_udt_retain_lo[dyn_udt_retain_dim];"
        acc = acc + cr + "dyn_udt_retain_dim++;"
        acc = acc + cr + "}"
        acc = acc + cr + "if (dyn_udt_retain_dim==" + LTRIM$(STR$(total_dims)) + ") break;"
        acc = acc + cr + "}"
        acc = acc + cr + "}"
        acc = acc + cr + "}"
    END IF

    acc = acc + cr + "if (dyn_udt_old){"
    IF elem_varstr THEN
        acc = acc + cr + "if (dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue && dyn_udt_old[3]>0){"
        acc = acc + cr + "ptrszint dyn_udt_old_free_total=1;"
        acc = acc + cr + "for(ptrszint dyn_udt_old_free_dim=0;dyn_udt_old_free_dim<dyn_udt_old[3];dyn_udt_old_free_dim++){dyn_udt_old_free_total*=dyn_udt_old[4+dyn_udt_old_free_dim*4+1];}"
        AppendDynStrFree "(void*)dyn_udt_old[0]", "dyn_udt_old_free_total", elem_bytes, acc
        acc = acc + cr + "}"
    ELSEIF elem_udt <> 0 AND (udtxvariable(elem_udt) OR UDTDynHasMemberArrays%(elem_udt, layout_mode)) THEN
        acc = acc + cr + "if (dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue && dyn_udt_old[3]>0){"
        acc = acc + cr + "ptrszint dyn_udt_old_free_total=1;"
        acc = acc + cr + "for(ptrszint dyn_udt_old_free_dim=0;dyn_udt_old_free_dim<dyn_udt_old[3];dyn_udt_old_free_dim++){dyn_udt_old_free_total*=dyn_udt_old[4+dyn_udt_old_free_dim*4+1];}"
        acc = acc + cr + "for(ptrszint dyn_udt_old_free_i=0;dyn_udt_old_free_i<dyn_udt_old_free_total;dyn_udt_old_free_i++){"
        AppendDynUDTOwnFreeAt "(void*)dyn_udt_old[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_old_free_i", acc, layout_mode
        IF Error_Happened THEN EXIT SUB
        acc = acc + cr + "}"
        acc = acc + cr + "}"
        IF UDTDynHasMemberArrays%(elem_udt, layout_mode) THEN acc = acc + cr + "if (dyn_udt_moved) free((void*)dyn_udt_moved);"
    END IF
    acc = acc + cr + "if (dyn_udt_old[0] && dyn_udt_old[0]!=(ptrszint)nothingvalue) free((void*)dyn_udt_old[0]);"
    acc = acc + cr + "if (dyn_udt_old[3]>0){ptrszint dyn_udt_lock_index=4*dyn_udt_old[3]+4; if (dyn_udt_old[dyn_udt_lock_index]) free_mem_lock((mem_lock*)dyn_udt_old[dyn_udt_lock_index]);}"
    acc = acc + cr + "free((void*)dyn_udt_old);"
    acc = acc + cr + "}"
    acc = acc + cr + "*dyn_udt_slot=dyn_udt_new;"
    acc = acc + cr + "}"
END SUB

' Element size for descriptor payloads. UDT elements use their canonical descriptor-layout
' size when the nested TYPE graph contains explicit _Dynamic members; udtesize retains
' the declaration-time inline size and is not the live descriptor payload stride.
FUNCTION udt_dyn_array_elem_bytes& (element)
    IF udtearrayelements(element) = 0 THEN EXIT FUNCTION

    ' Ordinary QB64 string arrays always store each qbs* in one uint64 slot,
    ' including 32-bit builds. Descriptor-backed _Dynamic AS STRING payloads
    ' must use the same 8-byte stride so they can be passed directly to a
    ' normal items() AS STRING parameter. A pointer-sized stride would make
    ' the parameter index the second element past the allocated payload on
    ' 32-bit builds and can crash in qbs_set/qbs_free.
    IF DynMemVarStr%(element) THEN
        udt_dyn_array_elem_bytes& = 8
        EXIT FUNCTION
    END IF

    IF (udtetype(element) AND ISUDT) <> 0 THEN
        udt_dyn_array_elem_bytes& = UDTDynLayoutSize&(udtetype(element) AND UDTMASK) \ 8
    ELSE
        udt_dyn_array_elem_bytes& = (udtesize(element) \ 8) \ udtearrayelements(element)
    END IF
END FUNCTION

' Element stride for inline (unmarked or _Static) member arrays containing nested UDTs.
' A nested inline-only TYPE keeps its legacy stride; a nested TYPE graph with explicit
' _Dynamic members uses its canonical descriptor-layout stride.
FUNCTION UDTDynInlineElemBytes& (element, layout_mode AS LONG)
    DIM nested_udt AS LONG

    IF udtearrayelements(element) = 0 THEN EXIT FUNCTION
    IF (udtetype(element) AND ISUDT) <> 0 THEN
        nested_udt = udtetype(element) AND UDTMASK
        IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
            UDTDynInlineElemBytes& = UDTDynLayoutSize&(nested_udt) \ 8
        ELSE
            UDTDynInlineElemBytes& = udt_array_member_bytes(element)
        END IF
    ELSE
        UDTDynInlineElemBytes& = udt_array_member_bytes(element)
    END IF
END FUNCTION

FUNCTION udt_array_member_bytes& (element)
    IF udtearrayelements(element) = 0 THEN EXIT FUNCTION
    udt_array_member_bytes& = (udtesize(element) \ 8) \ udtearrayelements(element)
END FUNCTION

SUB initialise_udt_varstrings (n$, udt, buf, base_offset)
    ' Keep this established entry point for every legacy inline-layout scalar UDT caller.
    ' Mode 1 initializes owned variable-STRING slots. The shared walker emits compact
    ' C++ runtime loops for fixed member arrays instead of unrolling every element here.
    AppendLegacyUDTVarOp n$, udt, buf, LTRIM$(STR$(base_offset)), 0, 1
END SUB

SUB free_udt_varstrings (n$, udt, buf, base_offset)
    ' Mode 2 mirrors initialization and frees each owned variable-STRING slot exactly once.
    AppendLegacyUDTVarOp n$, udt, buf, LTRIM$(STR$(base_offset)), 0, 2
END SUB

SUB clear_udt_with_varstrings (n$, udt, buf, base_offset)
    ' Mode 3 preserves live qbs objects, resets their lengths, and zeroes non-owning
    ' numeric/fixed-layout storage. The same walker keeps nested array code compact.
    AppendLegacyUDTVarOp n$, udt, buf, LTRIM$(STR$(base_offset)), 0, 3
END SUB

' Shared scalar adapter. The traversal itself is shared with the parent-array path below;
' keep this established internal signature so the three scalar entry points above remain untouched.
SUB AppendLegacyUDTVarOp (n$, udt, buf, root_expr$, loop_depth, op_mode)
    DIM AS STRING acc, base_addr

    acc = ""
    base_addr = "((char*)(" + n$ + "))"
    AppendLegacyInlineUDTVarOp base_addr, udt, root_expr$, acc, loop_depth, op_mode, "legacy_udt_i", 0
    IF LEN(acc) > 2 THEN WriteBufLineCpp buf, RIGHT$(acc, LEN(acc) - 2)
END SUB

SUB clear_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    AppendArrayUDTVarOp n$, udt, _TOSTR$(base_offset), bytesperelement$, acc$, 0, 3
END SUB

SUB initialise_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    AppendArrayUDTVarOp n$, udt, _TOSTR$(base_offset), bytesperelement$, acc$, 0, 1
END SUB

SUB free_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    AppendArrayUDTVarOp n$, udt, _TOSTR$(base_offset), bytesperelement$, acc$, 0, 2
END SUB

' Shared parent-array adapter. Preserve the existing accumulator contract and loop-name
' prefix while routing member traversal through the same legacy inline-layout walker.
SUB AppendArrayUDTVarOp (n$, udt, root_expr$, bytesperelement$, acc$, loop_depth, op_mode)
    DIM base_addr AS STRING

    base_addr = "((uint8*)((" + n$ + "[0]+" + bytesperelement$ + "*tmp_long)))"
    AppendLegacyInlineUDTVarOp base_addr, udt, root_expr$, acc$, loop_depth, op_mode, "arr_udt_i", -1
END SUB

' Shared legacy inline-layout walker for scalar UDT storage and one element of a parent
' UDT array. base_addr is already a byte-pointer expression, so the traversal only owns
' member offsets, qbs lifetime operations, CLEAR behavior, and compact generated loops.
'
' op_mode: 1 = initialize owned qbs*, 2 = free owned qbs*, 3 = CLEAR semantics.
' parent_array_mode preserves two historical details of the parent-array emitter: CLEAR
' recursively walks fixed-layout nested UDTs, and recursive calls propagate the existing
' Error_Happened check. The scalar path keeps its previous block-memset behavior instead.
SUB AppendLegacyInlineUDTVarOp (base_addr$, udt, root_expr$, acc$, loop_depth, op_mode, loop_prefix$, parent_array_mode)
    DIM AS LONG element, member_offset, elem_bytes, nested_udt, member_bytes
    DIM AS STRING cr, member_expr, item_expr, loop_name, count_text, stride_text, bytes_text

    IF op_mode <> 3 AND NOT udtxvariable(udt) THEN EXIT SUB

    cr = CHR$(13) + CHR$(10)
    member_offset = 0
    element = udtxnext(udt)
    DO WHILE element
        member_expr = "(" + root_expr$ + "+" + _TOSTR$(member_offset) + ")"
        member_bytes = udtesize(element) \ 8
        bytes_text = _TOSTR$(member_bytes)

        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            count_text = _TOSTR$(udtearrayelements(element))
            stride_text = _TOSTR$(elem_bytes)

            IF ((udtetype(element) AND ISSTRING) <> 0) AND ((udtetype(element) AND ISFIXEDLENGTH) = 0) THEN
                loop_name = loop_prefix$ + _TOSTR$(loop_depth)
                acc$ = acc$ + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                SELECT CASE op_mode
                    CASE 1
                        acc$ = acc$ + cr + "*(qbs**)(" + base_addr$ + "+(" + item_expr + "))=qbs_new(0,0);"
                    CASE 2
                        acc$ = acc$ + cr + "qbs_free(*(qbs**)(" + base_addr$ + "+(" + item_expr + ")));"
                    CASE 3
                        acc$ = acc$ + cr + "(*(qbs**)(" + base_addr$ + "+(" + item_expr + ")))->len=0;"
                END SELECT
                acc$ = acc$ + cr + "}"
            ELSEIF (udtetype(element) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(element) AND UDTMASK
                IF udtxvariable(nested_udt) OR (op_mode = 3 AND parent_array_mode) THEN
                    loop_name = loop_prefix$ + _TOSTR$(loop_depth)
                    acc$ = acc$ + cr + "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                    item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                    AppendLegacyInlineUDTVarOp base_addr$, nested_udt, item_expr, acc$, loop_depth + 1, op_mode, loop_prefix$, parent_array_mode
                    IF parent_array_mode AND Error_Happened THEN EXIT SUB
                    acc$ = acc$ + cr + "}"
                ELSEIF op_mode = 3 THEN
                    acc$ = acc$ + cr + "memset((void*)(" + base_addr$ + "+(" + member_expr + ")),0," + bytes_text + ");"
                END IF
            ELSEIF op_mode = 3 THEN
                acc$ = acc$ + cr + "memset((void*)(" + base_addr$ + "+(" + member_expr + ")),0," + bytes_text + ");"
            END IF
        ELSEIF ((udtetype(element) AND ISSTRING) <> 0) AND ((udtetype(element) AND ISFIXEDLENGTH) = 0) THEN
            SELECT CASE op_mode
                CASE 1
                    acc$ = acc$ + cr + "*(qbs**)(" + base_addr$ + "+(" + member_expr + "))=qbs_new(0,0);"
                CASE 2
                    acc$ = acc$ + cr + "qbs_free(*(qbs**)(" + base_addr$ + "+(" + member_expr + ")));"
                CASE 3
                    acc$ = acc$ + cr + "(*(qbs**)(" + base_addr$ + "+(" + member_expr + ")))->len=0;"
            END SELECT
        ELSEIF (udtetype(element) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(element) AND UDTMASK
            IF udtxvariable(nested_udt) OR (op_mode = 3 AND parent_array_mode) THEN
                AppendLegacyInlineUDTVarOp base_addr$, nested_udt, member_expr, acc$, loop_depth, op_mode, loop_prefix$, parent_array_mode
                IF parent_array_mode AND Error_Happened THEN EXIT SUB
            ELSEIF op_mode = 3 THEN
                acc$ = acc$ + cr + "memset((void*)(" + base_addr$ + "+(" + member_expr + ")),0," + bytes_text + ");"
            END IF
        ELSEIF op_mode = 3 THEN
            acc$ = acc$ + cr + "memset((void*)(" + base_addr$ + "+(" + member_expr + ")),0," + bytes_text + ");"
        END IF

        member_offset = member_offset + member_bytes
        element = udtenext(element)
    LOOP
END SUB

SUB copy_full_udt (dst$, src$, buf, base_offset, udt)
    CopyFullUDTExpr dst$, src$, buf, _TOSTR$(base_offset), udt, 0
END SUB

' Expression-aware legacy UDT copy. Variable-STRING member arrays are copied by a
' generated C++ loop; non-owning arrays remain one bulk memcpy.
SUB CopyFullUDTExpr (dst$, src$, buf, root_expr$, udt, loop_depth)
    DIM AS LONG element, member_offset, elem_bytes, nested_udt, member_bytes
    DIM AS STRING member_expr, item_expr, loop_name, count_text, stride_text, bytes_text

    IF NOT udtxvariable(udt) THEN
        WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + root_expr$ + ")),(" + src$ + "+(" + root_expr$ + "))," + _TOSTR$(udtxsize(udt) \ 8) + ");"
        EXIT SUB
    END IF

    member_offset = 0
    element = udtxnext(udt)
    DO WHILE element
        member_expr = "(" + root_expr$ + "+" + _TOSTR$(member_offset) + ")"
        member_bytes = udtesize(element) \ 8
        bytes_text = _TOSTR$(member_bytes)
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            count_text = _TOSTR$(udtearrayelements(element))
            stride_text = _TOSTR$(elem_bytes)
            IF ((udtetype(element) AND ISSTRING) <> 0) AND ((udtetype(element) AND ISFIXEDLENGTH) = 0) THEN
                loop_name = "copy_udt_i" + _TOSTR$(loop_depth)
                WriteBufLineCpp buf, "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                WriteBufLineCpp buf, "qbs_set(*(qbs**)(" + dst$ + "+(" + item_expr + ")),*(qbs**)(" + src$ + "+(" + item_expr + ")));"
                WriteBufLineCpp buf, "}"
            ELSEIF (udtetype(element) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(element) AND UDTMASK
                IF udtxvariable(nested_udt) THEN
                    loop_name = "copy_udt_i" + _TOSTR$(loop_depth)
                    WriteBufLineCpp buf, "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                    item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                    CopyFullUDTExpr dst$, src$, buf, item_expr, nested_udt, loop_depth + 1
                    IF Error_Happened THEN EXIT SUB
                    WriteBufLineCpp buf, "}"
                ELSE
                    WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + bytes_text + ");"
                END IF
            ELSE
                WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + bytes_text + ");"
            END IF
        ELSEIF ((udtetype(element) AND ISSTRING) <> 0) AND ((udtetype(element) AND ISFIXEDLENGTH) = 0) THEN
            WriteBufLineCpp buf, "qbs_set(*(qbs**)(" + dst$ + "+(" + member_expr + ")),*(qbs**)(" + src$ + "+(" + member_expr + ")));"
        ELSEIF (udtetype(element) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(element) AND UDTMASK
            CopyFullUDTExpr dst$, src$, buf, member_expr, nested_udt, loop_depth
            IF Error_Happened THEN EXIT SUB
        ELSE
            WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + bytes_text + ");"
        END IF
        member_offset = member_offset + member_bytes
        element = udtenext(element)
    LOOP
END SUB

' Copy one UDT value using its canonical descriptor layout. Owner graphs require a deep
' ownership-aware assignment; descriptor-only graphs copy scalar/inline fields first and
' then clone descriptor slots so source and destination do not share payload.
SUB copy_full_udt_dyn (dst$, src$, buf, base_offset, udt, layout_mode AS LONG)
    dyn_acc$ = ""

    ' Whole-value self-assignment must be a no-op. Owner Set semantics erase live
    ' destination descriptors before cloning the source, which is correct for distinct
    ' objects but would destroy the source when both addresses are identical. Keep the
    ' guard in this common descriptor-layout copy entry point so every caller gets the
    ' same protection without special parser-side identity handling.
    WriteBufLineCpp buf, "if ((void*)(" + dst$ + ")!=(void*)(" + src$ + ")){"

    IF udtxvariable(udt) THEN
        ' Owner-layout UDTs contain qbs* scalar slots and/or descriptor-owned
        ' members. Never raw-copy their scalar area first: doing so aliases qbs*
        ' owners and can leak or double-free the previous destination strings.
        ' AppendDynUDTOwnSetAt performs the full ownership-aware assignment:
        ' qbs_set for scalar strings, descriptor erase/clone for _Dynamic
        ' members, and recursive handling for nested owner UDTs.
        AppendDynUDTOwnSetAt dst$, src$, udt, base_offset, base_offset, LTRIM$(STR$(UDTDynLayoutSize&(udt) \ 8)), "0", "0", dyn_acc$, layout_mode
        IF Error_Happened THEN EXIT SUB
        IF dyn_acc$ <> "" THEN WriteBufLineCpp buf, dyn_acc$
        WriteBufLineCpp buf, "}"
        EXIT SUB
    END IF

    copy_dyn_udt_scalars dst$, src$, buf, base_offset, udt, layout_mode
    IF Error_Happened THEN EXIT SUB

    IF UDTDynHasMemberArrays%(udt, layout_mode) THEN
        AppendDynUDTDescCopy dst$, src$, udt, LTRIM$(STR$(base_offset)), LTRIM$(STR$(base_offset)), LTRIM$(STR$(UDTDynLayoutSize&(udt) \ 8)), dyn_acc$, layout_mode
        IF Error_Happened THEN EXIT SUB
        IF dyn_acc$ <> "" THEN WriteBufLineCpp buf, dyn_acc$
    END IF
    WriteBufLineCpp buf, "}"
END SUB

' Copy only the scalar/inline part of a descriptor-layout UDT. Descriptor pointer
' slots are skipped because they must be cloned, not memcpy'd, to avoid shared
' descriptors and double-free hazards.
SUB copy_dyn_udt_scalars (dst$, src$, buf, base_offset, udt, layout_mode AS LONG)
    CopyDynUDTExpr dst$, src$, buf, _TOSTR$(base_offset), udt, layout_mode, 0
END SUB

' Expression-aware scalar/inline copy for descriptor-layout UDTs. Inline arrays of
' nested owner/descriptor UDTs use generated C++ loops instead of compiler-time expansion.
SUB CopyDynUDTExpr (dst$, src$, buf, root_expr$, udt, layout_mode AS LONG, loop_depth AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG
    DIM inline_bytes AS LONG
    DIM member_bytes AS LONG
    DIM member_expr AS STRING
    DIM item_expr AS STRING
    DIM loop_name AS STRING
    DIM count_text AS STRING
    DIM stride_text AS STRING

    member_id = udtxnext(udt)
    DO WHILE member_id
        member_expr = "(" + root_expr$ + "+" + _TOSTR$(UDTDynMemberOffset&(member_id) \ 8) + ")"
        member_bytes = UDTDynMemberSize&(member_id) \ 8
        IF UDTMemberDynDesc%(member_id) THEN
            ' Descriptor slots are cloned by AppendDynUDTDescCopy().
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND UDTMASK
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    inline_bytes = UDTDynInlineElemBytes&(member_id, layout_mode)
                    count_text = _TOSTR$(udtearrayelements(member_id))
                    stride_text = _TOSTR$(inline_bytes)
                    loop_name = "copy_dyn_i" + _TOSTR$(loop_depth)
                    WriteBufLineCpp buf, "for(ptrszint " + loop_name + "=0;" + loop_name + "<" + count_text + ";" + loop_name + "++){"
                    item_expr = "(" + member_expr + "+" + loop_name + "*" + stride_text + ")"
                    ' Copy only scalar/inline bytes here. Descriptor slots are intentionally
                    ' left for the single top-level AppendDynUDTDescCopy traversal below,
                    ' which handles nested inline arrays itself. This avoids cloning the
                    ' same descriptor twice during ordinary UDT assignment.
                    CopyDynUDTExpr dst$, src$, buf, item_expr, nested_udt, layout_mode, loop_depth + 1
                    IF Error_Happened THEN EXIT SUB
                    WriteBufLineCpp buf, "}"
                ELSE
                    WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + _TOSTR$(member_bytes) + ");"
                END IF
            ELSE
                WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + _TOSTR$(member_bytes) + ");"
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            WriteBufLineCpp buf, "qbs_set(*(qbs**)(" + dst$ + "+(" + member_expr + ")),*(qbs**)(" + src$ + "+(" + member_expr + ")));"
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND UDTMASK
            IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                CopyDynUDTExpr dst$, src$, buf, member_expr, nested_udt, layout_mode, loop_depth
                IF Error_Happened THEN EXIT SUB
            ELSE
                WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + _TOSTR$(member_bytes) + ");"
            END IF
        ELSE
            WriteBufLineCpp buf, "memcpy((" + dst$ + "+(" + member_expr + ")),(" + src$ + "+(" + member_expr + "))," + _TOSTR$(member_bytes) + ");"
        END IF
        member_id = udtenext(member_id)
    LOOP
END SUB

SUB dump_udts
    fh = FREEFILE
    OPEN "types.txt" FOR OUTPUT AS #fh
    PRINT #fh, "Name   Size   Next   Var?"
    FOR i = 1 TO lasttype
        PRINT #fh, RTRIM$(udtxname(i)), udtxsize(i), udtxnext(i), udtxvariable(i)
    NEXT i
    PRINT #fh, "Name   Size   Next   Type   Tsize  Arr   FieldMode"
    FOR i = 1 TO lasttypeelement
        PRINT #fh, RTRIM$(udtename(i)), udtesize(i), udtenext(i), udtetype(i), udtetypesize(i), udtearrayelements(i), udtearraybase(i), udtearraydims(i), udtearraydesc(i), udtearrayfieldmode(i)
    NEXT i
    CLOSE #fh
END SUB

FUNCTION isuinteger (i$)
    IF LEN(i$) = 0 THEN EXIT FUNCTION
    IF ASC(i$, 1) = 48 AND LEN(i$) > 1 THEN EXIT FUNCTION
    FOR c = 1 TO LEN(i$)
        v = ASC(i$, c)
        IF v < 48 OR v > 57 THEN EXIT FUNCTION
    NEXT
    isuinteger = -1
END FUNCTION

FUNCTION Type_PromoteArithmeticType& (qbTypA AS LONG, qbTypB AS LONG)
    DIM typeA AS LONG: typeA = Type_StripContextFlags(qbTypA)
    DIM typeB AS LONG: typeB = Type_StripContextFlags(qbTypB)
    DIM isFloatA AS _BYTE: isFloatA = Type_IsFloatingPoint(typeA)
    DIM isFloatB AS _BYTE: isFloatB = Type_IsFloatingPoint(typeB)
    DIM isUnsignedA AS _BYTE: isUnsignedA = Type_IsUnsigned(typeA)
    DIM isUnsignedB AS _BYTE: isUnsignedB = Type_IsUnsigned(typeB)
    DIM sizeA AS _UNSIGNED LONG: sizeA = Type_GetSizeInBits(typeA)
    DIM sizeB AS _UNSIGNED LONG: sizeB = Type_GetSizeInBits(typeB)

    IF typeA = typeB THEN ' both are of the same type
        Type_PromoteArithmeticType = typeA
    ELSEIF typeA = UOFFSETTYPE _ORELSE typeB = UOFFSETTYPE THEN ' special case UOFFSET
        Type_PromoteArithmeticType = UOFFSETTYPE
    ELSEIF typeA = OFFSETTYPE _ORELSE typeB = OFFSETTYPE THEN ' special case OFFSET
        Type_PromoteArithmeticType = OFFSETTYPE
    ELSEIF (isFloatA _ANDALSO isFloatB) _ORELSE (isUnsignedA _ANDALSO isUnsignedB) THEN ' both are floating point or both are unsigned
        Type_PromoteArithmeticType = _IIF(sizeA > sizeB, typeA, typeB)
    ELSEIF sizeA = sizeB THEN ' both are of the same size
        IF isFloatA _ANDALSO NOT isFloatB THEN ' one is a floating point
            Type_PromoteArithmeticType = typeA
        ELSEIF NOT isFloatA _ANDALSO isFloatB THEN ' one is a floating point
            Type_PromoteArithmeticType = typeB
        ELSEIF isUnsignedA _ANDALSO NOT isUnsignedB THEN ' one is an unsigned
            Type_PromoteArithmeticType = typeA
        ELSEIF NOT isUnsignedA _ANDALSO isUnsignedB THEN ' one is an unsigned
            Type_PromoteArithmeticType = typeB
        ELSE
            Type_PromoteArithmeticType = typeA ' both are of the same sized signed type
        END IF
    ELSEIF sizeA < sizeB THEN ' one is smaller than the other
        IF isFloatA _ANDALSO NOT isFloatB THEN ' one is a floating point
            SELECT CASE typeA
                CASE SINGLETYPE
                    Type_PromoteArithmeticType = DOUBLETYPE
                CASE DOUBLETYPE
                    Type_PromoteArithmeticType = FLOATTYPE
                CASE FLOATTYPE
                    Type_PromoteArithmeticType = FLOATTYPE
                CASE ELSE
                    Type_PromoteArithmeticType = typeA
            END SELECT
        ELSE
            Type_PromoteArithmeticType = typeB ' promote the larger one
        END IF
    ELSEIF sizeA > sizeB THEN ' one is smaller than the other
        IF NOT isFloatA _ANDALSO isFloatB THEN ' one is a floating point
            SELECT CASE typeB
                CASE SINGLETYPE
                    Type_PromoteArithmeticType = DOUBLETYPE
                CASE DOUBLETYPE
                    Type_PromoteArithmeticType = FLOATTYPE
                CASE FLOATTYPE
                    Type_PromoteArithmeticType = FLOATTYPE
                CASE ELSE
                    Type_PromoteArithmeticType = typeB
            END SELECT
        ELSE
            Type_PromoteArithmeticType = typeA ' promote the larger one
        END IF
    END IF
END FUNCTION

FUNCTION Type_GetCppArithmeticType$ (typeId AS LONG)
    DIM sizeInBits AS _UNSIGNED LONG: sizeInBits = Type_GetSizeInBits(typeId)
    DIM cType AS STRING

    IF typeId AND ISOFFSETINBITS THEN
        IF sizeInBits <= 32 THEN cType = "int32_t" ELSE cType = "int64_t"
        IF typeId AND ISUNSIGNED THEN cType = "u" + cType
    ELSEIF typeId AND ISFLOAT THEN
        SELECT CASE sizeInBits
            CASE 32: cType = "float"
            CASE 64: cType = "double"
            CASE 256: cType = "long double"
            CASE ELSE: Give_Error "Invalid floating point type size"
        END SELECT
    ELSEIF typeId AND ISOFFSET THEN
        IF typeId AND ISUNSIGNED THEN cType = "uintptr_t" ELSE cType = "intptr_t"
    ELSE
        SELECT CASE sizeInBits
            CASE 8: cType = "int8_t"
            CASE 16: cType = "int16_t"
            CASE 32: cType = "int32_t"
            CASE 64: cType = "int64_t"
            CASE ELSE: Give_Error "Invalid integer type size": EXIT FUNCTION
        END SELECT
        IF typeId AND ISUNSIGNED THEN cType = "u" + cType
    END IF

    Type_GetCppArithmeticType = cType
END FUNCTION

FUNCTION Type_StripContextFlags& (typeId AS LONG)
    Type_StripContextFlags = typeId AND (NOT (ISARRAY OR ISREFERENCE OR ISUDT OR ISFIXEDLENGTH OR ISINCONVENTIONALMEMORY))
END FUNCTION

FUNCTION Type_GetSizeInBits~& (typeId AS LONG)
    Type_GetSizeInBits = typeId AND UDTMASK
END FUNCTION

FUNCTION Type_IsString%% (typeId AS LONG)
    Type_IsString = (typeId AND ISSTRING) <> _FALSE
END FUNCTION

FUNCTION Type_IsFixedString%% (typeId AS LONG)
    Type_IsFixedString = (typeId AND ISSTRING) _ANDALSO (typeId AND ISFIXEDLENGTH)
END FUNCTION

FUNCTION Type_IsDynamicString%% (typeId AS LONG)
    Type_IsDynamicString = (typeId AND ISSTRING) _ANDALSO _NEGATE (typeId AND ISFIXEDLENGTH)
END FUNCTION

FUNCTION Type_IsFloatingPoint%% (typeId AS LONG)
    Type_IsFloatingPoint = (typeId AND ISFLOAT) <> _FALSE
END FUNCTION

FUNCTION Type_IsUnsigned%% (typeId AS LONG)
    Type_IsUnsigned = (typeId AND ISUNSIGNED) <> _FALSE
END FUNCTION

FUNCTION Type_IsIntegral%% (typeId AS LONG)
    Type_IsIntegral = _NEGATE (typeId AND ISFLOAT) _ANDALSO _NEGATE (typeId AND ISSTRING)
END FUNCTION

FUNCTION Type_IsArithmetic%% (typeId AS LONG)
    Type_IsArithmetic = _NEGATE (typeId AND ISSTRING)
END FUNCTION

FUNCTION Type_IsArrayContainer%% (typeId AS LONG)
    ' ISPOINTER is always set by QB64 regardless of an index being used or not. This is probably a bug.
    'Type_IsArrayContainer = (typeId AND ISARRAY) _ANDALSO _NEGATE (typeId AND ISPOINTER)
    Type_IsArrayContainer = (typeId AND ISARRAY) <> _FALSE
END FUNCTION

FUNCTION Type_IsArrayElement%% (typeId AS LONG)
    Type_IsArrayElement = (typeId AND ISARRAY) _ANDALSO (typeId AND ISPOINTER)
END FUNCTION

FUNCTION Type_IsUDTContainer%% (typeId AS LONG)
    Type_IsUDTContainer = (typeId AND ISUDT) _ANDALSO _NEGATE (typeId AND ISPOINTER)
END FUNCTION

FUNCTION Type_IsUDTMember%% (typeId AS LONG)
    Type_IsUDTMember = (typeId AND ISUDT) _ANDALSO (typeId AND ISPOINTER)
END FUNCTION

FUNCTION Type_IsOffset%% (typeId AS LONG)
    Type_IsOffset = (typeId AND ISOFFSET) <> _FALSE
END FUNCTION

FUNCTION Type_IsBit%% (typeId AS LONG)
    Type_IsBit = (typeId AND ISOFFSETINBITS) <> _FALSE
END FUNCTION

FUNCTION Type_IsInConventionalMemory%% (typeId AS LONG)
    Type_IsInConventionalMemory = (typeId AND ISINCONVENTIONALMEMORY) <> _FALSE
END FUNCTION

