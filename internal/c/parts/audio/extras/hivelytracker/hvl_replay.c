/*
** Changes for the 1.4 release are commented. You can do
** a search for "1.4" and merge them into your own replay
** code.
**
** Changes for 1.5 are marked also.
**
** ... as are those for 1.6
**
** ... and for 1.8
*/

#include "hvl_replay.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHITENOISELEN (0x280 * 3)

#define WO_LOWPASSES 0
#define WO_TRIANGLE_04 (WO_LOWPASSES + ((0xfc + 0xfc + 0x80 * 0x1f + 0x80 + 3 * 0x280) * 31))
#define WO_TRIANGLE_08 (WO_TRIANGLE_04 + 0x04)
#define WO_TRIANGLE_10 (WO_TRIANGLE_08 + 0x08)
#define WO_TRIANGLE_20 (WO_TRIANGLE_10 + 0x10)
#define WO_TRIANGLE_40 (WO_TRIANGLE_20 + 0x20)
#define WO_TRIANGLE_80 (WO_TRIANGLE_40 + 0x40)
#define WO_SAWTOOTH_04 (WO_TRIANGLE_80 + 0x80)
#define WO_SAWTOOTH_08 (WO_SAWTOOTH_04 + 0x04)
#define WO_SAWTOOTH_10 (WO_SAWTOOTH_08 + 0x08)
#define WO_SAWTOOTH_20 (WO_SAWTOOTH_10 + 0x10)
#define WO_SAWTOOTH_40 (WO_SAWTOOTH_20 + 0x20)
#define WO_SAWTOOTH_80 (WO_SAWTOOTH_40 + 0x40)
#define WO_SQUARES (WO_SAWTOOTH_80 + 0x80)
#define WO_WHITENOISE (WO_SQUARES + (0x80 * 0x20))
#define WO_HIGHPASSES (WO_WHITENOISE + WHITENOISELEN)
#define WAVES_SIZE (WO_HIGHPASSES + ((0xfc + 0xfc + 0x80 * 0x1f + 0x80 + 3 * 0x280) * 31))

const uint16 lentab[45] = {3,    7,    0xf,  0x1f, 0x3f, 0x7f, 3,    7,    0xf,  0x1f, 0x3f, 0x7f, 0x7f, 0x7f, 0x7f,
                           0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
                           0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, (0x280 * 3) - 1};

const int16 vib_tab[64] = {0,    24,   49,   74,   97,   120,  141,  161,  180,  197,  212,  224,  235,  244,  250,  253,  255,  253,  250,  244,  235,  224,
                           212,  197,  180,  161,  141,  120,  97,   74,   49,   24,   0,    -24,  -49,  -74,  -97,  -120, -141, -161, -180, -197, -212, -224,
                           -235, -244, -250, -253, -255, -253, -250, -244, -235, -224, -212, -197, -180, -161, -141, -120, -97,  -74,  -49,  -24};

const uint16 period_tab[61] = {0x0000, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970, 0x08E8, 0x0868, 0x07F0, 0x0780, 0x0714, 0x06B0, 0x0650, 0x05F4,
                               0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0, 0x038A, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C,
                               0x023A, 0x021A, 0x01FC, 0x01E0, 0x01C5, 0x01AC, 0x0194, 0x017D, 0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
                               0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097, 0x008F, 0x0087, 0x007F, 0x0078, 0x0071};

const int32 stereopan_left[5] = {128, 96, 64, 32, 0};
const int32 stereopan_right[5] = {128, 160, 193, 225, 255};

const int16 filter_thing[2790] = {
    -1161,  -4413,  -7161,  -13094, 635,    13255,  2189,   6401,   9041,   16130,  13460,  5360,   6349,   12699,  19049,  25398,  30464,  32512,  32512,
    32515,  31625,  29756,  27158,  24060,  20667,  17156,  13970,  11375,  9263,   7543,   6142,   5002,   4074,   3318,   2702,   2178,   1755,   1415,
    1141,   909,    716,    563,    444,    331,    -665,   -2082,  -6170,  -9235,  -13622, 12545,  9617,   3951,   8345,   11246,  18486,  6917,   3848,
    8635,   17271,  25907,  32163,  32512,  32455,  30734,  27424,  23137,  18397,  13869,  10429,  7843,   5897,   4435,   3335,   2507,   1885,   1389,
    1023,   720,    530,    353,    260,    173,    96,     32,     -18,    -55,    -79,    -92,    -95,    -838,   -3229,  -7298,  -12386, -7107,  13946,
    6501,   5970,   9133,   14947,  16881,  6081,   3048,   10921,  21843,  31371,  32512,  32068,  28864,  23686,  17672,  12233,  8469,   5862,   4058,
    2809,   1944,   1346,   900,    601,    371,    223,    137,    64,     7,      -34,    -58,    -69,    -70,    -63,    -52,    -39,    -26,    -14,
    -5,     4984,   -4476,  -8102,  -14892, 2894,   12723,  4883,   8010,   9750,   17887,  11790,  5099,   2520,   13207,  26415,  32512,  32457,  28690,
    22093,  14665,  9312,   5913,   3754,   2384,   1513,   911,    548,    330,    143,    3,      -86,    -130,   -139,   -125,   -97,    -65,    -35,
    -11,    6,      15,     19,     19,     16,     12,     8,      6877,   -5755,  -9129,  -15709, 9705,   10893,  4157,   9882,   10897,  19236,  8153,
    4285,   2149,   15493,  30618,  32512,  30220,  22942,  14203,  8241,   4781,   2774,   1609,   933,    501,    220,    81,     35,     2,      -18,
    -26,    -25,    -20,    -13,    -7,     -1,     2,      4,      4,      3,      2,      1,      0,      0,      -1,     2431,   -6956,  -10698, -14594,
    12720,  8980,   3714,   10892,  12622,  19554,  6915,   3745,   1872,   17779,  32512,  32622,  26286,  16302,  8605,   4542,   2397,   1265,   599,
    283,    45,     -92,    -141,   -131,   -93,    -49,    -14,    8,      18,     18,     14,     8,      3,      0,      -2,     -3,     -2,     -2,
    -1,     0,      0,      -3654,  -8008,  -12743, -11088, 13625,  7342,   3330,   11330,  14859,  18769,  6484,   3319,   1660,   20065,  32512,  30699,
    21108,  10616,  5075,   2425,   1159,   477,    196,    1,      -93,    -109,   -82,    -44,    -12,    7,      14,     13,     9,      4,      0,
    -2,     -2,     -1,     -1,     0,      0,      0,      0,      0,      0,      -7765,  -8867,  -14957, -5862,  13550,  6139,   2988,   11284,  17054,
    16602,  6017,   2979,   1489,   22351,  32512,  28083,  15576,  6708,   2888,   1243,   535,    188,    32,     -47,    -64,    -47,    -22,    -3,
    7,      8,      5,      3,      0,      -1,     -1,     -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      -9079,  -9532,
    -16960, -335,   13001,  5333,   2704,   11192,  18742,  13697,  5457,   2703,   1351,   24637,  32512,  24556,  10851,  4185,   1614,   622,    184,
    15,     -57,    -59,    -34,    -9,     5,      8,      6,      2,      0,      -1,     -1,     0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      -8576,  -10043, -18551, 4372,   12190,  4809,   2472,   11230,  19803,  11170,  4953,   2473,   1236,   26923,
    32512,  20567,  7430,   2550,   875,    212,    51,     -30,    -43,    -25,    -6,     3,      5,      3,      1,      0,      -1,     0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      -6960,  -10485, -19740, 7864,   11223,  4449,   2279,
    11623,  20380,  9488,   4553,   2280,   1140,   29209,  31829,  16235,  4924,   1493,   452,    86,     -7,     -32,    -20,    -5,     2,      3,
    2,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    -4739,  -10974, -19831, 10240,  10190,  4169,   2114,   12524,  20649,  8531,   4226,   2114,   1057,   31495,  29672,  11916,  3168,   841,    121,
    17,     -22,    -18,    -5,     2,      2,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      -2333,  -11641, -19288, 11765,  9175,   3923,   1971,   13889,  20646,  8007,   3942,   1971,
    985,    32512,  27426,  8446,   1949,   449,    45,     -11,    -16,    -5,     1,      1,      1,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      29,     -12616, -17971, 12690,  8247,
    3693,   1846,   15662,  20271,  7658,   3692,   1846,   923,    32512,  25132,  6284,   1245,   246,    -71,    -78,    -17,    8,      7,      1,
    -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      2232,   -14001, -15234, 13198,  7447,   3478,   1736,   17409,  19411,  7332,   3472,   1736,   868,    32512,  22545,  4352,   731,
    18,     -117,   -40,    8,      9,      2,      -1,     -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      4197,   -15836, -11480, 13408,  6791,   3281,   1639,   19224,  18074,  6978,
    3276,   1639,   819,    32512,  19657,  2706,   380,    -148,   -86,    2,      13,     3,      -2,     0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      5863,   -17878, -9460,
    13389,  6270,   3104,   1551,   20996,  16431,  6616,   3102,   1551,   776,    32512,  16633,  1921,   221,    -95,    -39,    5,      5,      0,
    -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      7180,   -20270, -6194,  13181,  5866,   2946,   1473,   22548,  14746,  6273,   2946,   1473,   737,    32512,  13621,
    1263,   116,    -53,    -15,    4,      2,      -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      8117,   -21129, -2795,  12809,  5550,   2804,   1402,   23717,
    13326,  5962,   2804,   1402,   701,    32512,  10687,  776,    -56,    -56,    4,      4,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      8560,
    -19953, 508,    12299,  5295,   2675,   1337,   25109,  12263,  5684,   2675,   1338,   669,    32512,  7905,   433,    -36,    -22,    3,      1,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      8488,   -18731, 3672,   11679,  5080,   2558,   1279,   26855,  11480,  5434,   2557,   1279,   639,
    32512,  5357,   212,    -95,    0,      4,      -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      7977,   -24055, 6537,   10986,  4883,   2450,
    1225,   28611,  10918,  5206,   2450,   1225,   612,    32512,  3131,   83,     -35,    2,      1,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      7088,   -30584, 9054,   10265,  4696,   2351,   1176,   28707,  10494,  4996,   2351,   1175,   588,    32512,  1920,   -155,   -13,    4,
    -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      5952,   -32627, 11249,  9564,   4519,   2260,   1130,   28678,  10113,  4803,   2260,
    1130,   565,    32512,  1059,   -73,    -1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      4629,   -32753, 13199,  8934,
    4351,   2175,   1088,   28446,  9775,   4623,   2175,   1087,   544,    32512,  434,    -22,    1,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      3132,   -32768, 15225,  8430,   4194,   2097,   1049,   30732,  9439,   4456,   2097,   1049,   524,    32512,  75,     -6,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1345,   -32768, 16765,  8107,   4048,   2025,   1012,   32512,  9112,
    4302,   2025,   1012,   506,    32385,  392,    5,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      -706,   -32768,
    17879,  8005,   3913,   1956,   978,    32512,  8843,   4157,   1957,   978,    489,    31184,  1671,   122,    10,     0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      -3050,  -32768, 18923,  8163,   3799,   1893,   946,    32512,  8613,   4022,   1893,   945,    473,    29903,
    3074,   316,    52,     11,     3,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      -5812,  -32768, 19851,  8626,   3739,   1833,   917,
    32512,  7982,   3889,   1833,   916,    459,    28541,  4567,   731,    206,    66,     23,     8,      1,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    -9235,  -32768, 20587,  9408,   3841,   1784,   889,    32512,  6486,   3688,   1776,   889,    447,    27099,  6112,   1379,   313,    135,    65,
    33,     17,     7,      4,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,
    2,      2,      2,      2,      2,      2,      2,      -12713, 1188,   1318,   -1178,  -4304,  -26320, -14931, -1716,  -1486,  2494,   3611,   22275,
    27450,  -31839, -29668, -26258, -21608, -15880, -9560,  -3211,  3138,   9369,   15281,  20717,  25571,  29774,  32512,  32512,  32512,  32512,  32512,
    32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32748,  32600,  32750,  32566,  32659,  32730,  8886,   1762,   506,    -1665,  -12112,
    -24641, -8513,  -2224,  247,    3288,   9926,   25787,  28909,  -31048, -27034, -20726, -12532, -3896,  4733,   13043,  20568,  27010,  32215,  32512,
    32512,  32512,  32512,  32512,  32512,  32512,  32762,  32696,  32647,  32512,  32665,  32512,  32587,  32638,  32669,  32681,  32679,  32667,  32648,
    32624,  32598,  6183,   2141,   -630,   -2674,  -21856, -18306, -5711,  -2161,  2207,   4247,   17616,  26475,  29719,  -30017, -23596, -13741, -2819,
    8029,   18049,  26470,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32738,  32663,  32612,  32756,  32549,  32602,  32629,  32636,  32628,
    32610,  32588,  32564,  32542,  32524,  32510,  32500,  32494,  32492,  3604,   2248,   -1495,  -5612,  -26800, -13545, -4745,  -1390,  3443,   6973,
    23495,  27724,  30246,  -28745, -19355, -6335,  6861,   19001,  28690,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32667,  32743,
    32757,  32730,  32681,  32624,  32572,  32529,  32500,  32482,  32476,  32477,  32482,  32489,  32497,  32504,  32509,  32513,  7977,   1975,   -1861,
    -9752,  -25893, -10150, -4241,  86,     4190,   10643,  25235,  28481,  30618,  -27231, -14398, 1096,   15982,  27872,  32512,  32512,  32512,  32512,
    32512,  32734,  32631,  32767,  32531,  32553,  32557,  32551,  32539,  32527,  32516,  32509,  32505,  32504,  32505,  32506,  32508,  32510,  32511,
    32512,  32512,  32512,  32511,  14529,  1389,   -2028,  -14813, -22765, -7845,  -3774,  1986,   4706,   14562,  25541,  29019,  30894,  -25476, -9294,
    8516,   23979,  32512,  32512,  32512,  32512,  32512,  32512,  32708,  32762,  32727,  32654,  32579,  32522,  32490,  32478,  32480,  32488,  32498,
    32507,  32512,  32515,  32515,  32514,  32513,  32512,  32510,  32510,  32510,  32510,  17663,  557,    -2504,  -19988, -19501, -6436,  -3340,  4135,
    5461,   18788,  26016,  29448,  31107,  -23481, -4160,  15347,  30045,  32512,  32512,  32512,  32512,  32512,  32674,  32700,  32654,  32586,  32531,
    32498,  32486,  32488,  32496,  32504,  32510,  32513,  32514,  32513,  32512,  32511,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  16286,
    -402,   -3522,  -23951, -16641, -5631,  -2983,  6251,   6837,   22781,  26712,  29788,  31277,  -21244, 1108,   21806,  32512,  32512,  32512,  32512,
    32695,  32576,  32622,  32600,  32557,  32520,  32501,  32496,  32500,  32505,  32509,  32512,  32512,  32512,  32511,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  13436,  -1351,  -4793,  -25948, -14224, -5151,  -2702,  7687,   8805,   25705,  27348,  30064,  31415,
    -18766, 5872,   26652,  32512,  32512,  32512,  32747,  32581,  32620,  32586,  32540,  32508,  32497,  32499,  32505,  32510,  32512,  32512,  32512,
    32511,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  10427,  -2162,  -7136,  -26147, -12195, -4810,
    -2474,  8723,   11098,  27251,  27832,  30293,  31530,  -16047, 10877,  30990,  32512,  32512,  32512,  32512,  32584,  32571,  32536,  32511,  32502,
    32503,  32507,  32510,  32512,  32512,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  7797,   -2748,  -10188, -25174, -10519, -4515,  -2281,  9397,   13473,  27937,  28213,  30487,  31627,  -13087, 15816,  32512,  32512,  32512,
    32715,  32550,  32560,  32534,  32512,  32505,  32506,  32508,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  5840,   -3084,  -13327, -23617, -9177,  -4231,  -2116,  9892,   15843,  28292,  28538,
    30652,  31710,  -9886,  20235,  32512,  32512,  32512,  32512,  32550,  32534,  32514,  32507,  32507,  32510,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  4592,   -3215,  -15898, -21856,
    -8141,  -3958,  -1972,  10401,  18229,  28612,  28824,  30796,  31781,  -7103,  24037,  32512,  32512,  32745,  32535,  32534,  32517,  32508,  32508,
    32509,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  3964,   -3262,  -18721, -20087, -7368,  -3705,  -1847,  11014,  20634,  28996,  29075,  30920,  31843,  -4732,  27243,  32512,
    32512,  32648,  32627,  32530,  32495,  32500,  32510,  32512,  32512,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  3858,   -3404,  -21965, -18398, -6801,  -3479,  -1738,  12009,  22960,
    29429,  29294,  31030,  31898,  -2281,  30194,  32512,  32512,  32699,  32569,  32496,  32496,  32509,  32513,  32512,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  4177,   -3869,
    -24180, -16820, -6380,  -3280,  -1640,  13235,  25035,  29863,  29490,  31128,  31947,  251,    32758,  32512,  32749,  32652,  32508,  32490,  32507,
    32513,  32512,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  4837,   -4913,  -26436, -15364, -6056,  -3103,  -1553,  14759,  26704,  30256,  29664,  31215,  31991,  2863,
    32512,  32512,  32657,  32580,  32503,  32501,  32510,  32512,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  5755,   -6290,  -27702, -14036, -5788,  -2947,  -1474,
    16549,  27912,  30602,  29821,  31294,  32030,  5555,   32512,  32512,  32592,  32541,  32505,  32507,  32511,  32511,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    6898,   -8911,  -27788, -12841, -5550,  -2805,  -1403,  18509,  28687,  30906,  29963,  31364,  32066,  8328,   32512,  32512,  32623,  32511,  32502,
    32510,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  8107,   -11465, -27077, -11789, -5325,  -2676,  -1339,  19833,  29213,  31179,  30092,  31429,
    32098,  11181,  32512,  32512,  32561,  32508,  32508,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  9247,   -13203, -25808, -10886, -5109,
    -2559,  -1280,  21060,  29636,  31428,  30209,  31488,  32127,  14114,  32512,  32681,  32529,  32502,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  10252,  -16863, -24251, -10137, -4902,  -2451,  -1226,  21937,  30022,  31656,  30317,  31542,  32154,  17128,  32512,  32581,  32514,
    32508,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  11032,  -22427, -22598, -9535,  -4705,  -2353,  -1177,  20999,  30406,  31867,
    30415,  31591,  32179,  20222,  32512,  32591,  32501,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  11539,  -19778, -20962,
    -9060,  -4522,  -2261,  -1131,  19486,  30789,  32061,  30507,  31637,  32201,  23396,  32512,  32535,  32508,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  11803,  -12759, -19353, -8690,  -4353,  -2177,  -1089,  18499,  31165,  32240,  30591,  31678,  32222,  26651,  32512,
    32514,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  11826,  -7586,  -17510, -8384,  -4196,  -2099,  -1050,  26861,
    31521,  32406,  30669,  31718,  32241,  29986,  32585,  32510,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,
    32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  32511,  11599,
    -2848,  -15807, -8097,  -4051,  -2025,  -1014,  30693,  31850,  32561,  30743,  31755,  32261,  32512,  32524,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  11037,  -5302,  -14051, -7770,  -3913,  -1958,  -980,   28033,  32165,  32705,  30810,  31789,  32278,
    32512,  32729,  32536,  32513,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  10114,  -7837,  -12293, -7348,  -3782,  -1894,
    -948,   24926,  32473,  32512,  30873,  31819,  32294,  32512,  32512,  32580,  32527,  32515,  32512,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  8759,   -10456, -10591, -6766,  -3638,  -1835,  -917,   24058,  32600,  32512,  30934,  31850,  32309,  32512,  32512,  32729,  32591,  32537,
    32520,  32514,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,
    32510,  32510,  32510,  32510,  32510,  32510,  32510,  32510,  6811,   -13156, -9045,  -5965,  -3421,  -1776,  -890,   31582,  32246,  32512,  30988,
    31878,  32324,  32512,  32512,  32512,  32628,  32573,  32541,  32526,  32518,  32514,  32513,  32512,  32512,  32512,  32512,  32512,  32512,  32512,
    32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  32512,  4835};

