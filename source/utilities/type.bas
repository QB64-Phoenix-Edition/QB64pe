

'part for nested dynamic arrays:
'Emit C code that sets every descriptor slot in a descriptor-layout UDT element
'to NULL before ownership-aware cloning or partial initialization. A clean NULL
'baseline lets later free paths distinguish absent descriptors from initialized
'ones and avoids freeing garbage after failed/partial construction.
SUB AppendDynUDTDescNullSlots (rootptr$, udt AS LONG, off$, acc$, layout_mode AS LONG)
    DIM elemnum AS LONG
    DIM nestedudt AS LONG
    DIM dynbitpos AS LONG
    DIM dynoffbytes AS LONG
    DIM nestoff$

    elemnum = udtxnext(udt)
    dynbitpos = 0
    DO WHILE elemnum
        IF UDTMemberDynDesc%(elemnum, layout_mode) THEN
            IF dynbitpos MOD 8 THEN Give_Error "Non-byte aligned user defined type": EXIT SUB
            dynoffbytes = dynbitpos \ 8
            acc$ = acc$ + "*((ptrszint**)(((uint8*)(" + rootptr$ + "))+(" + off$ + "+" + _TOSTR$(dynoffbytes) + ")))=NULL;" + CHR$(13) + CHR$(10)
        ELSEIF (udtetype(elemnum) AND ISUDT) <> 0 THEN
            nestedudt = udtetype(elemnum) AND 511
            IF UDTDynHasMemberArrays%(nestedudt, layout_mode) THEN
                IF dynbitpos MOD 8 THEN Give_Error "Non-byte aligned user defined type": EXIT SUB
                dynoffbytes = dynbitpos \ 8
                nestoff$ = "(" + off$ + "+" + _TOSTR$(dynoffbytes) + ")"
                AppendDynUDTDescNullSlots rootptr$, nestedudt, nestoff$, acc$, layout_mode
                IF Error_Happened THEN EXIT SUB
            END IF
        END IF
        dynbitpos = dynbitpos + UDTDynMemberSize&(elemnum, layout_mode)
        elemnum = udtenext(elemnum)
    LOOP
END SUB


'part for nested dynamic arrays:
'Emit deep-copy C code for descriptor-backed member arrays inside one UDT element.
'Numeric/fixed-size descriptor payloads can be copied with memcpy, but variable
'strings and nested owner UDTs need ownership-aware qbs/descriptor cloning so the
'source and destination never share owned payloads.
SUB AppendDynUDTDescCopy (dstbase$, srcbase$, udt AS LONG, dstoff$, srcoff$, bytesperelement$, acc$, layout_mode AS LONG)
    IF udtxvariable(udt) THEN
        AppendDynUDTOwnSetAt "((uint8*)(" + dstbase$ + ")+(" + dstoff$ + "))", "((uint8*)(" + srcbase$ + ")+(" + srcoff$ + "))", udt, 0, 0, bytesperelement$, "0", "0", acc$, layout_mode
        EXIT SUB
    END IF

    DIM elemnum AS LONG
    DIM nestedudt AS LONG
    DIM dynbitpos AS LONG
    DIM dynoffbytes AS LONG
    DIM memberelembytes AS LONG
    DIM elemvarstr AS LONG
    DIM elemudtvarstr AS LONG
    DIM copycode AS STRING
    DIM nesteddst$
    DIM nestedsrc$

    elemnum = udtxnext(udt)
    dynbitpos = 0
    DO WHILE elemnum
        IF UDTMemberDynDesc%(elemnum, layout_mode) THEN
            IF dynbitpos MOD 8 THEN Give_Error "Non-byte aligned user defined type": EXIT SUB
            dynoffbytes = dynbitpos \ 8
            memberelembytes = udt_dyn_array_elem_bytes(elemnum, layout_mode)
            elemvarstr = DynMemVarStr%(elemnum)
            nestedudt = 0
            elemudtvarstr = 0
            IF (udtetype(elemnum) AND ISUDT) <> 0 THEN
                nestedudt = udtetype(elemnum) AND 511
                IF udtxvariable(nestedudt) THEN elemudtvarstr = -1
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
            ELSEIF elemudtvarstr OR (nestedudt <> 0 AND UDTDynHasMemberArrays%(nestedudt, layout_mode)) THEN
                copycode = copycode + "for(ptrszint dyn_elem_i=0; dyn_elem_i<(ptrszint)dyn_total; dyn_elem_i++){" + CHR$(13) + CHR$(10)
                AppendDynUDTOwnInitAt "(void*)dyn_new_desc[0]", nestedudt, 0, _TOSTR$(memberelembytes), "dyn_elem_i", copycode, "", layout_mode
                IF Error_Happened THEN EXIT SUB
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
                copycode = copycode + "for(ptrszint dyn_elem_i=0; dyn_elem_i<(ptrszint)dyn_total; dyn_elem_i++){" + CHR$(13) + CHR$(10)
                AppendDynUDTOwnSetAt "(void*)dyn_new_desc[0]", "(void*)dyn_src_desc[0]", nestedudt, 0, 0, _TOSTR$(memberelembytes), "dyn_elem_i", "dyn_elem_i", copycode, layout_mode
                IF Error_Happened THEN EXIT SUB
                copycode = copycode + "}" + CHR$(13) + CHR$(10)
            ELSE
                copycode = copycode + "memcpy((void*)dyn_new_desc[0],(void*)dyn_src_desc[0],(size_t)dyn_bytes);" + CHR$(13) + CHR$(10)
                IF nestedudt <> 0 AND UDTDynHasMemberArrays%(nestedudt, layout_mode) THEN
                    copycode = copycode + "for(ptrszint dyn_elem_i=0; dyn_elem_i<(ptrszint)dyn_total; dyn_elem_i++){" + CHR$(13) + CHR$(10)
                    AppendDynUDTDescNullSlots "(void*)dyn_new_desc[0]", nestedudt, "(dyn_elem_i*" + _TOSTR$(memberelembytes) + ")", copycode, layout_mode
                    IF Error_Happened THEN EXIT SUB
                    AppendDynUDTDescCopy "(void*)dyn_new_desc[0]", "(void*)dyn_src_desc[0]", nestedudt, "(dyn_elem_i*" + _TOSTR$(memberelembytes) + ")", "(dyn_elem_i*" + _TOSTR$(memberelembytes) + ")", _TOSTR$(memberelembytes), copycode, layout_mode
                    IF Error_Happened THEN EXIT SUB
                    copycode = copycode + "}" + CHR$(13) + CHR$(10)
                END IF
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
            IF (nestedudt <> 0 AND (elemudtvarstr OR UDTDynHasMemberArrays%(nestedudt, layout_mode))) THEN
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
            nestedudt = udtetype(elemnum) AND 511
            IF UDTDynHasMemberArrays%(nestedudt, layout_mode) THEN
                IF dynbitpos MOD 8 THEN Give_Error "Non-byte aligned user defined type": EXIT SUB
                dynoffbytes = dynbitpos \ 8
                nesteddst$ = "(" + dstoff$ + "+" + _TOSTR$(dynoffbytes) + ")"
                nestedsrc$ = "(" + srcoff$ + "+" + _TOSTR$(dynoffbytes) + ")"
                AppendDynUDTDescCopy dstbase$, srcbase$, nestedudt, nesteddst$, nestedsrc$, _TOSTR$(UDTDynMemberSize&(elemnum, layout_mode) \ 8), acc$, layout_mode
                IF Error_Happened THEN EXIT SUB
            END IF
        END IF
        dynbitpos = dynbitpos + UDTDynMemberSize&(elemnum, layout_mode)
        elemnum = udtenext(elemnum)
    LOOP
END SUB

