#ifndef INCLUDE_LIBQB_QBS_H
#define INCLUDE_LIBQB_QBS_H

#include <stdint.h>

// QB64 string descriptor structure
struct qbs_field {
    int32_t fileno;
    int64_t fileid;
    int64_t size;
    int64_t offset;
};

struct qbs {
    uint8_t *chr;    // a 32 bit pointer to the string's data
    int32_t len;     // must be signed for comparisons against signed int32s

    uint8_t in_cmem; // set to 1 if in the conventional memory DBLOCK
    uint16_t *cmem_descriptor;
    uint16_t cmem_descriptor_offset;

    uint32_t listi;    // the index in the list of strings that references it

    uint8_t tmp;       // set to 1 if the string can be deleted immediately after being processed
    uint32_t tmplisti; // the index in the list of strings that references it

    uint8_t fixed;     // fixed length string
    uint8_t readonly;  // set to 1 if string is read only
                       //
    qbs_field *field;
};

qbs *qbs_new(int32_t, uint8_t);
qbs *qbs_new_txt(const char *);
qbs *qbs_new_cmem(int32_t size, uint8_t tmp);
qbs *qbs_new_txt_len(const char *txt, int32_t len);
qbs *qbs_new_fixed(uint8_t *offset, uint32_t size, uint8_t tmp);
qbs *qbs_add(qbs *, qbs *);
qbs *qbs_set(qbs *, qbs *);

// Called by vWatch
void set_qbs_size(intptr_t *target_qbs, int32_t newlength);

void qbs_free(qbs *str);

qbs *qbs_str(int64_t value);
qbs *qbs_str(int32_t value);
qbs *qbs_str(int16_t value);
qbs *qbs_str(int8_t value);
qbs *qbs_str(uint64_t value);
qbs *qbs_str(uint32_t value);
qbs *qbs_str(uint16_t value);
qbs *qbs_str(uint8_t value);
qbs *qbs_str(float value);
qbs *qbs_str(double value);
qbs *qbs_str(long double value);

qbs *func_chr(int32_t value);

qbs *qbs_ucase(qbs *str);
qbs *qbs_lcase(qbs *str);
qbs *qbs_left(qbs *str, int32_t l);
qbs *qbs_right(qbs *str, int32_t l);

int32_t qbs_equal(qbs *str1, qbs *str2);
int32_t qbs_notequal(qbs *str1, qbs *str2);
int32_t qbs_greaterthan(qbs *str2, qbs *str1);
int32_t qbs_lessthan(qbs *str1, qbs *str2);
int32_t qbs_lessorequal(qbs *str1, qbs *str2);
int32_t qbs_greaterorequal(qbs *str2, qbs *str1);

int32_t qbs_asc(qbs *str, uint32_t i);
int32_t qbs_asc(qbs *str);
int32_t qbs_len(qbs *str);

// FIXME: Usages of these outside of qbx.c (and qbs_cleanup()) need to be removed.
extern intptr_t *qbs_tmp_list;
extern uint32_t qbs_tmp_list_lasti;
extern uint32_t qbs_tmp_list_nexti;

template <typename T> static T qbs_cleanup(uint32_t base, T passvalue) {

    while (qbs_tmp_list_nexti > base) {
        qbs_tmp_list_nexti--;
        if (qbs_tmp_list[qbs_tmp_list_nexti] != -1)
            qbs_free((qbs *)qbs_tmp_list[qbs_tmp_list_nexti]);
    } // clear any temp. strings created

    return passvalue;
}

void sub_lset(qbs *dest, qbs *source);
void sub_rset(qbs *dest, qbs *source);
qbs *func_space(int32_t spaces);
qbs *func_string(int32_t characters, int32_t asciivalue);
int32_t func_instr(int32_t start, qbs *str, qbs *substr, int32_t passed);
int32_t func__instrrev(int32_t start, qbs *str, qbs *substr, int32_t passed);
void sub_mid(qbs *dest, int32_t start, int32_t l, qbs *src, int32_t passed);
qbs *func_mid(qbs *str, int32_t start, int32_t l, int32_t passed);
qbs *qbs_ltrim(qbs *str);
qbs *qbs_rtrim(qbs *str);
qbs *qbs__trim(qbs *str);
int32_t func__str_nc_compare(qbs *s1, qbs *s2);
int32_t func__str_compare(qbs *s1, qbs *s2);

#endif