int8 waves[WAVES_SIZE];
uint32 panning_left[256], panning_right[256];

void hvl_GenPanningTables(void) {
    uint32 i;
    float64 aa, ab;

    // Sine based panning table
    aa = (3.14159265f * 2.0f) / 4.0f; // Quarter of the way through the sinewave == top peak
    ab = 0.0f;                        // Start of the climb from zero

    for (i = 0; i < 256; i++) {
        panning_left[i] = (uint32)(sin(aa) * 255.0f);
        panning_right[i] = (uint32)(sin(ab) * 255.0f);

        aa += (3.14159265 * 2.0f / 4.0f) / 256.0f;
        ab += (3.14159265 * 2.0f / 4.0f) / 256.0f;
    }
    panning_left[255] = 0;
    panning_right[0] = 0;
}

void hvl_GenSawtooth(int8 *buf, uint32 len) {
    uint32 i;
    int32 val, add;

    add = 256 / (len - 1);
    val = -128;

    for (i = 0; i < len; i++, val += add)
        *buf++ = (int8)val;
}

void hvl_GenTriangle(int8 *buf, uint32 len) {
    uint32 i;
    int32 d2, d5, d1, d4;
    int32 val;
    int8 *buf2;

    d2 = len;
    d5 = len >> 2;
    d1 = 128 / d5;
    d4 = -(d2 >> 1);
    val = 0;

    for (i = 0; i < d5; i++) {
        *buf++ = val;
        val += d1;
    }
    *buf++ = 0x7f;

    if (d5 != 1) {
        val = 128;
        for (i = 0; i < d5 - 1; i++) {
            val -= d1;
            *buf++ = val;
        }
    }

    buf2 = buf + d4;
    for (i = 0; i < d5 * 2; i++) {
        int8 c;

        c = *buf2++;
        if (c == 0x7f)
            c = 0x80;
        else
            c = -c;

        *buf++ = c;
    }
}

void hvl_GenSquare(int8 *buf) {
    uint32 i, j;

    for (i = 1; i <= 0x20; i++) {
        for (j = 0; j < (0x40 - i) * 2; j++)
            *buf++ = 0x80;
        for (j = 0; j < i * 2; j++)
            *buf++ = 0x7f;
    }
}

static inline int32 clipshifted8(int32 in) {
    int16 top = (int16)(in >> 16);
    if (top > 127)
        in = 127 << 16;
    else if (top < -128)
        in = -(128 << 16);
    return in;
}

void hvl_GenFilterWaves(const int8 *buf, int8 *lowbuf, int8 *highbuf) {

    const int16 *mid_table = &filter_thing[0];
    const int16 *low_table = &filter_thing[1395];

    int32 freq;
    int32 i;

    for (i = 0, freq = 25; i < 31; i++, freq += 9) {
        uint32 wv;
        const int8 *a0 = buf;

        for (wv = 0; wv < 6 + 6 + 0x20 + 1; wv++) {
            int32 in, fre, high, mid, low;
            uint32 j;

            mid = *mid_table++ << 8;
            low = *low_table++ << 8;

            for (j = 0; j <= lentab[wv]; j++) {
                in = a0[j] << 16;
                high = clipshifted8(in - mid - low);
                fre = (high >> 8) * freq;
                mid = clipshifted8(mid + fre);
                fre = (mid >> 8) * freq;
                low = clipshifted8(low + fre);
                *highbuf++ = high >> 16;
                *lowbuf++ = low >> 16;
            }
            a0 += lentab[wv] + 1;
        }
    }
}

void hvl_GenWhiteNoise(int8 *buf, uint32 len) {
    uint32 ays;

    ays = 0x41595321;

    do {
        uint16 ax, bx;
        int8 s;

        s = ays;

        if (ays & 0x100) {
            s = 0x7f;

            if (ays & 0x8000)
                s = 0x80;
        }

        *buf++ = s;
        len--;

        ays = (ays >> 5) | (ays << 27);
        ays = (ays & 0xffffff00) | ((ays & 0xff) ^ 0x9a);
        bx = ays;
        ays = (ays << 2) | (ays >> 30);
        ax = ays;
        bx += ax;
        ax ^= bx;
        ays = (ays & 0xffff0000) | ax;
        ays = (ays >> 3) | (ays << 29);
    } while (len);
}