'part for nested dynamic arrays:
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

    b = t AND 511

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
    bits = t AND 511
    IF t AND ISUDT THEN
        a$ = RTRIM$(udtxcname(t AND 511))
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
    bits = t AND 511
    IF t AND ISUDT THEN
        a$ = RTRIM$(udtxcname(t AND 511))
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
        b = t AND 511
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
    ' The dynamic-member metadata is parallel to the legacy UDT tables.
    ' *_dyn* stores the normal REDIM/runtime descriptor layout; *_fdyn* stores
    ' the forced-only layout used when explicit _DynamicField members appear in
    ' otherwise inline/DIM-created UDT storage. Both layouts are kept because
    ' the same TYPE can be instantiated through different storage paths.
    x = UBOUND(udtxname)
    REDIM _PRESERVE udtxname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtxcname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtxsize(x + 1000) AS LONG
    REDIM _PRESERVE udtxdynsize(x + 1000) AS LONG
    REDIM _PRESERVE udtxfdynsize(x + 1000) AS LONG
    REDIM _PRESERVE udtxnext(x + 1000) AS LONG
    REDIM _PRESERVE udtxvariable(x + 1000) AS INTEGER 'true if the udt contains variable length elements
    'elements
    REDIM _PRESERVE udtename(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtecname(x + 1000) AS STRING * 256
    REDIM _PRESERVE udtesize(x + 1000) AS LONG
    REDIM _PRESERVE udtedynoffset(x + 1000) AS LONG
    REDIM _PRESERVE udtedynsize(x + 1000) AS LONG
    REDIM _PRESERVE udtefdynoffset(x + 1000) AS LONG
    REDIM _PRESERVE udtefdynsize(x + 1000) AS LONG
    REDIM _PRESERVE udtetype(x + 1000) AS LONG
    REDIM _PRESERVE udtetypesize(x + 1000) AS LONG
    REDIM _PRESERVE udtearrayelements(x + 1000) AS LONG
    REDIM _PRESERVE udtearraybase(x + 1000) AS LONG
    REDIM _PRESERVE udtearraydims(x + 1000) AS LONG
    ' udtearraydesc stores serialized member-array bounds. Compile-time bounds
    ' are encoded as numeric lower/count pairs; runtime-bound descriptor fields
    ' use an @ prefix and keep the original bound expressions for later C prep.
    REDIM _PRESERVE udtearraydesc(x + 1000) AS STRING
    ' udtearrayfieldmode: 0 = legacy/default inline member array,
    ' 1 = explicit _StaticField, 2 = explicit _DynamicField descriptor member.
    ' Unmarked compile-time arrays must remain inline for compatibility, even
    ' when the parent UDT variable/array is created with REDIM.
    REDIM _PRESERVE udtearrayfieldmode(x + 1000) AS LONG
    REDIM _PRESERVE udtenext(x + 1000) AS LONG
END SUB

' Build the alternate physical layouts used by nested member arrays.
' Layout mode 1 is the forced-only descriptor layout for explicit _DynamicField
' members under otherwise inline storage. Layout mode 2 is the full dynamic
' layout used by REDIM/runtime descriptor paths. Static legacy layout remains
' stored in udtxsize/udtesize and is not modified here.
SUB BuildUDTDynLayout (udt_index AS LONG)
    BuildUDTDynLayoutM udt_index, 1
    BuildUDTDynLayoutM udt_index, 2
END SUB

SUB BuildUDTDynLayoutM (udt_index AS LONG, layout_mode AS LONG)
    ' Each descriptor-backed member occupies one pointer-sized slot in the
    ' parent UDT layout. Inline members keep their byte size, except nested UDTs
    ' that themselves contain descriptor-backed members: those must use the
    ' corresponding dynamic layout size so offsets stay consistent recursively.
    DIM member_id AS LONG
    DIM nested_udt AS LONG
    DIM dynbits AS LONG
    DIM ptrbits AS LONG
    DIM membersize AS LONG

    member_id = udtxnext(udt_index)
    dynbits = 0
    ptrbits = OFFSETTYPE AND 511

    DO WHILE member_id
        IF layout_mode = 1 THEN
            udtefdynoffset(member_id) = dynbits
        ELSE
            udtedynoffset(member_id) = dynbits
        END IF

        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            ' Descriptor-backed members occupy only a pointer-sized slot in the
            ' parent layout. Descriptor headers and payloads are allocated later
            ' by the initialization helpers.
            membersize = ptrbits
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    membersize = udtearrayelements(member_id) * UDTDynLayoutSize&(nested_udt, layout_mode)
                ELSE
                    membersize = udtesize(member_id)
                END IF
            ELSE
                membersize = udtesize(member_id)
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            nested_udt = udtetype(member_id) AND 511
            IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                membersize = UDTDynLayoutSize&(nested_udt, layout_mode)
            ELSE
                membersize = udtesize(member_id)
            END IF
        ELSE
            membersize = udtesize(member_id)
        END IF

        IF layout_mode = 1 THEN
            udtefdynsize(member_id) = membersize
        ELSE
            udtedynsize(member_id) = membersize
        END IF

        dynbits = dynbits + membersize
        member_id = udtenext(member_id)
    LOOP

    IF layout_mode = 1 THEN
        udtxfdynsize(udt_index) = dynbits
    ELSE
        udtxdynsize(udt_index) = dynbits
    END IF
END SUB

' Return true when this UDT, or any nested UDT member, contains descriptor-backed
' member arrays in the requested layout mode. This is the cheap gate used by
' init/free/copy code to decide whether plain memcpy is safe.
FUNCTION UDTDynHasMemberArrays% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG

    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            UDTDynHasMemberArrays% = -1
            EXIT FUNCTION
        END IF
        IF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                UDTDynHasMemberArrays% = -1
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' Return true when a descriptor-backed member array uses runtime-evaluated
' bounds. Runtime metadata is marked by an @ prefix in udtearraydesc and requires
' bound-prep code from qb64pe.bas before descriptors can be allocated.
FUNCTION UDTDynHasRunArrays% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG

    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            IF LEFT$(udtearraydesc(member_id), 1) = "@" THEN
                UDTDynHasRunArrays% = -1
                EXIT FUNCTION
            END IF
        END IF
        IF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF UDTDynHasRunArrays%(nested_udt, layout_mode) THEN
                UDTDynHasRunArrays% = -1
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
                    ' Variable-length STRING arrays are ownership-bearing. If they are
                    ' descriptor-backed, they must be explicitly marked _DynamicField so
                    ' legacy inline arrays do not silently change storage model.
                    IF UDTMemberDynDesc%(member_id, layout_mode) THEN
                        IF udtearrayfieldmode(member_id) <> 2 THEN
                            Give_Error "Dynamic TYPE variable-length STRING member arrays require _DynamicField for now"
                            UDTDynMembersOK% = 0
                            EXIT FUNCTION
                        END IF
                    ELSE
                        'Inline/static-bound variable-length STRING arrays are allowed in
                        'owner layouts now; the owner lifecycle helpers initialize/free
                        'each qbs* slot instead of treating the member as one scalar string.
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
                nested_udt = udtetype(member_id) AND 511
                ' A descriptor-backed UDT array whose element owns variable strings needs
                ' the recursive owner helpers. Keep that path explicit so ordinary inline
                ' UDT arrays remain compatible with legacy layout.
                IF UDTMemberDynDesc%(member_id, layout_mode) AND udtxvariable(nested_udt) THEN
                    IF udtearrayfieldmode(member_id) <> 2 THEN
                        Give_Error "Dynamic TYPE UDT member arrays containing variable-length strings require _DynamicField for now"
                        UDTDynMembersOK% = 0
                        EXIT FUNCTION
                    END IF
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
        ' Scalar variable STRINGs are only accepted when the enclosing UDT qualifies
        ' as an owner layout. Otherwise this older path still only supports fixed-size
        ' scalar storage in dynamic TYPE layouts.
        ELSEIF (udtetype(member_id) AND ISSTRING) <> 0 THEN
            IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                IF UDTDynOwnerOK%(udt_index, layout_mode) THEN EXIT FUNCTION
                IF Error_Happened THEN
                    UDTDynMembersOK% = 0
                    EXIT FUNCTION
                END IF
                Give_Error "Dynamic TYPE layout supports only fixed-length strings for now"
                UDTDynMembersOK% = 0
                EXIT FUNCTION
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            IF UDTDynMembersOK%(udtetype(member_id) AND 511, layout_mode) = 0 THEN
                UDTDynMembersOK% = 0
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' True for descriptor-backed _DynamicField AS STRING member arrays. The payload
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

' True for descriptor-backed _DynamicField AS UDT where the element UDT owns
' variable-length strings. Such payloads require owner-aware recursive handling
' instead of memcpy, even though the descriptor header itself is fixed-size.
FUNCTION DynMemUDTVarStr% (member_id AS LONG)
    DIM nested_udt AS LONG
    IF member_id <= 0 THEN EXIT FUNCTION
    IF udtearrayelements(member_id) = 0 THEN EXIT FUNCTION
    IF udtearrayfieldmode(member_id) <> 2 THEN EXIT FUNCTION
    IF (udtetype(member_id) AND ISUDT) = 0 THEN EXIT FUNCTION
    nested_udt = udtetype(member_id) AND 511
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
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            seen_dyn = -1
            IF DynMemVarStr%(member_id) THEN
                UDTDynHasVarDesc% = -1
                EXIT FUNCTION
            END IF
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
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
            nested_udt = udtetype(member_id) AND 511
            IF UDTDynHasVarDesc%(nested_udt, layout_mode) THEN
                UDTDynHasVarDesc% = -1
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' Validate owner-layout UDTs. An owner layout is a dynamic physical layout that
' contains scalar variable strings and/or descriptor-owned member arrays, so it
' must be initialized, freed and copied member-by-member instead of by raw bytes.
FUNCTION UDTDynOwnerOK% (udt_index AS LONG, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM nested_udt AS LONG
    DIM seen_dyn AS LONG

    UDTDynOwnerOK% = -1
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            seen_dyn = -1
            '_DynamicField AS STRING is now allowed in dynamic TYPE owner layouts.
            'The descriptor helpers initialize/free/copy the qbs* element slots, while
            '_MEM and PUT/GET remain rejected through the existing variable-string guards.
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                ' A descriptor-backed _DynamicField AS UDT element may itself be an owner
                ' layout. The recursive owner lifecycle helpers below now initialize, free and
                ' copy scalar variable STRINGs, _DynamicField AS STRING payloads, and nested
                ' descriptor-owned members, so this validation recurses instead of enforcing the
                ' older fixed-size-only restriction.
                IF UDTDynOwnerOK%(nested_udt, layout_mode) = 0 THEN
                    UDTDynOwnerOK% = 0
                    EXIT FUNCTION
                END IF
            END IF
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISSTRING) <> 0 THEN
                IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                    'Inline/static-bound variable-length STRING arrays are owner-managed
                    'qbs* slots. They are not fixed-length strings, but they are now
                    'initialized/freed/copied slot-by-slot in the owner helpers.
                END IF
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            ' Scalar variable strings are allowed before, after, or between descriptor-backed
            ' member arrays. Unsupported descriptor-owning member types are still rejected
            ' when their own _DynamicField member is visited.
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF UDTDynOwnerOK%(nested_udt, layout_mode) = 0 THEN
                UDTDynOwnerOK% = 0
                EXIT FUNCTION
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END FUNCTION

' Helper for order-sensitive owner validation. It answers whether a later member
' in the same TYPE still needs descriptor-backed storage in this layout mode.
FUNCTION UDTDynHasFollowingDynMember% (member_id AS LONG, layout_mode AS LONG)
    DIM scan_id AS LONG

    scan_id = udtenext(member_id)
    DO WHILE scan_id
        IF UDTMemberDynDesc%(scan_id, layout_mode) THEN
            UDTDynHasFollowingDynMember% = -1
            EXIT FUNCTION
        END IF
        scan_id = udtenext(scan_id)
    LOOP
END FUNCTION

' Generate C code to initialize legacy inline qbs* slots inside one UDT element.
' This is used for nested variable STRING ownership that is stored inline rather
' than behind a descriptor slot.
SUB AppendDynUDTVarInitAt (base_expr AS STRING, udt_index AS LONG, root_offset AS LONG, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING)
    DIM member_id AS LONG
    DIM member_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM nested_udt AS LONG
    DIM cr AS STRING

    IF udtxvariable(udt_index) = 0 THEN EXIT SUB
    cr = CHR$(13) + CHR$(10)
    member_offset = root_offset
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF udtearrayelements(member_id) THEN
            elem_step = udt_array_member_bytes(member_id)
            FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                array_offset = member_offset + array_idx * elem_step
                IF (udtetype(member_id) AND ISSTRING) <> 0 THEN
                    IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                        acc = acc + cr + "*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(array_offset)) + ")=qbs_new(0,0);"
                    END IF
                ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                    nested_udt = udtetype(member_id) AND 511
                    AppendDynUDTVarInitAt base_expr, nested_udt, array_offset, elem_bytes, index_expr, acc
                END IF
            NEXT
        ELSEIF (udtetype(member_id) AND ISSTRING) <> 0 THEN
            IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                acc = acc + cr + "*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(member_offset)) + ")=qbs_new(0,0);"
            END IF
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            AppendDynUDTVarInitAt base_expr, nested_udt, member_offset, elem_bytes, index_expr, acc
        END IF
        member_offset = member_offset + udtesize(member_id) \ 8
        member_id = udtenext(member_id)
    LOOP
