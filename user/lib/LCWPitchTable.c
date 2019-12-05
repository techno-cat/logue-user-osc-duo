/*
Copyright 2019 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "LCWPitchTable.h"

#define LCW_PRE_LEFT_SHIFT (8)

// u4.28
// Size: 12 * 16 + 1 = 193 // 半音あたり16分割 + 1
// delta = (440 << PRE_SHIFT) * 2^([0-192] / 192) / 48000.0
static uint32_t deltaTable[] = {
    0x258BF259, // [  0] 1.000000,  2.3467
    0x25AEB5BA, // [  1] 1.003617,  2.3552
    0x25D1994B, // [  2] 1.007246,  2.3637
    0x25F49D29, // [  3] 1.010889,  2.3722
    0x2617C172, // [  4] 1.014545,  2.3808
    0x263B0645, // [  5] 1.018215,  2.3894
    0x265E6BC0, // [  6] 1.021897,  2.3981
    0x2681F200, // [  7] 1.025593,  2.4067
    0x26A59924, // [  8] 1.029302,  2.4154
    0x26C9614A, // [  9] 1.033025,  2.4242
    0x26ED4A92, // [ 10] 1.036761,  2.4329
    0x2711551A, // [ 11] 1.040511,  2.4417
    0x27358100, // [ 12] 1.044274,  2.4506
    0x2759CE63, // [ 13] 1.048051,  2.4594
    0x277E3D63, // [ 14] 1.051841,  2.4683
    0x27A2CE1F, // [ 15] 1.055645,  2.4772
    0x27C780B5, // [ 16] 1.059463,  2.4862
    0x27EC5545, // [ 17] 1.063295,  2.4952
    0x28114BEF, // [ 18] 1.067140,  2.5042
    0x283664D2, // [ 19] 1.071000,  2.5133
    0x285BA00E, // [ 20] 1.074873,  2.5224
    0x2880FDC3, // [ 21] 1.078761,  2.5315
    0x28A67E10, // [ 22] 1.082662,  2.5406
    0x28CC2116, // [ 23] 1.086578,  2.5498
    0x28F1E6F4, // [ 24] 1.090508,  2.5591
    0x2917CFCC, // [ 25] 1.094452,  2.5683
    0x293DDBBD, // [ 26] 1.098410,  2.5776
    0x29640AE8, // [ 27] 1.102383,  2.5869
    0x298A5D6D, // [ 28] 1.106370,  2.5963
    0x29B0D36E, // [ 29] 1.110371,  2.6057
    0x29D76D0A, // [ 30] 1.114387,  2.6151
    0x29FE2A64, // [ 31] 1.118417,  2.6246
    0x2A250B9C, // [ 32] 1.122462,  2.6340
    0x2A4C10D3, // [ 33] 1.126522,  2.6436
    0x2A733A2B, // [ 34] 1.130596,  2.6531
    0x2A9A87C5, // [ 35] 1.134685,  2.6627
    0x2AC1F9C2, // [ 36] 1.138789,  2.6724
    0x2AE99045, // [ 37] 1.142907,  2.6820
    0x2B114B70, // [ 38] 1.147041,  2.6917
    0x2B392B63, // [ 39] 1.151189,  2.7015
    0x2B613042, // [ 40] 1.155353,  2.7112
    0x2B895A2E, // [ 41] 1.159531,  2.7210
    0x2BB1A949, // [ 42] 1.163725,  2.7309
    0x2BDA1DB7, // [ 43] 1.167934,  2.7408
    0x2C02B79A, // [ 44] 1.172158,  2.7507
    0x2C2B7714, // [ 45] 1.176397,  2.7606
    0x2C545C48, // [ 46] 1.180652,  2.7706
    0x2C7D6759, // [ 47] 1.184922,  2.7806
    0x2CA6986A, // [ 48] 1.189207,  2.7907
    0x2CCFEF9E, // [ 49] 1.193508,  2.8008
    0x2CF96D1A, // [ 50] 1.197825,  2.8109
    0x2D2310FF, // [ 51] 1.202157,  2.8211
    0x2D4CDB72, // [ 52] 1.206505,  2.8313
    0x2D76CC96, // [ 53] 1.210868,  2.8415
    0x2DA0E48F, // [ 54] 1.215247,  2.8518
    0x2DCB2382, // [ 55] 1.219643,  2.8621
    0x2DF58991, // [ 56] 1.224054,  2.8724
    0x2E2016E3, // [ 57] 1.228481,  2.8828
    0x2E4ACB99, // [ 58] 1.232924,  2.8933
    0x2E75A7DA, // [ 59] 1.237383,  2.9037
    0x2EA0ABCA, // [ 60] 1.241858,  2.9142
    0x2ECBD78E, // [ 61] 1.246349,  2.9248
    0x2EF72B4A, // [ 62] 1.250857,  2.9353
    0x2F22A723, // [ 63] 1.255381,  2.9460
    0x2F4E4B3F, // [ 64] 1.259921,  2.9566
    0x2F7A17C3, // [ 65] 1.264478,  2.9673
    0x2FA60CD5, // [ 66] 1.269051,  2.9780
    0x2FD22A99, // [ 67] 1.273641,  2.9888
    0x2FFE7135, // [ 68] 1.278247,  2.9996
    0x302AE0D0, // [ 69] 1.282870,  3.0105
    0x3057798F, // [ 70] 1.287510,  3.0214
    0x30843B99, // [ 71] 1.292166,  3.0323
    0x30B12713, // [ 72] 1.296840,  3.0433
    0x30DE3C24, // [ 73] 1.301530,  3.0543
    0x310B7AF3, // [ 74] 1.306237,  3.0653
    0x3138E3A6, // [ 75] 1.310961,  3.0764
    0x31667663, // [ 76] 1.315703,  3.0875
    0x31943353, // [ 77] 1.320461,  3.0987
    0x31C21A9B, // [ 78] 1.325237,  3.1099
    0x31F02C64, // [ 79] 1.330030,  3.1211
    0x321E68D4, // [ 80] 1.334840,  3.1324
    0x324CD013, // [ 81] 1.339668,  3.1438
    0x327B6249, // [ 82] 1.344513,  3.1551
    0x32AA1F9D, // [ 83] 1.349375,  3.1665
    0x32D90837, // [ 84] 1.354256,  3.1780
    0x33081C40, // [ 85] 1.359153,  3.1895
    0x33375BDF, // [ 86] 1.364069,  3.2010
    0x3366C73D, // [ 87] 1.369002,  3.2126
    0x33965E83, // [ 88] 1.373954,  3.2242
    0x33C621D8, // [ 89] 1.378923,  3.2359
    0x33F61167, // [ 90] 1.383910,  3.2476
    0x34262D57, // [ 91] 1.388915,  3.2593
    0x345675D3, // [ 92] 1.393938,  3.2711
    0x3486EB02, // [ 93] 1.398980,  3.2829
    0x34B78D0F, // [ 94] 1.404039,  3.2948
    0x34E85C23, // [ 95] 1.409117,  3.3067
    0x35195868, // [ 96] 1.414214,  3.3187
    0x354A8207, // [ 97] 1.419328,  3.3307
    0x357BD92C, // [ 98] 1.424462,  3.3427
    0x35AD5DFE, // [ 99] 1.429613,  3.3548
    0x35DF10AA, // [100] 1.434784,  3.3670
    0x3610F15A, // [101] 1.439973,  3.3791
    0x36430037, // [102] 1.445181,  3.3914
    0x36753D6D, // [103] 1.450408,  3.4036
    0x36A7A927, // [104] 1.455653,  3.4159
    0x36DA4390, // [105] 1.460918,  3.4283
    0x370D0CD3, // [106] 1.466201,  3.4407
    0x3740051C, // [107] 1.471504,  3.4531
    0x37732C95, // [108] 1.476826,  3.4656
    0x37A6836B, // [109] 1.482167,  3.4782
    0x37DA09CA, // [110] 1.487528,  3.4907
    0x380DBFDD, // [111] 1.492908,  3.5034
    0x3841A5D1, // [112] 1.498307,  3.5160
    0x3875BBD1, // [113] 1.503726,  3.5287
    0x38AA020C, // [114] 1.509164,  3.5415
    0x38DE78AC, // [115] 1.514623,  3.5543
    0x39131FDF, // [116] 1.520100,  3.5672
    0x3947F7D3, // [117] 1.525598,  3.5801
    0x397D00B3, // [118] 1.531116,  3.5930
    0x39B23AAE, // [119] 1.536653,  3.6060
    0x39E7A5F1, // [120] 1.542211,  3.6191
    0x3A1D42A9, // [121] 1.547788,  3.6321
    0x3A531104, // [122] 1.553386,  3.6453
    0x3A891131, // [123] 1.559004,  3.6585
    0x3ABF435D, // [124] 1.564643,  3.6717
    0x3AF5A7B6, // [125] 1.570302,  3.6850
    0x3B2C3E6C, // [126] 1.575981,  3.6983
    0x3B6307AC, // [127] 1.581681,  3.7117
    0x3B9A03A6, // [128] 1.587401,  3.7251
    0x3BD13288, // [129] 1.593142,  3.7386
    0x3C089482, // [130] 1.598904,  3.7521
    0x3C4029C3, // [131] 1.604687,  3.7657
    0x3C77F27A, // [132] 1.610490,  3.7793
    0x3CAFEED8, // [133] 1.616315,  3.7930
    0x3CE81F0B, // [134] 1.622161,  3.8067
    0x3D208344, // [135] 1.628027,  3.8204
    0x3D591BB3, // [136] 1.633915,  3.8343
    0x3D91E888, // [137] 1.639825,  3.8481
    0x3DCAE9F4, // [138] 1.645755,  3.8620
    0x3E042028, // [139] 1.651708,  3.8760
    0x3E3D8B54, // [140] 1.657681,  3.8900
    0x3E772BAA, // [141] 1.663677,  3.9041
    0x3EB1015A, // [142] 1.669694,  3.9182
    0x3EEB0C97, // [143] 1.675732,  3.9324
    0x3F254D91, // [144] 1.681793,  3.9466
    0x3F5FC47A, // [145] 1.687875,  3.9609
    0x3F9A7185, // [146] 1.693980,  3.9752
    0x3FD554E4, // [147] 1.700106,  3.9896
    0x40106EC8, // [148] 1.706255,  4.0040
    0x404BBF64, // [149] 1.712426,  4.0185
    0x408746EC, // [150] 1.718619,  4.0330
    0x40C30591, // [151] 1.724835,  4.0476
    0x40FEFB87, // [152] 1.731073,  4.0623
    0x413B2901, // [153] 1.737334,  4.0769
    0x41778E32, // [154] 1.743617,  4.0917
    0x41B42B4E, // [155] 1.749923,  4.1065
    0x41F1008A, // [156] 1.756252,  4.1213
    0x422E0E17, // [157] 1.762604,  4.1362
    0x426B542C, // [158] 1.768979,  4.1512
    0x42A8D2FC, // [159] 1.775376,  4.1662
    0x42E68ABC, // [160] 1.781797,  4.1813
    0x43247BA0, // [161] 1.788242,  4.1964
    0x4362A5DE, // [162] 1.794709,  4.2116
    0x43A109A9, // [163] 1.801200,  4.2268
    0x43DFA739, // [164] 1.807714,  4.2421
    0x441E7EC2, // [165] 1.814252,  4.2574
    0x445D907A, // [166] 1.820814,  4.2728
    0x449CDC97, // [167] 1.827399,  4.2883
    0x44DC634E, // [168] 1.834008,  4.3038
    0x451C24D7, // [169] 1.840641,  4.3194
    0x455C2167, // [170] 1.847298,  4.3350
    0x459C5935, // [171] 1.853979,  4.3507
    0x45DCCC79, // [172] 1.860684,  4.3664
    0x461D7B68, // [173] 1.867414,  4.3822
    0x465E663B, // [174] 1.874168,  4.3980
    0x469F8D29, // [175] 1.880946,  4.4140
    0x46E0F069, // [176] 1.887749,  4.4299
    0x47229034, // [177] 1.894576,  4.4459
    0x47646CC1, // [178] 1.901428,  4.4620
    0x47A68648, // [179] 1.908305,  4.4782
    0x47E8DD03, // [180] 1.915207,  4.4944
    0x482B7129, // [181] 1.922133,  4.5106
    0x486E42F4, // [182] 1.929085,  4.5269
    0x48B1529D, // [183] 1.936062,  4.5433
    0x48F4A05C, // [184] 1.943064,  4.5597
    0x49382C6C, // [185] 1.950091,  4.5762
    0x497BF706, // [186] 1.957144,  4.5928
    0x49C00065, // [187] 1.964222,  4.6094
    0x4A0448C1, // [188] 1.971326,  4.6260
    0x4A48D056, // [189] 1.978456,  4.6428
    0x4A8D975E, // [190] 1.985611,  4.6596
    0x4AD29E13, // [191] 1.992793,  4.6764
    0x4B17E4B1  // [192] 2.000000,  4.6933
};

uint32_t pitch_to_timer_delta(int32_t pitch)
{
  int32_t octave = (pitch >> 16) - LCW_PRE_LEFT_SHIFT;

#if (1)
  // Look up only Ver.
  uint32_t delta = deltaTable[((pitch & 0xFFFF) * 12) >> 12];
#else
  // Linear interpolation Ver.
  // あとで線形補間するのに4bit残しておく
  int32_t tmp = ((pitch & 0xFFFF) * 12) >> 8; 
  int32_t i = tmp >> 4;
  uint32_t delta = deltaTable[i];
  // 4bit分の線形補間
  uint32_t diff = deltaTable[i+1] - delta;
  delta += ( (diff >> 4) * (tmp & 0xF) );
#endif
  if ( octave < 0 ) {
    // memo: 四捨五入しても良いかも
    return delta >> -octave;
  }
  else {
    return delta << octave;
  }
}