void hvl_GenTables(void) {
    hvl_GenPanningTables();
    hvl_GenSawtooth(&waves[WO_SAWTOOTH_04], 0x04);
    hvl_GenSawtooth(&waves[WO_SAWTOOTH_08], 0x08);
    hvl_GenSawtooth(&waves[WO_SAWTOOTH_10], 0x10);
    hvl_GenSawtooth(&waves[WO_SAWTOOTH_20], 0x20);
    hvl_GenSawtooth(&waves[WO_SAWTOOTH_40], 0x40);
    hvl_GenSawtooth(&waves[WO_SAWTOOTH_80], 0x80);
    hvl_GenTriangle(&waves[WO_TRIANGLE_04], 0x04);
    hvl_GenTriangle(&waves[WO_TRIANGLE_08], 0x08);
    hvl_GenTriangle(&waves[WO_TRIANGLE_10], 0x10);
    hvl_GenTriangle(&waves[WO_TRIANGLE_20], 0x20);
    hvl_GenTriangle(&waves[WO_TRIANGLE_40], 0x40);
    hvl_GenTriangle(&waves[WO_TRIANGLE_80], 0x80);
    hvl_GenSquare(&waves[WO_SQUARES]);
    hvl_GenWhiteNoise(&waves[WO_WHITENOISE], WHITENOISELEN);
    hvl_GenFilterWaves(&waves[WO_TRIANGLE_04], &waves[WO_LOWPASSES], &waves[WO_HIGHPASSES]);
}

/*
** Waves
*/

void hvl_reset_some_stuff(struct hvl_tune *ht) {
    uint32 i;

    for (i = 0; i < MAX_CHANNELS; i++) {
        ht->ht_Voices[i].vc_Delta = 1;
        ht->ht_Voices[i].vc_OverrideTranspose = 1000; // 1.5
        ht->ht_Voices[i].vc_SamplePos = ht->ht_Voices[i].vc_Track = ht->ht_Voices[i].vc_Transpose = ht->ht_Voices[i].vc_NextTrack =
            ht->ht_Voices[i].vc_NextTranspose = 0;
        ht->ht_Voices[i].vc_ADSRVolume = ht->ht_Voices[i].vc_InstrPeriod = ht->ht_Voices[i].vc_TrackPeriod = ht->ht_Voices[i].vc_VibratoPeriod =
            ht->ht_Voices[i].vc_NoteMaxVolume = ht->ht_Voices[i].vc_PerfSubVolume = ht->ht_Voices[i].vc_TrackMasterVolume = 0;
        ht->ht_Voices[i].vc_NewWaveform = ht->ht_Voices[i].vc_Waveform = ht->ht_Voices[i].vc_PlantSquare = ht->ht_Voices[i].vc_PlantPeriod =
            ht->ht_Voices[i].vc_IgnoreSquare = 0;
        ht->ht_Voices[i].vc_TrackOn = ht->ht_Voices[i].vc_FixedNote = ht->ht_Voices[i].vc_VolumeSlideUp = ht->ht_Voices[i].vc_VolumeSlideDown =
            ht->ht_Voices[i].vc_HardCut = ht->ht_Voices[i].vc_HardCutRelease = ht->ht_Voices[i].vc_HardCutReleaseF = 0;
        ht->ht_Voices[i].vc_PeriodSlideSpeed = ht->ht_Voices[i].vc_PeriodSlidePeriod = ht->ht_Voices[i].vc_PeriodSlideLimit =
            ht->ht_Voices[i].vc_PeriodSlideOn = ht->ht_Voices[i].vc_PeriodSlideWithLimit = 0;
        ht->ht_Voices[i].vc_PeriodPerfSlideSpeed = ht->ht_Voices[i].vc_PeriodPerfSlidePeriod = ht->ht_Voices[i].vc_PeriodPerfSlideOn =
            ht->ht_Voices[i].vc_VibratoDelay = ht->ht_Voices[i].vc_VibratoCurrent = ht->ht_Voices[i].vc_VibratoDepth = ht->ht_Voices[i].vc_VibratoSpeed = 0;
        ht->ht_Voices[i].vc_SquareOn = ht->ht_Voices[i].vc_SquareInit = ht->ht_Voices[i].vc_SquareLowerLimit = ht->ht_Voices[i].vc_SquareUpperLimit =
            ht->ht_Voices[i].vc_SquarePos = ht->ht_Voices[i].vc_SquareSign = ht->ht_Voices[i].vc_SquareSlidingIn = ht->ht_Voices[i].vc_SquareReverse = 0;
        ht->ht_Voices[i].vc_FilterOn = ht->ht_Voices[i].vc_FilterInit = ht->ht_Voices[i].vc_FilterLowerLimit = ht->ht_Voices[i].vc_FilterUpperLimit =
            ht->ht_Voices[i].vc_FilterPos = ht->ht_Voices[i].vc_FilterSign = ht->ht_Voices[i].vc_FilterSpeed = ht->ht_Voices[i].vc_FilterSlidingIn =
                ht->ht_Voices[i].vc_IgnoreFilter = 0;
        ht->ht_Voices[i].vc_PerfCurrent = ht->ht_Voices[i].vc_PerfSpeed = ht->ht_Voices[i].vc_WaveLength = ht->ht_Voices[i].vc_NoteDelayOn =
            ht->ht_Voices[i].vc_NoteCutOn = 0;
        ht->ht_Voices[i].vc_AudioPeriod = ht->ht_Voices[i].vc_AudioVolume = ht->ht_Voices[i].vc_VoiceVolume = ht->ht_Voices[i].vc_VoicePeriod =
            ht->ht_Voices[i].vc_VoiceNum = ht->ht_Voices[i].vc_WNRandom = 0;
        ht->ht_Voices[i].vc_SquareWait = ht->ht_Voices[i].vc_FilterWait = ht->ht_Voices[i].vc_PerfWait = ht->ht_Voices[i].vc_NoteDelayWait =
            ht->ht_Voices[i].vc_NoteCutWait = 0;
        ht->ht_Voices[i].vc_PerfList = 0;
        ht->ht_Voices[i].vc_RingSamplePos = ht->ht_Voices[i].vc_RingDelta = ht->ht_Voices[i].vc_RingPlantPeriod = ht->ht_Voices[i].vc_RingAudioPeriod =
            ht->ht_Voices[i].vc_RingNewWaveform = ht->ht_Voices[i].vc_RingWaveform = ht->ht_Voices[i].vc_RingFixedPeriod = ht->ht_Voices[i].vc_RingBasePeriod =
                0;

        ht->ht_Voices[i].vc_RingMixSource = NULL;
        ht->ht_Voices[i].vc_RingAudioSource = NULL;

        memset(&ht->ht_Voices[i].vc_SquareTempBuffer, 0, 0x80);
        memset(&ht->ht_Voices[i].vc_ADSR, 0, sizeof(struct hvl_envelope));
        memset(&ht->ht_Voices[i].vc_VoiceBuffer, 0, 0x281);
        memset(&ht->ht_Voices[i].vc_RingVoiceBuffer, 0, 0x281);
    }

    for (i = 0; i < MAX_CHANNELS; i++) {
        ht->ht_Voices[i].vc_WNRandom = 0x280;
        ht->ht_Voices[i].vc_VoiceNum = i;
        ht->ht_Voices[i].vc_TrackMasterVolume = 0x40;
        ht->ht_Voices[i].vc_TrackOn = 1;
        ht->ht_Voices[i].vc_MixSource = ht->ht_Voices[i].vc_VoiceBuffer;
    }
}

BOOL hvl_InitSubsong(struct hvl_tune *ht, uint32 nr) {
    uint32 PosNr, i;

    if (nr > ht->ht_SubsongNr)
        return FALSE;

    ht->ht_SongNum = nr;

    PosNr = 0;
    if (nr)
        PosNr = ht->ht_Subsongs[nr - 1];

    ht->ht_PosNr = PosNr;
    ht->ht_PosJump = 0;
    ht->ht_PatternBreak = 0;
    ht->ht_NoteNr = 0;
    ht->ht_PosJumpNote = 0;
    ht->ht_Tempo = 6;
    ht->ht_StepWaitFrames = 0;
    ht->ht_GetNewPosition = 1;
    ht->ht_SongEndReached = 0;
    ht->ht_PlayingTime = 0;

    for (i = 0; i < MAX_CHANNELS; i += 4) {
        ht->ht_Voices[i + 0].vc_Pan = ht->ht_defpanleft;
        ht->ht_Voices[i + 0].vc_SetPan = ht->ht_defpanleft; // 1.4
        ht->ht_Voices[i + 0].vc_PanMultLeft = panning_left[ht->ht_defpanleft];
        ht->ht_Voices[i + 0].vc_PanMultRight = panning_right[ht->ht_defpanleft];
        ht->ht_Voices[i + 1].vc_Pan = ht->ht_defpanright;
        ht->ht_Voices[i + 1].vc_SetPan = ht->ht_defpanright; // 1.4
        ht->ht_Voices[i + 1].vc_PanMultLeft = panning_left[ht->ht_defpanright];
        ht->ht_Voices[i + 1].vc_PanMultRight = panning_right[ht->ht_defpanright];
        ht->ht_Voices[i + 2].vc_Pan = ht->ht_defpanright;
        ht->ht_Voices[i + 2].vc_SetPan = ht->ht_defpanright; // 1.4
        ht->ht_Voices[i + 2].vc_PanMultLeft = panning_left[ht->ht_defpanright];
        ht->ht_Voices[i + 2].vc_PanMultRight = panning_right[ht->ht_defpanright];
        ht->ht_Voices[i + 3].vc_Pan = ht->ht_defpanleft;
        ht->ht_Voices[i + 3].vc_SetPan = ht->ht_defpanleft; // 1.4
        ht->ht_Voices[i + 3].vc_PanMultLeft = panning_left[ht->ht_defpanleft];
        ht->ht_Voices[i + 3].vc_PanMultRight = panning_right[ht->ht_defpanleft];
    }

    hvl_reset_some_stuff(ht);

    return TRUE;
}

void hvl_InitReplayer(void) { hvl_GenTables(); }

struct hvl_tune *hvl_load_ahx(const uint8 *buf, uint32 buflen, uint32 defstereo, uint32 freq) {
    const uint8 *bptr;
    const TEXT *nptr;
    uint32 i, j, k, l, posn, insn, ssn, hs, trkn, trkl;
    struct hvl_tune *ht;
    struct hvl_plsentry *ple;
    const int32 defgain[] = {71, 72, 76, 85, 100};

    posn = ((buf[6] & 0x0f) << 8) | buf[7];
    insn = buf[12];
    ssn = buf[13];
    trkl = buf[10];
    trkn = buf[11];

    hs = sizeof(struct hvl_tune);
    hs += sizeof(struct hvl_position) * posn;
    hs += sizeof(struct hvl_instrument) * (insn + 1);
    hs += sizeof(uint16) * ssn;

    // Calculate the size of all instrument PList buffers
    bptr = &buf[14];
    bptr += ssn * 2;      // Skip past the subsong list
    bptr += posn * 4 * 2; // Skip past the positions
    bptr += trkn * trkl * 3;
    if ((buf[6] & 0x80) == 0)
        bptr += trkl * 3;

    // *NOW* we can finally calculate PList space
    for (i = 1; i <= insn; i++) {
        hs += bptr[21] * sizeof(struct hvl_plsentry);
        bptr += 22 + bptr[21] * 4;
    }

    ht = malloc(hs);
    if (!ht) {
        // printf("Out of memory!\n");
        return NULL;
    }

    ht->ht_Frequency = freq;
    ht->ht_FreqF = (float64)freq;

    ht->ht_Positions = (struct hvl_position *)(&ht[1]);
    ht->ht_Instruments = (struct hvl_instrument *)(&ht->ht_Positions[posn]);
    ht->ht_Subsongs = (uint16 *)(&ht->ht_Instruments[(insn + 1)]);
    ple = (struct hvl_plsentry *)(&ht->ht_Subsongs[ssn]);