END SUB

' Generate C code to free legacy inline qbs* slots inside one UDT element before
' the containing storage is released or replaced.
SUB AppendDynUDTVarFreeAt (base_expr AS STRING, udt_index AS LONG, root_offset AS LONG, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING)
    DIM member_id AS LONG
    DIM member_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM nested_udt AS LONG
    DIM cr AS STRING

    IF udtxvariable(udt_index) = 0 THEN EXIT SUB
    cr = CHR$(13) + CHR$(10)
    member_offset = root_offset
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF udtearrayelements(member_id) THEN
            elem_step = udt_array_member_bytes(member_id)
            FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                array_offset = member_offset + array_idx * elem_step
                IF (udtetype(member_id) AND ISSTRING) <> 0 THEN
                    IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                        acc = acc + cr + "qbs_free(*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(array_offset)) + "));"
                    END IF
                ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                    nested_udt = udtetype(member_id) AND 511
                    AppendDynUDTVarFreeAt base_expr, nested_udt, array_offset, elem_bytes, index_expr, acc
                END IF
            NEXT
        ELSEIF (udtetype(member_id) AND ISSTRING) <> 0 THEN
            IF (udtetype(member_id) AND ISFIXEDLENGTH) = 0 THEN
                acc = acc + cr + "qbs_free(*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(member_offset)) + "));"
            END IF
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            AppendDynUDTVarFreeAt base_expr, nested_udt, member_offset, elem_bytes, index_expr, acc
        END IF
        member_offset = member_offset + udtesize(member_id) \ 8
        member_id = udtenext(member_id)
    LOOP
END SUB

' Generate C code for deep assignment of legacy inline variable STRING members.
' Numeric/fixed-size fields still use memcpy; qbs* fields use qbs_set to avoid
' aliasing string ownership between source and destination.
SUB AppendDynUDTVarSetAt (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root AS LONG, src_root AS LONG, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING)
    DIM member_id AS LONG
    DIM dst_offset AS LONG
    DIM src_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM dst_array_offset AS LONG
    DIM src_array_offset AS LONG
    DIM nested_udt AS LONG
    DIM cr AS STRING

    cr = CHR$(13) + CHR$(10)
    dst_offset = dst_root
    src_offset = src_root
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF udtearrayelements(member_id) THEN
            elem_step = udt_array_member_bytes(member_id)
            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    dst_array_offset = dst_offset + array_idx * elem_step
                    src_array_offset = src_offset + array_idx * elem_step
                    acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_array_offset)) + "); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_array_offset)) + "); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}" 
                NEXT
            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF udtxvariable(nested_udt) THEN
                    FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                        dst_array_offset = dst_offset + array_idx * elem_step
                        src_array_offset = src_offset + array_idx * elem_step
                        AppendDynUDTVarSetAt dst_expr, src_expr, nested_udt, dst_array_offset, src_array_offset, elem_bytes, dst_index, src_index, acc
                    NEXT
                ELSE
                    acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
                END IF
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + "); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + "); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}" 
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF udtxvariable(nested_udt) THEN
                AppendDynUDTVarSetAt dst_expr, src_expr, nested_udt, dst_offset, src_offset, elem_bytes, dst_index, src_index, acc
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
            END IF
        ELSE
            acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
        END IF
        dst_offset = dst_offset + udtesize(member_id) \ 8
        src_offset = src_offset + udtesize(member_id) \ 8
        member_id = udtenext(member_id)
    LOOP
END SUB

' Descriptor payload helper for _DynamicField AS STRING. The payload stores qbs*
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


' Generate C code to initialize one owner-layout UDT element. The first
' descriptor-backed member triggers descriptor initialization for the whole UDT;
' scalar/inline variable strings and nested owner UDTs are then initialized
' recursively at their dynamic-layout offsets.
SUB AppendDynUDTOwnInitAt (base_expr AS STRING, udt_index AS LONG, root_offset AS LONG, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING, prep_prefix AS STRING, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM member_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM nested_udt AS LONG
    DIM desc_inited AS LONG
    DIM cr AS STRING

    cr = CHR$(13) + CHR$(10)
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_offset = root_offset + UDTDynMemberOffset&(member_id, layout_mode) \ 8
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            IF desc_inited = 0 THEN
                AppendDynUDTDescInitAt base_expr, udt_index, root_offset, elem_bytes, index_expr, acc, prep_prefix, layout_mode
                IF Error_Happened THEN EXIT SUB
                desc_inited = -1
            END IF
        ELSEIF udtearrayelements(member_id) THEN
            elem_step = UDTDynInlineElemBytes&(member_id, layout_mode)
            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                elem_step = udt_array_member_bytes(member_id)
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    array_offset = member_offset + array_idx * elem_step
                    acc = acc + cr + "*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(array_offset)) + ")=qbs_new(0,0);"
                NEXT
            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                        array_offset = member_offset + array_idx * elem_step
                        AppendDynUDTOwnInitAt base_expr, nested_udt, array_offset, elem_bytes, index_expr, acc, prep_prefix, layout_mode
                        IF Error_Happened THEN EXIT SUB
                    NEXT
                END IF
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            acc = acc + cr + "*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(member_offset)) + ")=qbs_new(0,0);"
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                AppendDynUDTOwnInitAt base_expr, nested_udt, member_offset, elem_bytes, index_expr, acc, prep_prefix, layout_mode
                IF Error_Happened THEN EXIT SUB
            END IF
        END IF
        member_id = udtenext(member_id)
    LOOP
END SUB

' Generate C code to release one owner-layout UDT element. Descriptor payloads
' are freed once per containing UDT element, while scalar/inline qbs* slots and
' nested owner UDTs are walked recursively.
SUB AppendDynUDTOwnFreeAt (base_expr AS STRING, udt_index AS LONG, root_offset AS LONG, elem_bytes AS STRING, index_expr AS STRING, acc AS STRING, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM member_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM nested_udt AS LONG
    DIM desc_freed AS LONG
    DIM cr AS STRING

    cr = CHR$(13) + CHR$(10)
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_offset = root_offset + UDTDynMemberOffset&(member_id, layout_mode) \ 8
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            IF desc_freed = 0 THEN
                AppendDynUDTDescFreeAt base_expr, udt_index, root_offset, elem_bytes, index_expr, acc, layout_mode
                IF Error_Happened THEN EXIT SUB
                desc_freed = -1
            END IF
        ELSEIF udtearrayelements(member_id) THEN
            elem_step = UDTDynInlineElemBytes&(member_id, layout_mode)
            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                elem_step = udt_array_member_bytes(member_id)
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    array_offset = member_offset + array_idx * elem_step
                    acc = acc + cr + "qbs_free(*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(array_offset)) + "));"
                NEXT
            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                        array_offset = member_offset + array_idx * elem_step
                        AppendDynUDTOwnFreeAt base_expr, nested_udt, array_offset, elem_bytes, index_expr, acc, layout_mode
                        IF Error_Happened THEN EXIT SUB
                    NEXT
                END IF
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            acc = acc + cr + "qbs_free(*(qbs**)(((uint8*)(" + base_expr + "))+" + elem_bytes + "*(" + index_expr + ")+" + LTRIM$(STR$(member_offset)) + "));"
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                AppendDynUDTOwnFreeAt base_expr, nested_udt, member_offset, elem_bytes, index_expr, acc, layout_mode
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
    DIM member_id AS LONG
    DIM dst_offset AS LONG
    DIM src_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM dst_array_offset AS LONG
    DIM src_array_offset AS LONG
    DIM nested_udt AS LONG
    DIM member_elem_bytes AS LONG
    DIM dst_slot AS STRING
    DIM src_slot AS STRING
    DIM cr AS STRING

    cr = CHR$(13) + CHR$(10)
    dst_offset = dst_root
    src_offset = src_root
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            member_elem_bytes = udt_dyn_array_elem_bytes(member_id, layout_mode)
            nested_udt = 0
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN nested_udt = udtetype(member_id) AND 511
            dst_slot = "((uint8*)" + dst_expr + "+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ")"
            src_slot = "((uint8*)" + src_expr + "+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ")"
            AppendDynMemberEraseEx dst_slot, member_elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
            IF Error_Happened THEN EXIT SUB
            AppendDynMemberCloneAfterRaw src_slot, dst_slot, member_elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        ELSEIF udtearrayelements(member_id) THEN
            elem_step = udt_array_member_bytes(member_id)
            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    dst_array_offset = dst_offset + array_idx * elem_step
                    src_array_offset = src_offset + array_idx * elem_step
                    acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_array_offset)) + "); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_array_offset)) + "); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}" 
                NEXT
            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    dst_array_offset = dst_offset + array_idx * elem_step
                    src_array_offset = src_offset + array_idx * elem_step
                    IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                        AppendDynUDTOwnSetAt dst_expr, src_expr, nested_udt, dst_array_offset, src_array_offset, elem_bytes, dst_index, src_index, acc, layout_mode
                    ELSE
                        acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_array_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_array_offset)) + ",(size_t)" + LTRIM$(STR$(elem_step)) + ");"
                    END IF
                    IF Error_Happened THEN EXIT SUB
                NEXT
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + "); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + "); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}" 
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                AppendDynUDTOwnSetAt dst_expr, src_expr, nested_udt, dst_offset, src_offset, elem_bytes, dst_index, src_index, acc, layout_mode
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
            END IF
        ELSE
            acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
        END IF
        dst_offset = dst_offset + UDTDynMemberSize&(member_id, layout_mode) \ 8
        src_offset = src_offset + UDTDynMemberSize&(member_id, layout_mode) \ 8
        member_id = udtenext(member_id)
    LOOP
END SUB