    ht->ht_WaveformTab[0] = &waves[WO_TRIANGLE_04];
    ht->ht_WaveformTab[1] = &waves[WO_SAWTOOTH_04];
    ht->ht_WaveformTab[3] = &waves[WO_WHITENOISE];

    ht->ht_Channels = 4;
    ht->ht_PositionNr = posn;
    ht->ht_Restart = (buf[8] << 8) | buf[9];
    ht->ht_SpeedMultiplier = ((buf[6] >> 5) & 3) + 1;
    ht->ht_TrackLength = trkl;
    ht->ht_TrackNr = trkn;
    ht->ht_InstrumentNr = insn;
    ht->ht_SubsongNr = ssn;
    ht->ht_defstereo = defstereo;
    ht->ht_defpanleft = stereopan_left[ht->ht_defstereo];
    ht->ht_defpanright = stereopan_right[ht->ht_defstereo];
    ht->ht_mixgain = (defgain[ht->ht_defstereo] * 256) / 100;

    if (ht->ht_Restart >= ht->ht_PositionNr)
        ht->ht_Restart = ht->ht_PositionNr - 1;

    // Do some validation
    if ((ht->ht_PositionNr > 1000) || (ht->ht_TrackLength > 64) || (ht->ht_InstrumentNr > 64)) {
        // printf("%d,%d,%d\n", ht->ht_PositionNr, ht->ht_TrackLength, ht->ht_InstrumentNr);
        free(ht);
        // printf("Invalid file.\n");
        return NULL;
    }

    strncpy(ht->ht_Name, (TEXT *)&buf[(buf[4] << 8) | buf[5]], 127);
    ht->ht_Name[127] = '\0';
    nptr = (TEXT *)&buf[((buf[4] << 8) | buf[5]) + strlen(ht->ht_Name) + 1];

    bptr = &buf[14];

    // Subsongs
    for (i = 0; i < ht->ht_SubsongNr; i++) {
        ht->ht_Subsongs[i] = (bptr[0] << 8) | bptr[1];
        if (ht->ht_Subsongs[i] >= ht->ht_PositionNr)
            ht->ht_Subsongs[i] = 0;
        bptr += 2;
    }

    // Position list
    for (i = 0; i < ht->ht_PositionNr; i++) {
        for (j = 0; j < 4; j++) {
            ht->ht_Positions[i].pos_Track[j] = *bptr++;
            ht->ht_Positions[i].pos_Transpose[j] = *(int8 *)bptr++;
        }
    }

    // Tracks
    for (i = 0; i <= ht->ht_TrackNr; i++) {
        if (((buf[6] & 0x80) == 0x80) && (i == 0)) {
            for (j = 0; j < ht->ht_TrackLength; j++) {
                ht->ht_Tracks[i][j].stp_Note = 0;
                ht->ht_Tracks[i][j].stp_Instrument = 0;
                ht->ht_Tracks[i][j].stp_FX = 0;
                ht->ht_Tracks[i][j].stp_FXParam = 0;
                ht->ht_Tracks[i][j].stp_FXb = 0;
                ht->ht_Tracks[i][j].stp_FXbParam = 0;
            }
            continue;
        }

        for (j = 0; j < ht->ht_TrackLength; j++) {
            ht->ht_Tracks[i][j].stp_Note = (bptr[0] >> 2) & 0x3f;
            ht->ht_Tracks[i][j].stp_Instrument = ((bptr[0] & 0x3) << 4) | (bptr[1] >> 4);
            ht->ht_Tracks[i][j].stp_FX = bptr[1] & 0xf;
            ht->ht_Tracks[i][j].stp_FXParam = bptr[2];
            ht->ht_Tracks[i][j].stp_FXb = 0;
            ht->ht_Tracks[i][j].stp_FXbParam = 0;
            bptr += 3;
        }
    }

    // Instruments
    for (i = 1; i <= ht->ht_InstrumentNr; i++) {
        if (nptr < (TEXT *)(buf + buflen)) {
            strncpy(ht->ht_Instruments[i].ins_Name, nptr, 127);
            ht->ht_Instruments[i].ins_Name[127] = '\0';
            nptr += strlen(nptr) + 1;
        } else {
            ht->ht_Instruments[i].ins_Name[0] = 0;
        }

        ht->ht_Instruments[i].ins_Volume = bptr[0];
        ht->ht_Instruments[i].ins_FilterSpeed = ((bptr[1] >> 3) & 0x1f) | ((bptr[12] >> 2) & 0x20);
        ht->ht_Instruments[i].ins_WaveLength = bptr[1] & 0x07;

        ht->ht_Instruments[i].ins_Envelope.aFrames = bptr[2];
        ht->ht_Instruments[i].ins_Envelope.aVolume = bptr[3];
        ht->ht_Instruments[i].ins_Envelope.dFrames = bptr[4];
        ht->ht_Instruments[i].ins_Envelope.dVolume = bptr[5];
        ht->ht_Instruments[i].ins_Envelope.sFrames = bptr[6];
        ht->ht_Instruments[i].ins_Envelope.rFrames = bptr[7];
        ht->ht_Instruments[i].ins_Envelope.rVolume = bptr[8];

        ht->ht_Instruments[i].ins_FilterLowerLimit = bptr[12] & 0x7f;
        ht->ht_Instruments[i].ins_VibratoDelay = bptr[13];
        ht->ht_Instruments[i].ins_HardCutReleaseFrames = (bptr[14] >> 4) & 0x07;
        ht->ht_Instruments[i].ins_HardCutRelease = bptr[14] & 0x80 ? 1 : 0;
        ht->ht_Instruments[i].ins_VibratoDepth = bptr[14] & 0x0f;
        ht->ht_Instruments[i].ins_VibratoSpeed = bptr[15];
        ht->ht_Instruments[i].ins_SquareLowerLimit = bptr[16];
        ht->ht_Instruments[i].ins_SquareUpperLimit = bptr[17];
        ht->ht_Instruments[i].ins_SquareSpeed = bptr[18];
        ht->ht_Instruments[i].ins_FilterUpperLimit = bptr[19] & 0x3f;
        ht->ht_Instruments[i].ins_PList.pls_Speed = bptr[20];
        ht->ht_Instruments[i].ins_PList.pls_Length = bptr[21];

        ht->ht_Instruments[i].ins_PList.pls_Entries = ple;
        ple += bptr[21];

        bptr += 22;
        for (j = 0; j < ht->ht_Instruments[i].ins_PList.pls_Length; j++) {
            k = (bptr[0] >> 5) & 7;
            if (k == 6)
                k = 12;
            if (k == 7)
                k = 15;
            l = (bptr[0] >> 2) & 7;
            if (l == 6)
                l = 12;
            if (l == 7)
                l = 15;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[1] = k;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[0] = l;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Waveform = ((bptr[0] << 1) & 6) | (bptr[1] >> 7);
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Fixed = (bptr[1] >> 6) & 1;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Note = bptr[1] & 0x3f;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] = bptr[2];
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] = bptr[3];

            // 1.6: Strip "toggle filter" commands if the module is
            //      version 0 (pre-filters). This is what AHX also does.
            if ((buf[3] == 0) && (l == 4) && ((bptr[2] & 0xf0) != 0))
                ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] &= 0x0f;
            if ((buf[3] == 0) && (k == 4) && ((bptr[3] & 0xf0) != 0))
                ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] &= 0x0f; // 1.8

            bptr += 4;
        }
    }

    hvl_InitSubsong(ht, 0);
    return ht;
}

struct hvl_tune *hvl_load_hvl(const uint8 *buf, uint32 buflen, uint32 defstereo, uint32 freq) {
    const uint8 *bptr;
    const TEXT *nptr;
    uint32 i, j, posn, insn, ssn, chnn, hs, trkl, trkn;
    struct hvl_tune *ht;
    struct hvl_plsentry *ple;

    posn = ((buf[6] & 0x0f) << 8) | buf[7];
    insn = buf[12];
    ssn = buf[13];
    chnn = (buf[8] >> 2) + 4;
    trkl = buf[10];
    trkn = buf[11];

    hs = sizeof(struct hvl_tune);
    hs += sizeof(struct hvl_position) * posn;
    hs += sizeof(struct hvl_instrument) * (insn + 1);
    hs += sizeof(uint16) * ssn;

    // Calculate the size of all instrument PList buffers
    bptr = &buf[16];
    bptr += ssn * 2;         // Skip past the subsong list
    bptr += posn * chnn * 2; // Skip past the positions

    // Skip past the tracks
    // 1.4: Fixed two really stupid bugs that cancelled each other
    //      out if the module had a blank first track (which is how
    //      come they were missed.
    for (i = ((buf[6] & 0x80) == 0x80) ? 1 : 0; i <= trkn; i++)
        for (j = 0; j < trkl; j++) {
            if (bptr[0] == 0x3f) {
                bptr++;
                continue;
            }
            bptr += 5;
        }

    // *NOW* we can finally calculate PList space
    for (i = 1; i <= insn; i++) {
        hs += bptr[21] * sizeof(struct hvl_plsentry);
        bptr += 22 + bptr[21] * 5;
    }

    ht = malloc(hs);
    if (!ht) {
        // printf("Out of memory!\n");
        return NULL;
    }

    ht->ht_Version = buf[3]; // 1.5
    ht->ht_Frequency = freq;
    ht->ht_FreqF = (float64)freq;

    ht->ht_Positions = (struct hvl_position *)(&ht[1]);
    ht->ht_Instruments = (struct hvl_instrument *)(&ht->ht_Positions[posn]);
    ht->ht_Subsongs = (uint16 *)(&ht->ht_Instruments[(insn + 1)]);
    ple = (struct hvl_plsentry *)(&ht->ht_Subsongs[ssn]);

    ht->ht_WaveformTab[0] = &waves[WO_TRIANGLE_04];
    ht->ht_WaveformTab[1] = &waves[WO_SAWTOOTH_04];
    ht->ht_WaveformTab[3] = &waves[WO_WHITENOISE];

    ht->ht_PositionNr = posn;
    ht->ht_Channels = (buf[8] >> 2) + 4;
    ht->ht_Restart = ((buf[8] & 3) << 8) | buf[9];
    ht->ht_SpeedMultiplier = ((buf[6] >> 5) & 3) + 1;
    ht->ht_TrackLength = buf[10];
    ht->ht_TrackNr = buf[11];
    ht->ht_InstrumentNr = insn;
    ht->ht_SubsongNr = ssn;
    ht->ht_mixgain = (buf[14] << 8) / 100;
    ht->ht_defstereo = buf[15];
    ht->ht_defpanleft = stereopan_left[ht->ht_defstereo];
    ht->ht_defpanright = stereopan_right[ht->ht_defstereo];

    if (ht->ht_Restart >= ht->ht_PositionNr)
        ht->ht_Restart = ht->ht_PositionNr - 1;

    // Do some validation
    if ((ht->ht_PositionNr > 1000) || (ht->ht_TrackLength > 64) || (ht->ht_InstrumentNr > 64)) {
        // printf("%d,%d,%d\n", ht->ht_PositionNr, ht->ht_TrackLength, ht->ht_InstrumentNr);
        free(ht);
        // printf("Invalid file.\n");
        return NULL;
    }

    strncpy(ht->ht_Name, (TEXT *)&buf[(buf[4] << 8) | buf[5]], 127);
    ht->ht_Name[127] = '\0';
    nptr = (TEXT *)&buf[((buf[4] << 8) | buf[5]) + strlen(ht->ht_Name) + 1];

    bptr = &buf[16];

    // Subsongs
    for (i = 0; i < ht->ht_SubsongNr; i++) {
        ht->ht_Subsongs[i] = (bptr[0] << 8) | bptr[1];
        bptr += 2;
    }

    // Position list
    for (i = 0; i < ht->ht_PositionNr; i++) {
        for (j = 0; j < ht->ht_Channels; j++) {
            ht->ht_Positions[i].pos_Track[j] = *bptr++;
            ht->ht_Positions[i].pos_Transpose[j] = *(int8 *)bptr++;
        }
    }