' Clone an owner-layout UDT element into zero-filled destination storage.
' Unlike AppendDynUDTOwnSetAt, this helper must not erase destination members first.
' It is used by whole-array assignment replace-copy: the new parent payload is
' calloc/zeroed, then each source element is deep-cloned into the clean slot.
' This avoids the unsafe init -> erase -> clone pattern for nested owner graphs.
SUB AppendDynUDTOwnCloneAt (dst_expr AS STRING, src_expr AS STRING, udt_index AS LONG, dst_root AS LONG, src_root AS LONG, elem_bytes AS STRING, dst_index AS STRING, src_index AS STRING, acc AS STRING, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM dst_offset AS LONG
    DIM src_offset AS LONG
    DIM elem_step AS LONG
    DIM array_idx AS LONG
    DIM dst_array_offset AS LONG
    DIM src_array_offset AS LONG
    DIM nested_udt AS LONG
    DIM member_elem_bytes AS LONG
    DIM dst_slot AS STRING
    DIM src_slot AS STRING
    DIM cr AS STRING

    cr = CHR$(13) + CHR$(10)
    dst_offset = dst_root
    src_offset = src_root
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            member_elem_bytes = udt_dyn_array_elem_bytes(member_id, layout_mode)
            nested_udt = 0
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN nested_udt = udtetype(member_id) AND 511
            dst_slot = "((uint8*)" + dst_expr + "+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ")"
            src_slot = "((uint8*)" + src_expr + "+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ")"
            AppendDynMemberCloneAfterRaw src_slot, dst_slot, member_elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        ELSEIF udtearrayelements(member_id) THEN
            elem_step = udt_array_member_bytes(member_id)
            IF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    dst_array_offset = dst_offset + array_idx * elem_step
                    src_array_offset = src_offset + array_idx * elem_step
                    acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_array_offset)) + "); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_array_offset)) + "); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}" 
                NEXT
            ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                    dst_array_offset = dst_offset + array_idx * elem_step
                    src_array_offset = src_offset + array_idx * elem_step
                    IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                        AppendDynUDTOwnCloneAt dst_expr, src_expr, nested_udt, dst_array_offset, src_array_offset, elem_bytes, dst_index, src_index, acc, layout_mode
                    ELSE
                        acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_array_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_array_offset)) + ",(size_t)" + LTRIM$(STR$(elem_step)) + ");"
                    END IF
                    IF Error_Happened THEN EXIT SUB
                NEXT
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
            END IF
        ELSEIF ((udtetype(member_id) AND ISSTRING) <> 0) AND ((udtetype(member_id) AND ISFIXEDLENGTH) = 0) THEN
            acc = acc + cr + "{qbs **dyn_udt_qbs_dst=(qbs**)(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + "); qbs *dyn_udt_qbs_src=*(qbs**)(((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + "); if(!*dyn_udt_qbs_dst) *dyn_udt_qbs_dst=qbs_new(0,0); if(dyn_udt_qbs_src) qbs_set(*dyn_udt_qbs_dst,dyn_udt_qbs_src);}" 
        ELSEIF (udtetype(member_id) AND ISUDT) <> 0 THEN
            nested_udt = udtetype(member_id) AND 511
            IF udtxvariable(nested_udt) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                AppendDynUDTOwnCloneAt dst_expr, src_expr, nested_udt, dst_offset, src_offset, elem_bytes, dst_index, src_index, acc, layout_mode
            ELSE
                acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
            END IF
        ELSE
            acc = acc + cr + "memcpy(((uint8*)(" + dst_expr + "))+" + elem_bytes + "*(" + dst_index + ")+" + LTRIM$(STR$(dst_offset)) + ",((uint8*)(" + src_expr + "))+" + elem_bytes + "*(" + src_index + ")+" + LTRIM$(STR$(src_offset)) + ",(size_t)" + LTRIM$(STR$(udtesize(member_id) \ 8)) + ");"
        END IF
        dst_offset = dst_offset + UDTDynMemberSize&(member_id, layout_mode) \ 8
        src_offset = src_offset + UDTDynMemberSize&(member_id, layout_mode) \ 8
        member_id = udtenext(member_id)
    LOOP
END SUB

' Entry point used by qb64pe.bas when an array of UDT elements needs descriptor
' initialization. Variable-string owner layouts go through the owner initializer;
' pure descriptor layouts can initialize descriptor slots directly.
SUB AppendDynUDTDescInit (n$, udt_index AS LONG, root_offset AS LONG, bytesperelement$, acc$, prep_prefix AS STRING, layout_mode AS LONG)
    IF udtxvariable(udt_index) THEN
        AppendDynUDTOwnInitAt "(void*)" + n$ + "[0]", udt_index, root_offset, bytesperelement$, "tmp_long", acc$, prep_prefix, layout_mode
    ELSE
        AppendDynUDTDescInitAt n$ + "[0]", udt_index, root_offset, bytesperelement$, "tmp_long", acc$, prep_prefix, layout_mode
    END IF
END SUB

' Generate the C descriptor headers and payload allocations for nested member
' arrays. Each descriptor records payload pointer, dimension metadata and a mem
' lock. Runtime-bound descriptors consume prepared lower/upper variables;
' compile-time descriptors use serialized lower/count metadata from type parsing.
SUB AppendDynUDTDescInitAt (data_expr AS STRING, udt_index AS LONG, root_offset AS LONG, bytesperelement AS STRING, index_expr AS STRING, acc AS STRING, prep_prefix AS STRING, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM member_offset AS LONG
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
    DIM desc_var AS STRING
    DIM member_desc AS STRING
    DIM lower_expr AS STRING
    DIM upper_expr AS STRING
    DIM lower_text AS STRING
    DIM upper_text AS STRING
    DIM total_expr AS STRING
    DIM nested_udt AS LONG
    DIM elem_idx AS STRING
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM inline_bytes AS LONG
    DIM cr AS STRING

    cr = CHR$(13) + CHR$(10)
    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_offset = root_offset + UDTDynMemberOffset&(member_id, layout_mode) \ 8

        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            ' A descriptor-backed member stores only a pointer-sized slot in the
            ' parent layout; this generated C block allocates the descriptor
            ' header and the actual payload behind that slot.
            elem_bytes = udt_dyn_array_elem_bytes(member_id, layout_mode)
            dims_count = udtearraydims(member_id)
            desc_slots = 4 * dims_count + 4 + 1
            count_total = 1
            desc_pos = 1
            stride_val = 1
            member_desc = udtearraydesc(member_id)
            member_ptr = "((ptrszint**)((uint8*)(" + data_expr + ")+" + bytesperelement + "*(" + index_expr + ")+" + LTRIM$(STR$(member_offset)) + "))"
            desc_var = "dyn_udt_desc"

            acc = acc + cr + "{"
            acc = acc + cr + "ptrszint **dyn_udt_slot=" + member_ptr + ";"
            acc = acc + cr + "ptrszint *dyn_udt_desc=(ptrszint*)calloc((size_t)" + LTRIM$(STR$(desc_slots)) + ",ptrsz);"
            acc = acc + cr + "if (!dyn_udt_desc) error(257);"
            acc = acc + cr + "*dyn_udt_slot=dyn_udt_desc;"
            acc = acc + cr + "new_mem_lock();"
            acc = acc + cr + "mem_lock_tmp->type=4;"
            acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_slots - 1)) + "]=(ptrszint)mem_lock_tmp;"
            acc = acc + cr + desc_var + "[1]=0;"
            acc = acc + cr + desc_var + "[2]=1;"
            acc = acc + cr + desc_var + "[3]=" + LTRIM$(STR$(dims_count)) + ";"

            ' Runtime-bound descriptors cannot be resolved while parsing TYPE.
            ' qb64pe.bas emits prep variables for each dimension; this block consumes
            ' those values and performs runtime bounds validation before allocation.
            IF LEFT$(member_desc, 1) = "@" THEN
                IF prep_prefix = "" THEN
                    Give_Error "Invalid TYPE member array metadata"
                    EXIT SUB
                END IF

                acc = acc + cr + "ptrszint dyn_udt_total=1;"
                acc = acc + cr + "ptrszint dyn_udt_stride=1;"
                acc = acc + cr + "ptrszint dyn_udt_lower=0;"
                acc = acc + cr + "ptrszint dyn_udt_upper=0;"

                FOR dim_idx = 1 TO dims_count
                    IF ParseNextUDTRunDescDim(member_desc, desc_pos, lower_expr, upper_expr) = 0 THEN
                        Give_Error "Invalid TYPE member array metadata"
                        EXIT SUB
                    END IF

                    lower_text = prep_prefix + "_m" + LTRIM$(STR$(member_id)) + "_d" + LTRIM$(STR$(dim_idx)) + "_lo"
                    upper_text = prep_prefix + "_m" + LTRIM$(STR$(member_id)) + "_d" + LTRIM$(STR$(dim_idx)) + "_hi"

                    desc_idx = (dims_count - dim_idx) * 4 + 4
                    acc = acc + cr + "dyn_udt_lower=" + lower_text + ";"
                    acc = acc + cr + "dyn_udt_upper=" + upper_text + ";"
                    acc = acc + cr + "if (dyn_udt_upper<dyn_udt_lower) error(5);"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx)) + "]=dyn_udt_lower;"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx + 1)) + "]=dyn_udt_upper-dyn_udt_lower+1;"
                    acc = acc + cr + "if (" + desc_var + "[" + LTRIM$(STR$(desc_idx + 1)) + "]<=0) error(5);"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx + 2)) + "]=dyn_udt_stride;"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx + 3)) + "]=0;"
                    acc = acc + cr + "dyn_udt_total*=" + desc_var + "[" + LTRIM$(STR$(desc_idx + 1)) + "];"
                    acc = acc + cr + "dyn_udt_stride*=" + desc_var + "[" + LTRIM$(STR$(desc_idx + 1)) + "];"
                NEXT

                acc = acc + cr + desc_var + "[0]=(ptrszint)calloc((size_t)(dyn_udt_total*" + LTRIM$(STR$(elem_bytes)) + "),1);"
                acc = acc + cr + "if (!" + desc_var + "[0]) error(257);"
                total_expr = "dyn_udt_total"
            ELSE
                ' Compile-time descriptor metadata is stored as lower/count pairs.
                ' Count_total is checked against udtearrayelements to catch corrupt
                ' or mismatched TYPE metadata before C code is emitted.
                FOR dim_idx = 1 TO dims_count
                    IF ParseNextUDTArrayDescriptorDim(member_desc, desc_pos, lower_val, count_val) = 0 THEN
                        Give_Error "Invalid TYPE member array metadata"
                        EXIT SUB
                    END IF
                    desc_idx = (dims_count - dim_idx) * 4 + 4
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx)) + "]=" + LTRIM$(STR$(lower_val)) + ";"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx + 1)) + "]=" + LTRIM$(STR$(count_val)) + ";"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx + 2)) + "]=" + LTRIM$(STR$(stride_val)) + ";"
                    acc = acc + cr + desc_var + "[" + LTRIM$(STR$(desc_idx + 3)) + "]=0;"
                    stride_val = stride_val * count_val
                    count_total = count_total * count_val
                NEXT

                IF count_total <> udtearrayelements(member_id) THEN
                    Give_Error "Invalid TYPE member array metadata"
                    EXIT SUB
                END IF

                acc = acc + cr + desc_var + "[0]=(ptrszint)calloc((size_t)(" + LTRIM$(STR$(count_total)) + "*" + LTRIM$(STR$(elem_bytes)) + "),1);"
                acc = acc + cr + "if (!" + desc_var + "[0]) error(257);"
                total_expr = LTRIM$(STR$(count_total))
            END IF

            IF DynMemVarStr%(member_id) THEN
                AppendDynStrInit "(void*)" + desc_var + "[0]", total_expr, elem_bytes, acc
            END IF

            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF DynMemUDTVarStr%(member_id) OR UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    elem_idx = "dyn_udt_own_m" + LTRIM$(STR$(member_id))
                    acc = acc + cr + "for(ptrszint " + elem_idx + "=0;" + elem_idx + "<" + total_expr + ";" + elem_idx + "++){"
                    AppendDynUDTOwnInitAt "(void*)" + desc_var + "[0]", nested_udt, 0, LTRIM$(STR$(elem_bytes)), elem_idx, acc, prep_prefix, layout_mode
                    IF Error_Happened THEN EXIT SUB
                    acc = acc + cr + "}"
                END IF
            END IF

            acc = acc + cr + "}"
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    inline_bytes = UDTDynInlineElemBytes&(member_id, layout_mode)
                    FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                        array_offset = member_offset + array_idx * inline_bytes
                        AppendDynUDTDescInitAt data_expr, nested_udt, array_offset, bytesperelement, index_expr, acc, prep_prefix, layout_mode
                        IF Error_Happened THEN EXIT SUB
                    NEXT
                END IF
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            AppendDynUDTDescInitAt data_expr, udtetype(member_id) AND 511, member_offset, bytesperelement, index_expr, acc, prep_prefix, layout_mode
            IF Error_Happened THEN EXIT SUB
        END IF

        member_id = udtenext(member_id)
    LOOP
END SUB

' Parse one runtime-bound dimension from an @ descriptor string. The parser keeps
' nested parentheses balanced so expressions like f(a, b) do not split at the
' wrong comma or semicolon.
FUNCTION ParseNextUDTRunDescDim& (descriptor AS STRING, descriptor_position AS LONG, lower_expr AS STRING, upper_expr AS STRING)
    DIM start_at AS LONG
    DIM scan_i AS LONG
    DIM desc_chars AS LONG
    DIM depth_count AS LONG
    DIM semi_at AS LONG
    DIM comma_at AS LONG
    DIM dim_text AS STRING
    DIM ch AS STRING

    IF descriptor_position <= 0 THEN descriptor_position = 1
    IF LEFT$(descriptor, 1) = "@" AND descriptor_position = 1 THEN descriptor_position = 2

    desc_chars = LEN(descriptor)
    IF descriptor_position > desc_chars THEN EXIT FUNCTION

    start_at = descriptor_position
    depth_count = 0
    semi_at = 0
    FOR scan_i = start_at TO desc_chars
        ch = MID$(descriptor, scan_i, 1)
        IF ch = "(" THEN depth_count = depth_count + 1
        IF ch = ")" THEN depth_count = depth_count - 1
        IF depth_count < 0 THEN EXIT FUNCTION
        IF depth_count = 0 AND ch = ";" THEN
            semi_at = scan_i
            EXIT FOR
        END IF
    NEXT
    IF depth_count <> 0 THEN EXIT FUNCTION

    IF semi_at THEN
        dim_text = MID$(descriptor, start_at, semi_at - start_at)
        descriptor_position = semi_at + 1
    ELSE
        dim_text = MID$(descriptor, start_at)
        descriptor_position = desc_chars + 1
    END IF

    dim_text = LTRIM$(RTRIM$(dim_text))
    IF dim_text = "" THEN EXIT FUNCTION

    depth_count = 0
    comma_at = 0
    FOR scan_i = 1 TO LEN(dim_text)
        ch = MID$(dim_text, scan_i, 1)
        IF ch = "(" THEN depth_count = depth_count + 1
        IF ch = ")" THEN depth_count = depth_count - 1
        IF depth_count < 0 THEN EXIT FUNCTION
        IF depth_count = 0 AND ch = "," THEN
            IF comma_at THEN EXIT FUNCTION
            comma_at = scan_i
        END IF
    NEXT
    IF depth_count <> 0 THEN EXIT FUNCTION
    IF comma_at <= 1 OR comma_at >= LEN(dim_text) THEN EXIT FUNCTION

    lower_expr = LTRIM$(RTRIM$(LEFT$(dim_text, comma_at - 1)))
    upper_expr = LTRIM$(RTRIM$(MID$(dim_text, comma_at + 1)))
    IF lower_expr = "" OR upper_expr = "" THEN EXIT FUNCTION

    ParseNextUDTRunDescDim& = -1
END FUNCTION

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
    DIM member_id AS LONG
    DIM member_offset AS LONG
    DIM member_slot AS STRING
    DIM elem_bytes AS LONG
    DIM nested_udt AS LONG
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM inline_bytes AS LONG

    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_offset = root_offset + UDTDynMemberOffset&(member_id, layout_mode) \ 8

        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            member_slot = "((uint8*)(" + data_expr + ")+" + bytesperelement + "*(" + index_expr + ")+" + LTRIM$(STR$(member_offset)) + ")"
            elem_bytes = udt_dyn_array_elem_bytes(member_id, layout_mode)
            nested_udt = 0
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN nested_udt = udtetype(member_id) AND 511
            AppendDynMemberEraseEx member_slot, elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    inline_bytes = UDTDynInlineElemBytes&(member_id, layout_mode)
                    FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                        array_offset = member_offset + array_idx * inline_bytes
                        AppendDynUDTDescFreeAt data_expr, nested_udt, array_offset, bytesperelement, index_expr, acc, layout_mode
                        IF Error_Happened THEN EXIT SUB
                    NEXT
                END IF
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            AppendDynUDTDescFreeAt data_expr, udtetype(member_id) AND 511, member_offset, bytesperelement, index_expr, acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        END IF

        member_id = udtenext(member_id)
    LOOP
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
        ' Clone into zero-filled owner elements. AppendDynUDTOwnSetAt now creates
        ' scalar qbs* slots lazily, so the clone path no longer has to initialize
        ' every destination element and then immediately erase its descriptors.
        acc = acc + cr + "if (" + pfx + "src[0] && " + pfx + "src[0]!=(ptrszint)nothingvalue && " + pfx + "total){"
        acc = acc + cr + "for(ptrszint " + pfx + "own_i=0; " + pfx + "own_i<" + pfx + "total; " + pfx + "own_i++){"
        AppendDynUDTOwnSetAt "(void*)" + pfx + "dst[0]", "(void*)" + pfx + "src[0]", elem_udt, 0, 0, LTRIM$(STR$(elem_bytes)), pfx + "own_i", pfx + "own_i", acc, layout_mode
        IF Error_Happened THEN EXIT SUB
        acc = acc + cr + "}"
        acc = acc + cr + "}else{"
        acc = acc + cr + "for(ptrszint " + pfx + "own_i=0; " + pfx + "own_i<" + pfx + "total; " + pfx + "own_i++){"
        AppendDynUDTOwnInitAt "(void*)" + pfx + "dst[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), pfx + "own_i", acc, "", layout_mode
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

' Generate C code to clone all descriptor-backed members inside one UDT element.
' Scalar bytes are deliberately not copied here; callers pair this with the
' appropriate scalar/owner copy path for the surrounding layout.
SUB AppendDynUDTDescCloneAfterRaw (src_base AS STRING, dst_base AS STRING, udt_index AS LONG, root_offset AS LONG, bytesperelement AS STRING, src_index AS STRING, dst_index AS STRING, acc AS STRING, layout_mode AS LONG)
    DIM member_id AS LONG
    DIM member_offset AS LONG
    DIM elem_bytes AS LONG
    DIM src_slot AS STRING
    DIM dst_slot AS STRING
    DIM nested_udt AS LONG
    DIM array_idx AS LONG
    DIM array_offset AS LONG
    DIM inline_bytes AS LONG

    member_id = udtxnext(udt_index)
    DO WHILE member_id
        member_offset = root_offset + UDTDynMemberOffset&(member_id, layout_mode) \ 8

        IF UDTMemberDynDesc%(member_id, layout_mode) THEN
            elem_bytes = udt_dyn_array_elem_bytes(member_id, layout_mode)
            nested_udt = 0
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN nested_udt = udtetype(member_id) AND 511
            src_slot = "((uint8*)" + src_base + "+" + bytesperelement + "*(" + src_index + ")+" + LTRIM$(STR$(member_offset)) + ")"
            dst_slot = "((uint8*)" + dst_base + "+" + bytesperelement + "*(" + dst_index + ")+" + LTRIM$(STR$(member_offset)) + ")"
            AppendDynMemberCloneAfterRaw src_slot, dst_slot, elem_bytes, nested_udt, DynMemVarStr%(member_id), acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        ELSEIF udtearrayelements(member_id) THEN
            IF (udtetype(member_id) AND ISUDT) <> 0 THEN
                nested_udt = udtetype(member_id) AND 511
                IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
                    inline_bytes = UDTDynInlineElemBytes&(member_id, layout_mode)
                    FOR array_idx = 0 TO udtearrayelements(member_id) - 1
                        array_offset = member_offset + array_idx * inline_bytes
                        AppendDynUDTDescCloneAfterRaw src_base, dst_base, nested_udt, array_offset, bytesperelement, src_index, dst_index, acc, layout_mode
                        IF Error_Happened THEN EXIT SUB
                    NEXT
                END IF
            END IF
        ELSEIF udtetype(member_id) AND ISUDT THEN
            AppendDynUDTDescCloneAfterRaw src_base, dst_base, udtetype(member_id) AND 511, member_offset, bytesperelement, src_index, dst_index, acc, layout_mode
            IF Error_Happened THEN EXIT SUB
        END IF

        member_id = udtenext(member_id)
    LOOP
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
        AppendDynUDTOwnInitAt "(void*)dyn_udt_new[0]", elem_udt, 0, LTRIM$(STR$(elem_bytes)), "dyn_udt_own_init_i", acc, prep_prefix, layout_mode
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

' Element size for descriptor payloads. UDT elements use the selected dynamic
' layout size because nested descriptor/owner fields are stored by that layout,
' not by the legacy inline udtesize value.
FUNCTION udt_dyn_array_elem_bytes& (element, layout_mode AS LONG)
    IF udtearrayelements(element) = 0 THEN EXIT FUNCTION
    IF (udtetype(element) AND ISUDT) <> 0 THEN
        udt_dyn_array_elem_bytes& = UDTDynLayoutSize&(udtetype(element) AND 511, layout_mode) \ 8
    ELSE
        udt_dyn_array_elem_bytes& = (udtesize(element) \ 8) \ udtearrayelements(element)
    END IF
END FUNCTION