    // Tracks
    for (i = 0; i <= ht->ht_TrackNr; i++) {
        if (((buf[6] & 0x80) == 0x80) && (i == 0)) {
            for (j = 0; j < ht->ht_TrackLength; j++) {
                ht->ht_Tracks[i][j].stp_Note = 0;
                ht->ht_Tracks[i][j].stp_Instrument = 0;
                ht->ht_Tracks[i][j].stp_FX = 0;
                ht->ht_Tracks[i][j].stp_FXParam = 0;
                ht->ht_Tracks[i][j].stp_FXb = 0;
                ht->ht_Tracks[i][j].stp_FXbParam = 0;
            }
            continue;
        }

        for (j = 0; j < ht->ht_TrackLength; j++) {
            if (bptr[0] == 0x3f) {
                ht->ht_Tracks[i][j].stp_Note = 0;
                ht->ht_Tracks[i][j].stp_Instrument = 0;
                ht->ht_Tracks[i][j].stp_FX = 0;
                ht->ht_Tracks[i][j].stp_FXParam = 0;
                ht->ht_Tracks[i][j].stp_FXb = 0;
                ht->ht_Tracks[i][j].stp_FXbParam = 0;
                bptr++;
                continue;
            }

            ht->ht_Tracks[i][j].stp_Note = bptr[0];
            ht->ht_Tracks[i][j].stp_Instrument = bptr[1];
            ht->ht_Tracks[i][j].stp_FX = bptr[2] >> 4;
            ht->ht_Tracks[i][j].stp_FXParam = bptr[3];
            ht->ht_Tracks[i][j].stp_FXb = bptr[2] & 0xf;
            ht->ht_Tracks[i][j].stp_FXbParam = bptr[4];
            bptr += 5;
        }
    }

    // Instruments
    for (i = 1; i <= ht->ht_InstrumentNr; i++) {
        if (nptr < (TEXT *)(buf + buflen)) {
            strncpy(ht->ht_Instruments[i].ins_Name, nptr, 127);
            ht->ht_Instruments[i].ins_Name[127] = '\0';
            nptr += strlen(nptr) + 1;
        } else {
            ht->ht_Instruments[i].ins_Name[0] = 0;
        }

        ht->ht_Instruments[i].ins_Volume = bptr[0];
        ht->ht_Instruments[i].ins_FilterSpeed = ((bptr[1] >> 3) & 0x1f) | ((bptr[12] >> 2) & 0x20);
        ht->ht_Instruments[i].ins_WaveLength = bptr[1] & 0x07;

        ht->ht_Instruments[i].ins_Envelope.aFrames = bptr[2];
        ht->ht_Instruments[i].ins_Envelope.aVolume = bptr[3];
        ht->ht_Instruments[i].ins_Envelope.dFrames = bptr[4];
        ht->ht_Instruments[i].ins_Envelope.dVolume = bptr[5];
        ht->ht_Instruments[i].ins_Envelope.sFrames = bptr[6];
        ht->ht_Instruments[i].ins_Envelope.rFrames = bptr[7];
        ht->ht_Instruments[i].ins_Envelope.rVolume = bptr[8];

        ht->ht_Instruments[i].ins_FilterLowerLimit = bptr[12] & 0x7f;
        ht->ht_Instruments[i].ins_VibratoDelay = bptr[13];
        ht->ht_Instruments[i].ins_HardCutReleaseFrames = (bptr[14] >> 4) & 0x07;
        ht->ht_Instruments[i].ins_HardCutRelease = bptr[14] & 0x80 ? 1 : 0;
        ht->ht_Instruments[i].ins_VibratoDepth = bptr[14] & 0x0f;
        ht->ht_Instruments[i].ins_VibratoSpeed = bptr[15];
        ht->ht_Instruments[i].ins_SquareLowerLimit = bptr[16];
        ht->ht_Instruments[i].ins_SquareUpperLimit = bptr[17];
        ht->ht_Instruments[i].ins_SquareSpeed = bptr[18];
        ht->ht_Instruments[i].ins_FilterUpperLimit = bptr[19] & 0x3f;
        ht->ht_Instruments[i].ins_PList.pls_Speed = bptr[20];
        ht->ht_Instruments[i].ins_PList.pls_Length = bptr[21];

        ht->ht_Instruments[i].ins_PList.pls_Entries = ple;
        ple += bptr[21];

        bptr += 22;
        for (j = 0; j < ht->ht_Instruments[i].ins_PList.pls_Length; j++) {
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[0] = bptr[0] & 0xf;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[1] = (bptr[1] >> 3) & 0xf;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Waveform = bptr[1] & 7;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Fixed = (bptr[2] >> 6) & 1;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Note = bptr[2] & 0x3f;
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] = bptr[3];
            ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] = bptr[4];
            bptr += 5;
        }
    }

    hvl_InitSubsong(ht, 0);
    return ht;
}

struct hvl_tune *hvl_ParseTune(const uint8 *buf, uint32 buflen, uint32 freq, uint32 defstereo) {
    struct hvl_tune *ht = NULL;
    if ((buf[0] == 'T') && (buf[1] == 'H') && (buf[2] == 'X') && (buf[3] < 3)) {
        ht = hvl_load_ahx(buf, buflen, defstereo, freq);
    }

    else if ((buf[0] == 'H') && (buf[1] == 'V') && (buf[2] == 'L') && (buf[3] < 2)) {
        ht = hvl_load_hvl(buf, buflen, defstereo, freq);
    } else {
        // printf("Invalid file.\n");
    }
    return ht;
}

struct hvl_tune *hvl_LoadTune(const TEXT *name, uint32 freq, uint32 defstereo) {
    struct hvl_tune *ht = NULL;
    uint8 *buf;
    uint32 buflen;
    FILE *fh;

    fh = fopen(name, "rb");
    if (!fh) {
        // printf("Can't open file\n");
        return NULL;
    }

    fseek(fh, 0, SEEK_END);
    buflen = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    buf = malloc(buflen);
    if (!buf) {
        fclose(fh);
        // printf("Out of memory!\n");
        return NULL;
    }

    if (fread(buf, 1, buflen, fh) != buflen) {
        fclose(fh);
        free(buf);
        // printf("Unable to read from file!\n");
        return NULL;
    }
    fclose(fh);

    ht = hvl_ParseTune(buf, buflen, freq, defstereo);
    free(buf);
    return ht;
}

void hvl_FreeTune(struct hvl_tune *ht) {
    if (!ht)
        return;
    free(ht);
}

void hvl_process_stepfx_1(struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam) {
    switch (FX) {
    case 0x0: // Position Jump HI
        if (((FXParam & 0x0f) > 0) && ((FXParam & 0x0f) <= 9))
            ht->ht_PosJump = FXParam & 0xf;
        break;

    case 0x5: // Volume Slide + Tone Portamento
    case 0xa: // Volume Slide
        voice->vc_VolumeSlideDown = FXParam & 0x0f;
        voice->vc_VolumeSlideUp = FXParam >> 4;
        break;

    case 0x7: // Panning
        if (FXParam > 127)
            FXParam -= 256;
        voice->vc_Pan = (FXParam + 128);
        voice->vc_SetPan = (FXParam + 128); // 1.4
        voice->vc_PanMultLeft = panning_left[voice->vc_Pan];
        voice->vc_PanMultRight = panning_right[voice->vc_Pan];
        break;

    case 0xb: // Position jump
        ht->ht_PosJump = ht->ht_PosJump * 100 + (FXParam & 0x0f) + (FXParam >> 4) * 10;
        ht->ht_PatternBreak = 1;
        if (ht->ht_PosJump <= ht->ht_PosNr)
            ht->ht_SongEndReached = 1;
        break;

    case 0xd: // Pattern break
        ht->ht_PosJump = ht->ht_PosNr + 1;
        ht->ht_PosJumpNote = (FXParam & 0x0f) + (FXParam >> 4) * 10;
        ht->ht_PatternBreak = 1;
        if (ht->ht_PosJumpNote > ht->ht_TrackLength)
            ht->ht_PosJumpNote = 0;
        break;

    case 0xe: // Extended commands
        switch (FXParam >> 4) {
        case 0xc: // Note cut
            if ((FXParam & 0x0f) < ht->ht_Tempo) {
                voice->vc_NoteCutWait = FXParam & 0x0f;
                if (voice->vc_NoteCutWait) {
                    voice->vc_NoteCutOn = 1;
                    voice->vc_HardCutRelease = 0;
                }
            }
            break;

            // 1.6: 0xd case removed
        }
        break;

    case 0xf: // Speed
        ht->ht_Tempo = FXParam;
        if (FXParam == 0)
            ht->ht_SongEndReached = 1;
        break;
    }
}

void hvl_process_stepfx_2(const struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam, int32 *Note) {
    switch (FX) {
    case 0x9: // Set squarewave offset
        voice->vc_SquarePos = FXParam >> (5 - voice->vc_WaveLength);
        //      voice->vc_PlantSquare  = 1;
        voice->vc_IgnoreSquare = 1;
        break;

    case 0x3: // Tone portamento
        if (FXParam != 0)
            voice->vc_PeriodSlideSpeed = FXParam;
    case 0x5: // Tone portamento + volume slide

        if (*Note) {
            int32 new, diff;

            new = period_tab[*Note];
            diff = period_tab[voice->vc_TrackPeriod];
            diff -= new;
            new = diff + voice->vc_PeriodSlidePeriod;

            if (new)
                voice->vc_PeriodSlideLimit = -diff;
        }
        voice->vc_PeriodSlideOn = 1;
        voice->vc_PeriodSlideWithLimit = 1;
        *Note = 0;
        break;
    }
}

void hvl_process_stepfx_3(struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam) {
    int32 i;

    switch (FX) {
    case 0x01: // Portamento up (period slide down)
        voice->vc_PeriodSlideSpeed = -FXParam;
        voice->vc_PeriodSlideOn = 1;
        voice->vc_PeriodSlideWithLimit = 0;
        break;
    case 0x02: // Portamento down
        voice->vc_PeriodSlideSpeed = FXParam;
        voice->vc_PeriodSlideOn = 1;
        voice->vc_PeriodSlideWithLimit = 0;
        break;
    case 0x04: // Filter override
        if ((FXParam == 0) || (FXParam == 0x40))
            break;
        if (FXParam < 0x40) {
            voice->vc_IgnoreFilter = FXParam;
            break;
        }
        if (FXParam > 0x7f)
            break;
        voice->vc_FilterPos = FXParam - 0x40;
        break;
    case 0x0c: // Volume
        FXParam &= 0xff;
        if (FXParam <= 0x40) {
            voice->vc_NoteMaxVolume = FXParam;
            break;
        }

        if ((FXParam -= 0x50) < 0)
            break; // 1.6

        if (FXParam <= 0x40) {
            for (i = 0; i < ht->ht_Channels; i++)
                ht->ht_Voices[i].vc_TrackMasterVolume = FXParam;
            break;
        }

        if ((FXParam -= 0xa0 - 0x50) < 0)
            break; // 1.6

        if (FXParam <= 0x40)
            voice->vc_TrackMasterVolume = FXParam;
        break;

    case 0xe: // Extended commands;
        switch (FXParam >> 4) {
        case 0x1:                                            // Fineslide up
            voice->vc_PeriodSlidePeriod -= (FXParam & 0x0f); // 1.8
            voice->vc_PlantPeriod = 1;
            break;

        case 0x2:                                            // Fineslide down
            voice->vc_PeriodSlidePeriod += (FXParam & 0x0f); // 1.8
            voice->vc_PlantPeriod = 1;
            break;

        case 0x4: // Vibrato control
            voice->vc_VibratoDepth = FXParam & 0x0f;
            break;

        case 0x0a: // Fine volume up
            voice->vc_NoteMaxVolume += FXParam & 0x0f;

            if (voice->vc_NoteMaxVolume > 0x40)
                voice->vc_NoteMaxVolume = 0x40;
            break;

        case 0x0b: // Fine volume down
            voice->vc_NoteMaxVolume -= FXParam & 0x0f;

            if (voice->vc_NoteMaxVolume < 0)
                voice->vc_NoteMaxVolume = 0;
            break;

        case 0x0f: // Misc flags (1.5)
            if (ht->ht_Version < 1)
                break;
            switch (FXParam & 0xf) {
            case 1:
                voice->vc_OverrideTranspose = voice->vc_Transpose;
                break;
            }
            break;
        }
        break;
    }
}