' Element stride for inline member arrays that contain nested UDTs. Plain inline
' arrays keep their legacy stride; nested UDTs with descriptor-backed members use
' the selected dynamic layout stride so recursive offsets remain valid.
FUNCTION UDTDynInlineElemBytes& (element, layout_mode AS LONG)
    DIM nested_udt AS LONG

    IF udtearrayelements(element) = 0 THEN EXIT FUNCTION
    IF (udtetype(element) AND ISUDT) <> 0 THEN
        nested_udt = udtetype(element) AND 511
        IF UDTDynHasMemberArrays%(nested_udt, layout_mode) THEN
            UDTDynInlineElemBytes& = UDTDynLayoutSize&(nested_udt, layout_mode) \ 8
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
    IF NOT udtxvariable(udt) THEN EXIT SUB
    element = udtxnext(udt)
    offset = base_offset
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            FOR array_i = 0 TO udtearrayelements(element) - 1
                array_offset = offset + array_i * elem_bytes
                IF udtetype(element) AND ISSTRING THEN
                    IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                        WriteBufLine buf, "*(qbs**)(((char*)" + n$ + ")+" + STR$(array_offset) + ") = qbs_new(0,0);"
                    END IF
                ELSEIF udtetype(element) AND ISUDT THEN
                    initialise_udt_varstrings n$, udtetype(element) AND 511, buf, array_offset
                END IF
            NEXT
        ELSEIF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                WriteBufLine buf, "*(qbs**)(((char*)" + n$ + ")+" + STR$(offset) + ") = qbs_new(0,0);"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            initialise_udt_varstrings n$, udtetype(element) AND 511, buf, offset
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB free_udt_varstrings (n$, udt, buf, base_offset)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    element = udtxnext(udt)
    offset = base_offset
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            FOR array_i = 0 TO udtearrayelements(element) - 1
                array_offset = offset + array_i * elem_bytes
                IF udtetype(element) AND ISSTRING THEN
                    IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                        WriteBufLine buf, "qbs_free(*((qbs**)(((char*)" + n$ + ")+" + STR$(array_offset) + ")));"
                    END IF
                ELSEIF udtetype(element) AND ISUDT THEN
                    free_udt_varstrings n$, udtetype(element) AND 511, buf, array_offset
                END IF
            NEXT
        ELSEIF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                WriteBufLine buf, "qbs_free(*((qbs**)(((char*)" + n$ + ")+" + STR$(offset) + ")));"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            free_udt_varstrings n$, udtetype(element) AND 511, buf, offset
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB clear_udt_with_varstrings (n$, udt, buf, base_offset)
    ' Clear one UDT value member-by-member. This covers numeric members, fixed-length strings,
    ' variable-length strings, nested UDTs, and inline static member arrays.
    element = udtxnext(udt)
    offset = base_offset
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            IF udtetype(element) AND ISSTRING THEN
                IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                    FOR array_i = 0 TO udtearrayelements(element) - 1
                        array_offset = offset + array_i * elem_bytes
                        WriteBufLine buf, "(*(qbs**)(((char*)" + n$ + ")+" + STR$(array_offset) + "))->len=0;"
                    NEXT
                ELSE
                    WriteBufLine buf, "memset((char*)" + n$ + "+" + STR$(offset) + ",0," + STR$(udtesize(element) \ 8) + ");"
                END IF
            ELSEIF udtetype(element) AND ISUDT THEN
                FOR array_i = 0 TO udtearrayelements(element) - 1
                    array_offset = offset + array_i * elem_bytes
                    clear_udt_with_varstrings n$, udtetype(element) AND 511, buf, array_offset
                NEXT
            ELSE
                WriteBufLine buf, "memset((char*)" + n$ + "+" + STR$(offset) + ",0," + STR$(udtesize(element) \ 8) + ");"
            END IF
        ELSEIF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                WriteBufLine buf, "(*(qbs**)(((char*)" + n$ + ")+" + STR$(offset) + "))->len=0;"
            ELSE
                WriteBufLine buf, "memset((char*)" + n$ + "+" + STR$(offset) + ",0," + STR$(udtesize(element) \ 8) + ");"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            clear_udt_with_varstrings n$, udtetype(element) AND 511, buf, offset
        ELSE
            WriteBufLine buf, "memset((char*)" + n$ + "+" + STR$(offset) + ",0," + STR$(udtesize(element) \ 8) + ");"
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB clear_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    ' Build the code that clears one element of a static or dynamic array-of-UDT without destroying
    ' the array container itself. This covers nested numeric members, fixed-length strings,
    ' variable-length strings, nested UDTs, and inline static member arrays.
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            IF udtetype(element) AND ISSTRING THEN
                IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                    FOR array_i = 0 TO udtearrayelements(element) - 1
                        array_offset = offset + array_i * elem_bytes
                        acc$ = acc$ + CHR$(13) + CHR$(10) + "(*(qbs**)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(array_offset) + "))->len=0;"
                    NEXT
                ELSE
                    acc$ = acc$ + CHR$(13) + CHR$(10) + "memset((void*)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + "),0," + STR$(udtesize(element) \ 8) + ");"
                END IF
            ELSEIF udtetype(element) AND ISUDT THEN
                FOR array_i = 0 TO udtearrayelements(element) - 1
                    array_offset = offset + array_i * elem_bytes
                    clear_array_udt_varstrings n$, udtetype(element) AND 511, array_offset, bytesperelement$, acc$
                NEXT
            ELSE
                acc$ = acc$ + CHR$(13) + CHR$(10) + "memset((void*)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + "),0," + STR$(udtesize(element) \ 8) + ");"
            END IF
        ELSEIF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                acc$ = acc$ + CHR$(13) + CHR$(10) + "(*(qbs**)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + "))->len=0;"
            ELSE
                acc$ = acc$ + CHR$(13) + CHR$(10) + "memset((void*)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + "),0," + STR$(udtesize(element) \ 8) + ");"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            clear_array_udt_varstrings n$, udtetype(element) AND 511, offset, bytesperelement$, acc$
        ELSE
            acc$ = acc$ + CHR$(13) + CHR$(10) + "memset((void*)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + "),0," + STR$(udtesize(element) \ 8) + ");"
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB initialise_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    IF NOT udtxvariable(udt) THEN EXIT SUB
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            FOR array_i = 0 TO udtearrayelements(element) - 1
                array_offset = offset + array_i * elem_bytes
                IF udtetype(element) AND ISSTRING THEN
                    IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                        acc$ = acc$ + CHR$(13) + CHR$(10) + "*(qbs**)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(array_offset) + ")=qbs_new(0,0);"
                    END IF
                ELSEIF udtetype(element) AND ISUDT THEN
                    initialise_array_udt_varstrings n$, udtetype(element) AND 511, array_offset, bytesperelement$, acc$
                END IF
            NEXT
        ELSEIF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                acc$ = acc$ + CHR$(13) + CHR$(10) + "*(qbs**)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + ")=qbs_new(0,0);"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            initialise_array_udt_varstrings n$, udtetype(element) AND 511, offset, bytesperelement$, acc$
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB free_array_udt_varstrings (n$, udt, base_offset, bytesperelement$, acc$)
    ' Build the code that frees nested variable members for one array element before the raw storage
    ' block of the surrounding array is released. This mirrors clear_array_udt_varstrings(), but uses
    ' qbs_free on variable-length strings because the enclosing array container is going away entirely.
    IF NOT udtxvariable(udt) THEN EXIT SUB
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            FOR array_i = 0 TO udtearrayelements(element) - 1
                array_offset = offset + array_i * elem_bytes
                IF udtetype(element) AND ISSTRING THEN
                    IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                        acc$ = acc$ + CHR$(13) + CHR$(10) + "qbs_free(*(qbs**)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(array_offset) + "));"
                    END IF
                ELSEIF udtetype(element) AND ISUDT THEN
                    free_array_udt_varstrings n$, udtetype(element) AND 511, array_offset, bytesperelement$, acc$
                END IF
            NEXT
        ELSEIF udtetype(element) AND ISSTRING THEN
            IF (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                acc$ = acc$ + CHR$(13) + CHR$(10) + "qbs_free(*(qbs**)(" + n$ + "[0]+" + bytesperelement$ + "*tmp_long+" + STR$(offset) + "));"
            END IF
        ELSEIF udtetype(element) AND ISUDT THEN
            free_array_udt_varstrings n$, udtetype(element) AND 511, offset, bytesperelement$, acc$
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB

SUB copy_full_udt (dst$, src$, buf, base_offset, udt)
    IF NOT udtxvariable(udt) THEN
        WriteBufLine buf, "memcpy(" + dst$ + "+" + STR$(base_offset) + "," + src$ + "+" + STR$(base_offset) + "," + STR$(udtxsize(udt) \ 8) + ");"
        EXIT SUB
    END IF
    offset = base_offset
    element = udtxnext(udt)
    DO WHILE element
        IF udtearrayelements(element) THEN
            elem_bytes = udt_array_member_bytes(element)
            IF ((udtetype(element) AND ISSTRING) > 0) AND (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
                FOR array_i = 0 TO udtearrayelements(element) - 1
                    array_offset = offset + array_i * elem_bytes
                    WriteBufLine buf, "qbs_set(*(qbs**)(" + dst$ + "+" + STR$(array_offset) + "), *(qbs**)(" + src$ + "+" + STR$(array_offset) + "));"
                NEXT
            ELSEIF ((udtetype(element) AND ISUDT) > 0) THEN
                IF udtxvariable(udtetype(element) AND 511) THEN
                    FOR array_i = 0 TO udtearrayelements(element) - 1
                        array_offset = offset + array_i * elem_bytes
                        copy_full_udt dst$, src$, buf, array_offset, udtetype(element) AND 511
                    NEXT
                ELSE
                    WriteBufLine buf, "memcpy((" + dst$ + "+" + STR$(offset) + "),(" + src$ + "+" + STR$(offset) + ")," + STR$(udtesize(element) \ 8) + ");"
                END IF
            ELSE
                WriteBufLine buf, "memcpy((" + dst$ + "+" + STR$(offset) + "),(" + src$ + "+" + STR$(offset) + ")," + STR$(udtesize(element) \ 8) + ");"
            END IF
        ELSEIF ((udtetype(element) AND ISSTRING) > 0) AND (udtetype(element) AND ISFIXEDLENGTH) = 0 THEN
            WriteBufLine buf, "qbs_set(*(qbs**)(" + dst$ + "+" + STR$(offset) + "), *(qbs**)(" + src$ + "+" + STR$(offset) + "));"
        ELSEIF ((udtetype(element) AND ISUDT) > 0) THEN
            copy_full_udt dst$, src$, buf, offset, udtetype(element) AND 511
        ELSE
            WriteBufLine buf, "memcpy((" + dst$ + "+" + STR$(offset) + "),(" + src$ + "+" + STR$(offset) + ")," + STR$(udtesize(element) \ 8) + ");"
        END IF
        offset = offset + udtesize(element) \ 8
        element = udtenext(element)
    LOOP
END SUB


' Copy one UDT value using the selected dynamic layout. Owner layouts require a
' deep owner-aware assignment; pure descriptor layouts copy scalar fields first
' and then clone descriptor slots so source and destination do not share payload.
SUB copy_full_udt_dyn (dst$, src$, buf, base_offset, udt, layout_mode AS LONG)
    dyn_acc$ = ""

    IF udtxvariable(udt) THEN
        ' Owner-layout UDTs contain qbs* scalar slots and/or descriptor-owned
        ' members. Never raw-copy their scalar area first: doing so aliases qbs*
        ' owners and can leak or double-free the previous destination strings.
        ' AppendDynUDTOwnSetAt performs the full ownership-aware assignment:
        ' qbs_set for scalar strings, descriptor erase/clone for _DynamicField
        ' members, and recursive handling for nested owner UDTs.
        AppendDynUDTOwnSetAt dst$, src$, udt, base_offset, base_offset, LTRIM$(STR$(UDTDynLayoutSize&(udt, layout_mode) \ 8)), "0", "0", dyn_acc$, layout_mode
        IF Error_Happened THEN EXIT SUB
        IF dyn_acc$ <> "" THEN WriteBufLine buf, dyn_acc$
        EXIT SUB
    END IF

    copy_dyn_udt_scalars dst$, src$, buf, base_offset, udt, layout_mode
    IF Error_Happened THEN EXIT SUB

    IF UDTDynHasMemberArrays%(udt, layout_mode) THEN
        AppendDynUDTDescCopy dst$, src$, udt, LTRIM$(STR$(base_offset)), LTRIM$(STR$(base_offset)), LTRIM$(STR$(UDTDynLayoutSize&(udt, layout_mode) \ 8)), dyn_acc$, layout_mode
        IF Error_Happened THEN EXIT SUB
        IF dyn_acc$ <> "" THEN WriteBufLine buf, dyn_acc$
    END IF
END SUB

' Copy only the scalar/inline part of a dynamic-layout UDT. Descriptor pointer
' slots are skipped because they must be cloned, not memcpy'd, to avoid shared
' descriptors and double-free hazards.
SUB copy_dyn_udt_scalars (dst$, src$, buf, base_offset, udt, layout_mode AS LONG)
    copyoff& = base_offset
    member_id& = udtxnext(udt)
    DO WHILE member_id&
        IF UDTMemberDynDesc%(member_id&, layout_mode) THEN
            'Descriptor slots are handled by AppendDynUDTDescCopy(). Do not memcpy them.
        ELSEIF udtearrayelements(member_id&) THEN
            IF (udtetype(member_id&) AND ISUDT) <> 0 THEN
                nested_udt& = udtetype(member_id&) AND 511
                IF udtxvariable(nested_udt&) OR UDTDynHasMemberArrays%(nested_udt&, layout_mode) THEN
                    inline_bytes& = UDTDynInlineElemBytes&(member_id&, layout_mode)
                    FOR arr_idx& = 0 TO udtearrayelements(member_id&) - 1
                        arr_off& = copyoff& + arr_idx& * inline_bytes&
                        IF udtxvariable(nested_udt&) THEN
                            static_copy$ = ""
                            AppendDynUDTOwnSetAt dst$, src$, nested_udt&, arr_off&, arr_off&, LTRIM$(STR$(inline_bytes&)), "0", "0", static_copy$, layout_mode
                            IF Error_Happened THEN EXIT SUB
                            IF static_copy$ <> "" THEN WriteBufLine buf, static_copy$
                        ELSE
                            copy_dyn_udt_scalars dst$, src$, buf, arr_off&, nested_udt&, layout_mode
                            IF Error_Happened THEN EXIT SUB
                            static_copy$ = ""
                            AppendDynUDTDescCopy dst$, src$, nested_udt&, LTRIM$(STR$(arr_off&)), LTRIM$(STR$(arr_off&)), LTRIM$(STR$(inline_bytes&)), static_copy$, layout_mode
                            IF Error_Happened THEN EXIT SUB
                            IF static_copy$ <> "" THEN WriteBufLine buf, static_copy$
                        END IF
                    NEXT
                ELSE
                    WriteBufLine buf, "memcpy((" + dst$ + "+" + LTRIM$(STR$(copyoff&)) + "),(" + src$ + "+" + LTRIM$(STR$(copyoff&)) + ")," + LTRIM$(STR$(UDTDynMemberSize&(member_id&, layout_mode) \ 8)) + ");"
                END IF
            ELSE
                WriteBufLine buf, "memcpy((" + dst$ + "+" + LTRIM$(STR$(copyoff&)) + "),(" + src$ + "+" + LTRIM$(STR$(copyoff&)) + ")," + LTRIM$(STR$(UDTDynMemberSize&(member_id&, layout_mode) \ 8)) + ");"
            END IF
        ELSEIF ((udtetype(member_id&) AND ISSTRING) <> 0) AND ((udtetype(member_id&) AND ISFIXEDLENGTH) = 0) THEN
            WriteBufLine buf, "qbs_set(*(qbs**)(" + dst$ + "+" + LTRIM$(STR$(copyoff&)) + "), *(qbs**)(" + src$ + "+" + LTRIM$(STR$(copyoff&)) + "));"
        ELSEIF (udtetype(member_id&) AND ISUDT) <> 0 THEN
            nested_udt& = udtetype(member_id&) AND 511
            IF udtxvariable(nested_udt&) THEN
                static_copy$ = ""
                AppendDynUDTOwnSetAt dst$, src$, nested_udt&, copyoff&, copyoff&, LTRIM$(STR$(UDTDynMemberSize&(member_id&, layout_mode) \ 8)), "0", "0", static_copy$, layout_mode
                IF Error_Happened THEN EXIT SUB
                IF static_copy$ <> "" THEN WriteBufLine buf, static_copy$
            ELSEIF UDTDynHasMemberArrays%(nested_udt&, layout_mode) THEN
                copy_dyn_udt_scalars dst$, src$, buf, copyoff&, nested_udt&, layout_mode
                IF Error_Happened THEN EXIT SUB
            ELSE
                WriteBufLine buf, "memcpy((" + dst$ + "+" + LTRIM$(STR$(copyoff&)) + "),(" + src$ + "+" + LTRIM$(STR$(copyoff&)) + ")," + LTRIM$(STR$(UDTDynMemberSize&(member_id&, layout_mode) \ 8)) + ");"
            END IF
        ELSE
            WriteBufLine buf, "memcpy((" + dst$ + "+" + LTRIM$(STR$(copyoff&)) + "),(" + src$ + "+" + LTRIM$(STR$(copyoff&)) + ")," + LTRIM$(STR$(UDTDynMemberSize&(member_id&, layout_mode) \ 8)) + ");"
        END IF
        copyoff& = copyoff& + (UDTDynMemberSize&(member_id&, layout_mode) \ 8)
        member_id& = udtenext(member_id&)
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
    Type_GetSizeInBits = typeId AND 511
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