void hvl_process_step(struct hvl_tune *ht, struct hvl_voice *voice) {
    int32 Note, Instr, donenotedel;
    const struct hvl_step *Step;

    if (voice->vc_TrackOn == 0)
        return;

    voice->vc_VolumeSlideUp = voice->vc_VolumeSlideDown = 0;

    Step = &ht->ht_Tracks[ht->ht_Positions[ht->ht_PosNr].pos_Track[voice->vc_VoiceNum]][ht->ht_NoteNr];

    Note = Step->stp_Note;
    Instr = Step->stp_Instrument;

    // --------- 1.6: from here --------------

    donenotedel = 0;

    // Do notedelay here
    if (((Step->stp_FX & 0xf) == 0xe) && ((Step->stp_FXParam & 0xf0) == 0xd0)) {
        if (voice->vc_NoteDelayOn) {
            voice->vc_NoteDelayOn = 0;
            donenotedel = 1;
        } else {
            if ((Step->stp_FXParam & 0x0f) < ht->ht_Tempo) {
                voice->vc_NoteDelayWait = Step->stp_FXParam & 0x0f;
                if (voice->vc_NoteDelayWait) {
                    voice->vc_NoteDelayOn = 1;
                    return;
                }
            }
        }
    }

    if ((donenotedel == 0) && ((Step->stp_FXb & 0xf) == 0xe) && ((Step->stp_FXbParam & 0xf0) == 0xd0)) {
        if (voice->vc_NoteDelayOn) {
            voice->vc_NoteDelayOn = 0;
        } else {
            if ((Step->stp_FXbParam & 0x0f) < ht->ht_Tempo) {
                voice->vc_NoteDelayWait = Step->stp_FXbParam & 0x0f;
                if (voice->vc_NoteDelayWait) {
                    voice->vc_NoteDelayOn = 1;
                    return;
                }
            }
        }
    }

    // --------- 1.6: to here --------------

    if (Note)
        voice->vc_OverrideTranspose = 1000; // 1.5

    hvl_process_stepfx_1(ht, voice, Step->stp_FX & 0xf, Step->stp_FXParam);
    hvl_process_stepfx_1(ht, voice, Step->stp_FXb & 0xf, Step->stp_FXbParam);

    if ((Instr) && (Instr <= ht->ht_InstrumentNr)) {
        struct hvl_instrument *Ins;
        int16 SquareLower, SquareUpper, d6, d3, d4;

        /* 1.4: Reset panning to last set position */
        voice->vc_Pan = voice->vc_SetPan;
        voice->vc_PanMultLeft = panning_left[voice->vc_Pan];
        voice->vc_PanMultRight = panning_right[voice->vc_Pan];

        voice->vc_PeriodSlideSpeed = voice->vc_PeriodSlidePeriod = voice->vc_PeriodSlideLimit = 0;

        voice->vc_PerfSubVolume = 0x40;
        voice->vc_ADSRVolume = 0;
        voice->vc_Instrument = Ins = &ht->ht_Instruments[Instr];
        voice->vc_SamplePos = 0;

        voice->vc_ADSR.aFrames = Ins->ins_Envelope.aFrames;
        voice->vc_ADSR.aVolume = voice->vc_ADSR.aFrames ? Ins->ins_Envelope.aVolume * 256 / voice->vc_ADSR.aFrames : Ins->ins_Envelope.aVolume * 256; // XXX
        voice->vc_ADSR.dFrames = Ins->ins_Envelope.dFrames;
        voice->vc_ADSR.dVolume = voice->vc_ADSR.dFrames ? (Ins->ins_Envelope.dVolume - Ins->ins_Envelope.aVolume) * 256 / voice->vc_ADSR.dFrames
                                                        : Ins->ins_Envelope.dVolume * 256; // XXX
        voice->vc_ADSR.sFrames = Ins->ins_Envelope.sFrames;
        voice->vc_ADSR.rFrames = Ins->ins_Envelope.rFrames;
        voice->vc_ADSR.rVolume = voice->vc_ADSR.rFrames ? (Ins->ins_Envelope.rVolume - Ins->ins_Envelope.dVolume) * 256 / voice->vc_ADSR.rFrames
                                                        : Ins->ins_Envelope.rVolume * 256; // XXX

        voice->vc_WaveLength = Ins->ins_WaveLength;
        voice->vc_NoteMaxVolume = Ins->ins_Volume;

        voice->vc_VibratoCurrent = 0;
        voice->vc_VibratoDelay = Ins->ins_VibratoDelay;
        voice->vc_VibratoDepth = Ins->ins_VibratoDepth;
        voice->vc_VibratoSpeed = Ins->ins_VibratoSpeed;
        voice->vc_VibratoPeriod = 0;

        voice->vc_HardCutRelease = Ins->ins_HardCutRelease;
        voice->vc_HardCut = Ins->ins_HardCutReleaseFrames;

        voice->vc_IgnoreSquare = voice->vc_SquareSlidingIn = 0;
        voice->vc_SquareWait = voice->vc_SquareOn = 0;

        SquareLower = Ins->ins_SquareLowerLimit >> (5 - voice->vc_WaveLength);
        SquareUpper = Ins->ins_SquareUpperLimit >> (5 - voice->vc_WaveLength);

        if (SquareUpper < SquareLower) {
            int16 t = SquareUpper;
            SquareUpper = SquareLower;
            SquareLower = t;
        }

        voice->vc_SquareUpperLimit = SquareUpper;
        voice->vc_SquareLowerLimit = SquareLower;

        voice->vc_IgnoreFilter = voice->vc_FilterWait = voice->vc_FilterOn = 0;
        voice->vc_FilterSlidingIn = 0;

        d6 = Ins->ins_FilterSpeed;
        d3 = Ins->ins_FilterLowerLimit;
        d4 = Ins->ins_FilterUpperLimit;

        if (d3 & 0x80)
            d6 |= 0x20;
        if (d4 & 0x80)
            d6 |= 0x40;

        voice->vc_FilterSpeed = d6;
        d3 &= ~0x80;
        d4 &= ~0x80;

        if (d3 > d4) {
            int16 t = d3;
            d3 = d4;
            d4 = t;
        }

        voice->vc_FilterUpperLimit = d4;
        voice->vc_FilterLowerLimit = d3;
        voice->vc_FilterPos = 32;

        voice->vc_PerfWait = voice->vc_PerfCurrent = 0;
        voice->vc_PerfSpeed = Ins->ins_PList.pls_Speed;
        voice->vc_PerfList = &voice->vc_Instrument->ins_PList;

        voice->vc_RingMixSource = NULL; // No ring modulation
        voice->vc_RingSamplePos = 0;
        voice->vc_RingPlantPeriod = 0;
        voice->vc_RingNewWaveform = 0;
    }

    voice->vc_PeriodSlideOn = 0;

    hvl_process_stepfx_2(ht, voice, Step->stp_FX & 0xf, Step->stp_FXParam, &Note);
    hvl_process_stepfx_2(ht, voice, Step->stp_FXb & 0xf, Step->stp_FXbParam, &Note);

    if (Note) {
        voice->vc_TrackPeriod = Note;
        voice->vc_PlantPeriod = 1;
    }

    hvl_process_stepfx_3(ht, voice, Step->stp_FX & 0xf, Step->stp_FXParam);
    hvl_process_stepfx_3(ht, voice, Step->stp_FXb & 0xf, Step->stp_FXbParam);
}

void hvl_plist_command_parse(const struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam) {
    switch (FX) {
    case 0:
        if ((FXParam > 0) && (FXParam < 0x40)) {
            if (voice->vc_IgnoreFilter) {
                voice->vc_FilterPos = voice->vc_IgnoreFilter;
                voice->vc_IgnoreFilter = 0;
            } else {
                voice->vc_FilterPos = FXParam;
            }
            voice->vc_NewWaveform = 1;
        }
        break;

    case 1:
        voice->vc_PeriodPerfSlideSpeed = FXParam;
        voice->vc_PeriodPerfSlideOn = 1;
        break;

    case 2:
        voice->vc_PeriodPerfSlideSpeed = -FXParam;
        voice->vc_PeriodPerfSlideOn = 1;
        break;

    case 3:
        if (voice->vc_IgnoreSquare == 0)
            voice->vc_SquarePos = FXParam >> (5 - voice->vc_WaveLength);
        else
            voice->vc_IgnoreSquare = 0;
        break;

    case 4:
        if (FXParam == 0) {
            voice->vc_SquareInit = (voice->vc_SquareOn ^= 1);
            voice->vc_SquareSign = 1;
        } else {

            if (FXParam & 0x0f) {
                voice->vc_SquareInit = (voice->vc_SquareOn ^= 1);
                voice->vc_SquareSign = 1;
                if ((FXParam & 0x0f) == 0x0f)
                    voice->vc_SquareSign = -1;
            }

            if (FXParam & 0xf0) {
                voice->vc_FilterInit = (voice->vc_FilterOn ^= 1);
                voice->vc_FilterSign = 1;
                if ((FXParam & 0xf0) == 0xf0)
                    voice->vc_FilterSign = -1;
            }
        }
        break;

    case 5:
        voice->vc_PerfCurrent = FXParam;
        break;

    case 7:
        // Ring modulate with triangle
        if ((FXParam >= 1) && (FXParam <= 0x3C)) {
            voice->vc_RingBasePeriod = FXParam;
            voice->vc_RingFixedPeriod = 1;
        } else if ((FXParam >= 0x81) && (FXParam <= 0xBC)) {
            voice->vc_RingBasePeriod = FXParam - 0x80;
            voice->vc_RingFixedPeriod = 0;
        } else {
            voice->vc_RingBasePeriod = 0;
            voice->vc_RingFixedPeriod = 0;
            voice->vc_RingNewWaveform = 0;
            voice->vc_RingAudioSource = NULL; // turn it off
            voice->vc_RingMixSource = NULL;
            break;
        }
        voice->vc_RingWaveform = 0;
        voice->vc_RingNewWaveform = 1;
        voice->vc_RingPlantPeriod = 1;
        break;

    case 8: // Ring modulate with sawtooth
        if ((FXParam >= 1) && (FXParam <= 0x3C)) {
            voice->vc_RingBasePeriod = FXParam;
            voice->vc_RingFixedPeriod = 1;
        } else if ((FXParam >= 0x81) && (FXParam <= 0xBC)) {
            voice->vc_RingBasePeriod = FXParam - 0x80;
            voice->vc_RingFixedPeriod = 0;
        } else {
            voice->vc_RingBasePeriod = 0;
            voice->vc_RingFixedPeriod = 0;
            voice->vc_RingNewWaveform = 0;
            voice->vc_RingAudioSource = NULL;
            voice->vc_RingMixSource = NULL;
            break;
        }

        voice->vc_RingWaveform = 1;
        voice->vc_RingNewWaveform = 1;
        voice->vc_RingPlantPeriod = 1;
        break;

    /* New in HivelyTracker 1.4 */
    case 9:
        if (FXParam > 127)
            FXParam -= 256;
        voice->vc_Pan = (FXParam + 128);
        voice->vc_PanMultLeft = panning_left[voice->vc_Pan];
        voice->vc_PanMultRight = panning_right[voice->vc_Pan];
        break;

    case 12:
        if (FXParam <= 0x40) {
            voice->vc_NoteMaxVolume = FXParam;
            break;
        }

        if ((FXParam -= 0x50) < 0)
            break;

        if (FXParam <= 0x40) {
            voice->vc_PerfSubVolume = FXParam;
            break;
        }

        if ((FXParam -= 0xa0 - 0x50) < 0)
            break;

        if (FXParam <= 0x40)
            voice->vc_TrackMasterVolume = FXParam;
        break;

    case 15:
        voice->vc_PerfSpeed = voice->vc_PerfWait = FXParam;
        break;
    }
}

void hvl_process_frame(struct hvl_tune *ht, struct hvl_voice *voice) {
    static const uint8 Offsets[] = {0x00, 0x04, 0x04 + 0x08, 0x04 + 0x08 + 0x10, 0x04 + 0x08 + 0x10 + 0x20, 0x04 + 0x08 + 0x10 + 0x20 + 0x40};

    if (voice->vc_TrackOn == 0)
        return;

    if (voice->vc_NoteDelayOn) {
        if (voice->vc_NoteDelayWait <= 0)
            hvl_process_step(ht, voice);
        else
            voice->vc_NoteDelayWait--;
    }

    if (voice->vc_HardCut) {
        int32 nextinst;

        if (ht->ht_NoteNr + 1 < ht->ht_TrackLength)
            nextinst = ht->ht_Tracks[voice->vc_Track][ht->ht_NoteNr + 1].stp_Instrument;
        else
            nextinst = ht->ht_Tracks[voice->vc_NextTrack][0].stp_Instrument;

        if (nextinst) {
            int32 d1;

            d1 = ht->ht_Tempo - voice->vc_HardCut;

            if (d1 < 0)
                d1 = 0;

            if (!voice->vc_NoteCutOn) {
                voice->vc_NoteCutOn = 1;
                voice->vc_NoteCutWait = d1;
                voice->vc_HardCutReleaseF = -(d1 - ht->ht_Tempo);
            } else {
                voice->vc_HardCut = 0;
            }
        }
    }

    if (voice->vc_NoteCutOn) {
        if (voice->vc_NoteCutWait <= 0) {
            voice->vc_NoteCutOn = 0;

            if (voice->vc_HardCutRelease) {
                voice->vc_ADSR.rFrames = voice->vc_HardCutReleaseF;
                voice->vc_ADSR.rVolume = 0;
                if (voice->vc_ADSR.rFrames > 0)
                    voice->vc_ADSR.rVolume = -(voice->vc_ADSRVolume - (voice->vc_Instrument->ins_Envelope.rVolume << 8)) / voice->vc_ADSR.rFrames;
                voice->vc_ADSR.aFrames = voice->vc_ADSR.dFrames = voice->vc_ADSR.sFrames = 0;
            } else {
                voice->vc_NoteMaxVolume = 0;
            }
        } else {
            voice->vc_NoteCutWait--;
        }
    }

    // ADSR envelope
    if (voice->vc_ADSR.aFrames) {
        voice->vc_ADSRVolume += voice->vc_ADSR.aVolume;

        if (--voice->vc_ADSR.aFrames <= 0)
            voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.aVolume << 8;

    } else if (voice->vc_ADSR.dFrames) {

        voice->vc_ADSRVolume += voice->vc_ADSR.dVolume;

        if (--voice->vc_ADSR.dFrames <= 0)
            voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.dVolume << 8;

    } else if (voice->vc_ADSR.sFrames) {

        voice->vc_ADSR.sFrames--;

    } else if (voice->vc_ADSR.rFrames) {

        voice->vc_ADSRVolume += voice->vc_ADSR.rVolume;

        if (--voice->vc_ADSR.rFrames <= 0)
            voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.rVolume << 8;
    }

    // VolumeSlide
    voice->vc_NoteMaxVolume = voice->vc_NoteMaxVolume + voice->vc_VolumeSlideUp - voice->vc_VolumeSlideDown;

    if (voice->vc_NoteMaxVolume < 0)
        voice->vc_NoteMaxVolume = 0;
    else if (voice->vc_NoteMaxVolume > 0x40)
        voice->vc_NoteMaxVolume = 0x40;

    // Portamento
    if (voice->vc_PeriodSlideOn) {
        if (voice->vc_PeriodSlideWithLimit) {
            int32 d0, d2;

            d0 = voice->vc_PeriodSlidePeriod - voice->vc_PeriodSlideLimit;
            d2 = voice->vc_PeriodSlideSpeed;

            if (d0 > 0)
                d2 = -d2;

            if (d0) {
                int32 d3;

                d3 = (d0 + d2) ^ d0;

                if (d3 >= 0)
                    d0 = voice->vc_PeriodSlidePeriod + d2;
                else
                    d0 = voice->vc_PeriodSlideLimit;

                voice->vc_PeriodSlidePeriod = d0;
                voice->vc_PlantPeriod = 1;
            }
        } else {
            voice->vc_PeriodSlidePeriod += voice->vc_PeriodSlideSpeed;
            voice->vc_PlantPeriod = 1;
        }
    }

    // Vibrato
    if (voice->vc_VibratoDepth) {
        if (voice->vc_VibratoDelay <= 0) {
            voice->vc_VibratoPeriod = (vib_tab[voice->vc_VibratoCurrent] * voice->vc_VibratoDepth) >> 7;
            voice->vc_PlantPeriod = 1;
            voice->vc_VibratoCurrent = (voice->vc_VibratoCurrent + voice->vc_VibratoSpeed) & 0x3f;
        } else {
            voice->vc_VibratoDelay--;
        }
    }

    // PList
    if (voice->vc_PerfList != 0) {
        if (voice->vc_Instrument && voice->vc_PerfCurrent < voice->vc_Instrument->ins_PList.pls_Length) {
            int signedOverflow = (voice->vc_PerfWait == 128);

            voice->vc_PerfWait--;
            if (signedOverflow || (int8)voice->vc_PerfWait <= 0) {
                uint32 i;
                int32 cur;

                cur = voice->vc_PerfCurrent++;
                voice->vc_PerfWait = voice->vc_PerfSpeed;

                if (voice->vc_PerfList->pls_Entries[cur].ple_Waveform) {
                    voice->vc_Waveform = voice->vc_PerfList->pls_Entries[cur].ple_Waveform - 1;
                    voice->vc_NewWaveform = 1;
                    voice->vc_PeriodPerfSlideSpeed = voice->vc_PeriodPerfSlidePeriod = 0;
                }

                // Holdwave
                voice->vc_PeriodPerfSlideOn = 0;

                for (i = 0; i < 2; i++)
                    hvl_plist_command_parse(ht, voice, voice->vc_PerfList->pls_Entries[cur].ple_FX[i] & 0xff,
                                            voice->vc_PerfList->pls_Entries[cur].ple_FXParam[i] & 0xff);

                // GetNote
                if (voice->vc_PerfList->pls_Entries[cur].ple_Note) {
                    voice->vc_InstrPeriod = voice->vc_PerfList->pls_Entries[cur].ple_Note;
                    voice->vc_PlantPeriod = 1;
                    voice->vc_FixedNote = voice->vc_PerfList->pls_Entries[cur].ple_Fixed;
                }
            }
        } else {
            if (voice->vc_PerfWait)
                voice->vc_PerfWait--;
            else
                voice->vc_PeriodPerfSlideSpeed = 0;
        }
    }

    // PerfPortamento
    if (voice->vc_PeriodPerfSlideOn) {
        voice->vc_PeriodPerfSlidePeriod -= voice->vc_PeriodPerfSlideSpeed;

        if (voice->vc_PeriodPerfSlidePeriod)
            voice->vc_PlantPeriod = 1;
    }

    if (voice->vc_Waveform == 3 - 1 && voice->vc_SquareOn) {
        if (--voice->vc_SquareWait <= 0) {
            int32 d1, d2, d3;

            d1 = voice->vc_SquareLowerLimit;
            d2 = voice->vc_SquareUpperLimit;
            d3 = voice->vc_SquarePos;

            if (voice->vc_SquareInit) {
                voice->vc_SquareInit = 0;

                if (d3 <= d1) {
                    voice->vc_SquareSlidingIn = 1;
                    voice->vc_SquareSign = 1;
                } else if (d3 >= d2) {
                    voice->vc_SquareSlidingIn = 1;
                    voice->vc_SquareSign = -1;
                }
            }

            // NoSquareInit
            if (d1 == d3 || d2 == d3) {
                if (voice->vc_SquareSlidingIn)
                    voice->vc_SquareSlidingIn = 0;
                else
                    voice->vc_SquareSign = -voice->vc_SquareSign;
            }

            d3 += voice->vc_SquareSign;
            voice->vc_SquarePos = d3;
            voice->vc_PlantSquare = 1;
            voice->vc_SquareWait = voice->vc_Instrument->ins_SquareSpeed;
        }
    }

    if (voice->vc_FilterOn && --voice->vc_FilterWait <= 0) {
        uint32 i, FMax;
        int32 d1, d2, d3;

        d1 = voice->vc_FilterLowerLimit;
        d2 = voice->vc_FilterUpperLimit;
        d3 = voice->vc_FilterPos;

        if (voice->vc_FilterInit) {
            voice->vc_FilterInit = 0;
            if (d3 <= d1) {
                voice->vc_FilterSlidingIn = 1;
                voice->vc_FilterSign = 1;
            } else if (d3 >= d2) {
                voice->vc_FilterSlidingIn = 1;
                voice->vc_FilterSign = -1;
            }
        }

        // NoFilterInit
        FMax = (voice->vc_FilterSpeed < 4) ? (5 - voice->vc_FilterSpeed) : 1;

        for (i = 0; i < FMax; i++) {
            if ((d1 == d3) || (d2 == d3)) {
                if (voice->vc_FilterSlidingIn)
                    voice->vc_FilterSlidingIn = 0;
                else
                    voice->vc_FilterSign = -voice->vc_FilterSign;
            }
            d3 += voice->vc_FilterSign;
        }

        if (d3 < 1)
            d3 = 1;
        if (d3 > 63)
            d3 = 63;
        voice->vc_FilterPos = d3;
        voice->vc_NewWaveform = 1;
        voice->vc_FilterWait = voice->vc_FilterSpeed - 3;

        if (voice->vc_FilterWait < 1)
            voice->vc_FilterWait = 1;
    }

    if (voice->vc_Waveform == 3 - 1 || voice->vc_PlantSquare) {
        // CalcSquare
        uint32 i;
        int32 Delta;
        const int8 *SquarePtr;
        int32 X;

        SquarePtr = &waves[WO_SQUARES + (voice->vc_FilterPos - 0x20) * (0xfc + 0xfc + 0x80 * 0x1f + 0x80 + 0x280 * 3)];
        X = voice->vc_SquarePos << (5 - voice->vc_WaveLength);

        if (X > 0x20) {
            X = 0x40 - X;
            voice->vc_SquareReverse = 1;
        }

        // OkDownSquare
        if (X > 0)
            SquarePtr += (X - 1) << 7;

        Delta = 32 >> voice->vc_WaveLength;
        ht->ht_WaveformTab[2] = voice->vc_SquareTempBuffer;

        for (i = 0; i < (1 << voice->vc_WaveLength) * 4; i++) {
            voice->vc_SquareTempBuffer[i] = *SquarePtr;
            SquarePtr += Delta;
        }

        voice->vc_NewWaveform = 1;
        voice->vc_Waveform = 3 - 1;
        voice->vc_PlantSquare = 0;
    }

    if (voice->vc_Waveform == 4 - 1)
        voice->vc_NewWaveform = 1;

    if (voice->vc_RingNewWaveform) {
        const int8 *rasrc;

        if (voice->vc_RingWaveform > 1)
            voice->vc_RingWaveform = 1;

        rasrc = ht->ht_WaveformTab[voice->vc_RingWaveform];
        rasrc += Offsets[voice->vc_WaveLength];

        voice->vc_RingAudioSource = rasrc;
    }

    if (voice->vc_NewWaveform) {
        const int8 *AudioSource;

        AudioSource = ht->ht_WaveformTab[voice->vc_Waveform];

        if (voice->vc_Waveform != 3 - 1)
            AudioSource += (voice->vc_FilterPos - 0x20) * (0xfc + 0xfc + 0x80 * 0x1f + 0x80 + 0x280 * 3);

        if (voice->vc_Waveform < 3 - 1) {
            // GetWLWaveformlor2
            AudioSource += Offsets[voice->vc_WaveLength];
        }

        if (voice->vc_Waveform == 4 - 1) {
            // AddRandomMoving
            AudioSource += (voice->vc_WNRandom & (2 * 0x280 - 1)) & ~1;
            // GoOnRandom
            voice->vc_WNRandom += 2239384;
            voice->vc_WNRandom = ((((voice->vc_WNRandom >> 8) | (voice->vc_WNRandom << 24)) + 782323) ^ 75) - 6735;
        }

        voice->vc_AudioSource = AudioSource;
    }

    // Ring modulation period calculation
    if (voice->vc_RingAudioSource) {
        voice->vc_RingAudioPeriod = voice->vc_RingBasePeriod;

        if (!(voice->vc_RingFixedPeriod)) {
            if (voice->vc_OverrideTranspose != 1000) // 1.5
                voice->vc_RingAudioPeriod += voice->vc_OverrideTranspose + voice->vc_TrackPeriod - 1;
            else
                voice->vc_RingAudioPeriod += voice->vc_Transpose + voice->vc_TrackPeriod - 1;
        }

        if (voice->vc_RingAudioPeriod > 5 * 12)
            voice->vc_RingAudioPeriod = 5 * 12;

        if (voice->vc_RingAudioPeriod < 0)
            voice->vc_RingAudioPeriod = 0;

        voice->vc_RingAudioPeriod = period_tab[voice->vc_RingAudioPeriod];

        if (!(voice->vc_RingFixedPeriod))
            voice->vc_RingAudioPeriod += voice->vc_PeriodSlidePeriod;

        voice->vc_RingAudioPeriod += voice->vc_PeriodPerfSlidePeriod + voice->vc_VibratoPeriod;

        if (voice->vc_RingAudioPeriod > 0x0d60)
            voice->vc_RingAudioPeriod = 0x0d60;

        if (voice->vc_RingAudioPeriod < 0x0071)
            voice->vc_RingAudioPeriod = 0x0071;
    }

    // Normal period calculation
    voice->vc_AudioPeriod = voice->vc_InstrPeriod;

    if (!(voice->vc_FixedNote)) {
        if (voice->vc_OverrideTranspose != 1000) // 1.5
            voice->vc_AudioPeriod += voice->vc_OverrideTranspose + voice->vc_TrackPeriod - 1;
        else
            voice->vc_AudioPeriod += voice->vc_Transpose + voice->vc_TrackPeriod - 1;
    }

    if (voice->vc_AudioPeriod > 5 * 12)
        voice->vc_AudioPeriod = 5 * 12;

    if (voice->vc_AudioPeriod < 0)
        voice->vc_AudioPeriod = 0;

    voice->vc_AudioPeriod = period_tab[voice->vc_AudioPeriod];

    if (!(voice->vc_FixedNote))
        voice->vc_AudioPeriod += voice->vc_PeriodSlidePeriod;

    voice->vc_AudioPeriod += voice->vc_PeriodPerfSlidePeriod + voice->vc_VibratoPeriod;

    if (voice->vc_AudioPeriod > 0x0d60)
        voice->vc_AudioPeriod = 0x0d60;

    if (voice->vc_AudioPeriod < 0x0071)
        voice->vc_AudioPeriod = 0x0071;

    voice->vc_AudioVolume =
        (((((((voice->vc_ADSRVolume >> 8) * voice->vc_NoteMaxVolume) >> 6) * voice->vc_PerfSubVolume) >> 6) * voice->vc_TrackMasterVolume) >> 6);
}

void hvl_set_audio(struct hvl_voice *voice, float64 freqf) {
    if (voice->vc_TrackOn == 0) {
        voice->vc_VoiceVolume = 0;
        return;
    }

    voice->vc_VoiceVolume = voice->vc_AudioVolume;

    if (voice->vc_PlantPeriod) {
        float64 freq2;
        uint32 delta;

        voice->vc_PlantPeriod = 0;
        voice->vc_VoicePeriod = voice->vc_AudioPeriod;

        freq2 = Period2Freq(voice->vc_AudioPeriod);
        delta = (uint32)(freq2 / freqf);

        if (delta > (0x280 << 16))
            delta -= (0x280 << 16);
        if (delta == 0)
            delta = 1;
        voice->vc_Delta = delta;
    }

    if (voice->vc_NewWaveform) {
        const int8 *src;

        src = voice->vc_AudioSource;

        if (voice->vc_Waveform == 4 - 1) {
            memcpy(&voice->vc_VoiceBuffer[0], src, 0x280);
        } else {
            uint32 i, WaveLoops;

            WaveLoops = (1 << (5 - voice->vc_WaveLength)) * 5;

            for (i = 0; i < WaveLoops; i++)
                memcpy(&voice->vc_VoiceBuffer[i * 4 * (1 << voice->vc_WaveLength)], src, 4 * (1 << voice->vc_WaveLength));
        }

        voice->vc_VoiceBuffer[0x280] = voice->vc_VoiceBuffer[0];
        voice->vc_MixSource = voice->vc_VoiceBuffer;
    }

    /* Ring Modulation */
    if (voice->vc_RingPlantPeriod) {
        float64 freq2;
        uint32 delta;

        voice->vc_RingPlantPeriod = 0;
        freq2 = Period2Freq(voice->vc_RingAudioPeriod);
        delta = (uint32)(freq2 / freqf);

        if (delta > (0x280 << 16))
            delta -= (0x280 << 16);
        if (delta == 0)
            delta = 1;
        voice->vc_RingDelta = delta;
    }

    if (voice->vc_RingNewWaveform) {
        const int8 *src;
        uint32 i, WaveLoops;

        src = voice->vc_RingAudioSource;

        WaveLoops = (1 << (5 - voice->vc_WaveLength)) * 5;

        for (i = 0; i < WaveLoops; i++)
            memcpy(&voice->vc_RingVoiceBuffer[i * 4 * (1 << voice->vc_WaveLength)], src, 4 * (1 << voice->vc_WaveLength));

        voice->vc_RingVoiceBuffer[0x280] = voice->vc_RingVoiceBuffer[0];
        voice->vc_RingMixSource = voice->vc_RingVoiceBuffer;
    }
}

void hvl_play_irq(struct hvl_tune *ht) {
    uint32 i;

    if (ht->ht_StepWaitFrames == 0) {
        if (ht->ht_GetNewPosition) {
            int32 nextpos = (ht->ht_PosNr + 1 == ht->ht_PositionNr) ? 0 : (ht->ht_PosNr + 1);

            for (i = 0; i < ht->ht_Channels; i++) {
                ht->ht_Voices[i].vc_Track = ht->ht_Positions[ht->ht_PosNr].pos_Track[i];
                ht->ht_Voices[i].vc_Transpose = ht->ht_Positions[ht->ht_PosNr].pos_Transpose[i];
                ht->ht_Voices[i].vc_NextTrack = ht->ht_Positions[nextpos].pos_Track[i];
                ht->ht_Voices[i].vc_NextTranspose = ht->ht_Positions[nextpos].pos_Transpose[i];
            }
            ht->ht_GetNewPosition = 0;
        }

        for (i = 0; i < ht->ht_Channels; i++)
            hvl_process_step(ht, &ht->ht_Voices[i]);

        ht->ht_StepWaitFrames = ht->ht_Tempo;
    }

    for (i = 0; i < ht->ht_Channels; i++)
        hvl_process_frame(ht, &ht->ht_Voices[i]);

    ht->ht_PlayingTime++;
    if (--ht->ht_StepWaitFrames == 0) {
        if (!ht->ht_PatternBreak) {
            ht->ht_NoteNr++;
            if (ht->ht_NoteNr >= ht->ht_TrackLength) {
                ht->ht_PosJump = ht->ht_PosNr + 1;
                ht->ht_PosJumpNote = 0;
                ht->ht_PatternBreak = 1;
            }
        }

        if (ht->ht_PatternBreak) {
            ht->ht_PatternBreak = 0;
            ht->ht_PosNr = ht->ht_PosJump;
            ht->ht_NoteNr = ht->ht_PosJumpNote;
            if (ht->ht_PosNr == ht->ht_PositionNr) {
                ht->ht_SongEndReached = 1;
                ht->ht_PosNr = ht->ht_Restart;
            }
            ht->ht_PosJumpNote = 0;
            ht->ht_PosJump = 0;

            ht->ht_GetNewPosition = 1;
        }
    }

    for (i = 0; i < ht->ht_Channels; i++)
        hvl_set_audio(&ht->ht_Voices[i], ht->ht_Frequency);
}

void hvl_mixchunk(struct hvl_tune *ht, uint32 samples, int8 *buf1, int8 *buf2, int32 bufmod) {
    const int8 *src[MAX_CHANNELS];
    const int8 *rsrc[MAX_CHANNELS];
    uint32 delta[MAX_CHANNELS];
    uint32 rdelta[MAX_CHANNELS];
    int32 vol[MAX_CHANNELS];
    uint32 pos[MAX_CHANNELS];
    uint32 rpos[MAX_CHANNELS];
    uint32 cnt;
    int32 panl[MAX_CHANNELS];
    int32 panr[MAX_CHANNELS];
    //  uint32  vu[MAX_CHANNELS];
    int32 a = 0, b = 0, j;
    uint32 i, chans, loops;

    chans = ht->ht_Channels;
    for (i = 0; i < chans; i++) {
        delta[i] = ht->ht_Voices[i].vc_Delta;
        vol[i] = ht->ht_Voices[i].vc_VoiceVolume;
        pos[i] = ht->ht_Voices[i].vc_SamplePos;
        src[i] = ht->ht_Voices[i].vc_MixSource;
        panl[i] = ht->ht_Voices[i].vc_PanMultLeft;
        panr[i] = ht->ht_Voices[i].vc_PanMultRight;

        /* Ring Modulation */
        rdelta[i] = ht->ht_Voices[i].vc_RingDelta;
        rpos[i] = ht->ht_Voices[i].vc_RingSamplePos;
        rsrc[i] = ht->ht_Voices[i].vc_RingMixSource;

        //    vu[i] = 0;
    }

    do {
        loops = samples;
        for (i = 0; i < chans; i++) {
            if (pos[i] >= (0x280 << 16))
                pos[i] -= 0x280 << 16;
            cnt = ((0x280 << 16) - pos[i] - 1) / delta[i] + 1;
            if (cnt < loops)
                loops = cnt;

            if (rsrc[i]) {
                if (rpos[i] >= (0x280 << 16))
                    rpos[i] -= 0x280 << 16;
                cnt = ((0x280 << 16) - rpos[i] - 1) / rdelta[i] + 1;
                if (cnt < loops)
                    loops = cnt;
            }
        }

        samples -= loops;

        // Inner loop
        do {
            a = 0;
            b = 0;
            for (i = 0; i < chans; i++) {
                if (rsrc[i]) {
                    /* Ring Modulation */
                    j = ((src[i][pos[i] >> 16] * rsrc[i][rpos[i] >> 16]) >> 7) * vol[i];
                    rpos[i] += rdelta[i];
                } else {
                    j = src[i][pos[i] >> 16] * vol[i];
                }

                //        if( abs( j ) > vu[i] ) vu[i] = abs( j );

                a += (j * panl[i]) >> 7;
                b += (j * panr[i]) >> 7;
                pos[i] += delta[i];
            }

            a = (a * ht->ht_mixgain) >> 8;
            b = (b * ht->ht_mixgain) >> 8;

            if (a < -0x8000)
                a = -0x8000;
            if (a > 0x7fff)
                a = 0x7fff;
            if (b < -0x8000)
                b = -0x8000;
            if (b > 0x7fff)
                b = 0x7fff;

            *(int16 *)buf1 = a;
            *(int16 *)buf2 = b;

            loops--;

            buf1 += bufmod;
            buf2 += bufmod;
        } while (loops > 0);
    } while (samples > 0);

    for (i = 0; i < chans; i++) {
        ht->ht_Voices[i].vc_SamplePos = pos[i];
        ht->ht_Voices[i].vc_RingSamplePos = rpos[i];
        //    ht->ht_Voices[i].vc_VUMeter = vu[i];
    }
}

void hvl_DecodeFrame(struct hvl_tune *ht, int8 *buf1, int8 *buf2, int32 bufmod) {
    uint32 samples, loops;

    samples = ht->ht_Frequency / 50 / ht->ht_SpeedMultiplier;
    loops = ht->ht_SpeedMultiplier;

    do {
        hvl_play_irq(ht);
        hvl_mixchunk(ht, samples, buf1, buf2, bufmod);
        buf1 += samples * bufmod;
        buf2 += samples * bufmod;
        loops--;
    } while (loops);
}
