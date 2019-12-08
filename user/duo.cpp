/*
Copyright 2019 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "userosc.h"
#include "LCWPitchTable.h"
#include "LCWAntiAliasingTable.h"
#include "LCWOscWaveSource.h"

#define LCW_OSC_TIMER_BITS (LCW_PITCH_DELTA_VALUE_BITS)
#define LCW_OSC_TIMER_MAX (1 << LCW_OSC_TIMER_BITS)
#define LCW_OSC_TIMER_MASK ((LCW_OSC_TIMER_MAX) - 1)

#define LCW_OSC_FRAC_BITS (LCW_OSC_TIMER_BITS - LCW_OSC_TABLE_BITS)
#define LCW_OSC_FRAC_MAX (1 << LCW_OSC_FRAC_BITS)
#define LCW_OSC_FRAC_MASK ((LCW_OSC_FRAC_MAX) - 1)

#define LCW_PORTA_TABLE_SIZE (101)
// s7.24
static const int32_t portaTable[LCW_PORTA_TABLE_SIZE] = {
  0x008C8C2D, // [  0] 0.549014, 0.0002(sec), n:    11
  0x00CC0BE8, // [  1] 0.797057, 0.0006(sec), n:    30
  0x00DBFAB5, // [  2] 0.859294, 0.0009(sec), n:    45
  0x00E506F4, // [  3] 0.894637, 0.0013(sec), n:    62
  0x00EAF328, // [  4] 0.917773, 0.0017(sec), n:    80
  0x00EF1ED1, // [  5] 0.934064, 0.0021(sec), n:   101
  0x00F2322B, // [  6] 0.946078, 0.0026(sec), n:   124
  0x00F48A00, // [  7] 0.955231, 0.0031(sec), n:   150
  0x00F65E47, // [  8] 0.962376, 0.0038(sec), n:   180
  0x00F7D2F2, // [  9] 0.968063, 0.0044(sec), n:   212
  0x00F90030, // [ 10] 0.972659, 0.0052(sec), n:   249
  0x00F9F6DF, // [ 11] 0.976423, 0.0060(sec), n:   289
  0x00FAC31D, // [ 12] 0.979540, 0.0070(sec), n:   334
  0x00FB6DD5, // [ 13] 0.982145, 0.0080(sec), n:   383
  0x00FBFDB6, // [ 14] 0.984340, 0.0091(sec), n:   437
  0x00FC77DC, // [ 15] 0.986204, 0.0104(sec), n:   497
  0x00FCE03D, // [ 16] 0.987797, 0.0117(sec), n:   562
  0x00FD39F4, // [ 17] 0.989166, 0.0132(sec), n:   634
  0x00FD8778, // [ 18] 0.990348, 0.0148(sec), n:   712
  0x00FDCAC6, // [ 19] 0.991375, 0.0166(sec), n:   797
  0x00FE0576, // [ 20] 0.992271, 0.0185(sec), n:   890
  0x00FE38D9, // [ 21] 0.993055, 0.0206(sec), n:   991
  0x00FE6601, // [ 22] 0.993744, 0.0229(sec), n:  1100
  0x00FE8DD3, // [ 23] 0.994352, 0.0254(sec), n:  1219
  0x00FEB10D, // [ 24] 0.994889, 0.0281(sec), n:  1348
  0x00FED04F, // [ 25] 0.995366, 0.0310(sec), n:  1487
  0x00FEEC1F, // [ 26] 0.995790, 0.0341(sec), n:  1637
  0x00FF04EE, // [ 27] 0.996169, 0.0375(sec), n:  1799
  0x00FF1B1E, // [ 28] 0.996508, 0.0411(sec), n:  1974
  0x00FF2F01, // [ 29] 0.996811, 0.0451(sec), n:  2162
  0x00FF40DF, // [ 30] 0.997084, 0.0493(sec), n:  2365
  0x00FF50F4, // [ 31] 0.997329, 0.0538(sec), n:  2582
  0x00FF5F75, // [ 32] 0.997550, 0.0587(sec), n:  2816
  0x00FF6C90, // [ 33] 0.997750, 0.0639(sec), n:  3067
  0x00FF786D, // [ 34] 0.997931, 0.0695(sec), n:  3335
  0x00FF832F, // [ 35] 0.998095, 0.0755(sec), n:  3623
  0x00FF8CF4, // [ 36] 0.998245, 0.0819(sec), n:  3931
  0x00FF95D7, // [ 37] 0.998380, 0.0888(sec), n:  4260
  0x00FF9DEF, // [ 38] 0.998504, 0.0961(sec), n:  4612
  0x00FFA551, // [ 39] 0.998616, 0.1039(sec), n:  4988
  0x00FFAC10, // [ 40] 0.998719, 0.1123(sec), n:  5389
  0x00FFB23B, // [ 41] 0.998813, 0.1212(sec), n:  5817
  0x00FFB7E1, // [ 42] 0.998900, 0.1307(sec), n:  6273
  0x00FFBD0F, // [ 43] 0.998979, 0.1408(sec), n:  6759
  0x00FFC1D0, // [ 44] 0.999051, 0.1516(sec), n:  7276
  0x00FFC62E, // [ 45] 0.999118, 0.1630(sec), n:  7826
  0x00FFCA33, // [ 46] 0.999179, 0.1752(sec), n:  8411
  0x00FFCDE6, // [ 47] 0.999236, 0.1882(sec), n:  9032
  0x00FFD14F, // [ 48] 0.999288, 0.2019(sec), n:  9692
  0x00FFD475, // [ 49] 0.999336, 0.2165(sec), n: 10393
  0x00FFD75D, // [ 50] 0.999380, 0.2320(sec), n: 11136
  0x00FFDA0C, // [ 51] 0.999421, 0.2484(sec), n: 11924
  0x00FFDC88, // [ 52] 0.999459, 0.2658(sec), n: 12760
  0x00FFDED5, // [ 53] 0.999494, 0.2843(sec), n: 13645
  0x00FFE0F7, // [ 54] 0.999526, 0.3038(sec), n: 14583
  0x00FFE2F1, // [ 55] 0.999557, 0.3245(sec), n: 15576
  0x00FFE4C7, // [ 56] 0.999585, 0.3464(sec), n: 16626
  0x00FFE67B, // [ 57] 0.999611, 0.3695(sec), n: 17736
  0x00FFE811, // [ 58] 0.999635, 0.3940(sec), n: 18911
  0x00FFE98A, // [ 59] 0.999657, 0.4198(sec), n: 20151
  0x00FFEAE9, // [ 60] 0.999678, 0.4471(sec), n: 21462
  0x00FFEC30, // [ 61] 0.999698, 0.4760(sec), n: 22846
  0x00FFED61, // [ 62] 0.999716, 0.5064(sec), n: 24307
  0x00FFEE7D, // [ 63] 0.999733, 0.5385(sec), n: 25848
  0x00FFEF86, // [ 64] 0.999749, 0.5724(sec), n: 27474
  0x00FFF07E, // [ 65] 0.999763, 0.6081(sec), n: 29188
  0x00FFF165, // [ 66] 0.999777, 0.6457(sec), n: 30995
  0x00FFF23E, // [ 67] 0.999790, 0.6854(sec), n: 32899
  0x00FFF308, // [ 68] 0.999802, 0.7272(sec), n: 34904
  0x00FFF3C5, // [ 69] 0.999813, 0.7712(sec), n: 37015
  0x00FFF477, // [ 70] 0.999824, 0.8175(sec), n: 39237
  0x00FFF51D, // [ 71] 0.999834, 0.8662(sec), n: 41576
  0x00FFF5B8, // [ 72] 0.999843, 0.9174(sec), n: 44036
  0x00FFF64A, // [ 73] 0.999852, 0.9713(sec), n: 46623
  0x00FFF6D3, // [ 74] 0.999860, 1.0280(sec), n: 49342
  0x00FFF754, // [ 75] 0.999868, 1.0875(sec), n: 52201
  0x00FFF7CD, // [ 76] 0.999875, 1.1501(sec), n: 55204
  0x00FFF83E, // [ 77] 0.999882, 1.2158(sec), n: 58359
  0x00FFF8A9, // [ 78] 0.999888, 1.2848(sec), n: 61672
  0x00FFF90D, // [ 79] 0.999894, 1.3573(sec), n: 65149
  0x00FFF96C, // [ 80] 0.999900, 1.4333(sec), n: 68800
  0x00FFF9C4, // [ 81] 0.999905, 1.5131(sec), n: 72630
  0x00FFFA18, // [ 82] 0.999910, 1.5968(sec), n: 76647
  0x00FFFA67, // [ 83] 0.999915, 1.6846(sec), n: 80861
  0x00FFFAB1, // [ 84] 0.999919, 1.7766(sec), n: 85278
  0x00FFFAF7, // [ 85] 0.999923, 1.8731(sec), n: 89909
  0x00FFFB39, // [ 86] 0.999927, 1.9742(sec), n: 94761
  0x00FFFB77, // [ 87] 0.999931, 2.0801(sec), n: 99846
  0x00FFFBB2, // [ 88] 0.999934, 2.1911(sec), n:105171
  0x00FFFBEA, // [ 89] 0.999938, 2.3073(sec), n:110748
  0x00FFFC1E, // [ 90] 0.999941, 2.4289(sec), n:116586
  0x00FFFC4F, // [ 91] 0.999944, 2.5562(sec), n:122698
  0x00FFFC7E, // [ 92] 0.999946, 2.6895(sec), n:129094
  0x00FFFCAB, // [ 93] 0.999949, 2.8289(sec), n:135786
  0x00FFFCD4, // [ 94] 0.999952, 2.9747(sec), n:142786
  0x00FFFCFC, // [ 95] 0.999954, 3.1272(sec), n:150107
  0x00FFFD21, // [ 96] 0.999956, 3.2867(sec), n:157762
  0x00FFFD45, // [ 97] 0.999958, 3.4534(sec), n:165764
  0x00FFFD66, // [ 98] 0.999960, 3.6277(sec), n:174128
  0x00FFFD86, // [ 99] 0.999962, 3.8098(sec), n:182868
  0x00FFFDA4  // [100] 0.999964, 4.0000(sec), n:192000
};

static struct {
  uint16_t shape = 0;
  uint16_t mixRatio = 0;  // = shiftshape, [0 - 0x400]
  uint16_t wave = 0;      // [0 .. 2]
  int16_t note = 0;       // [-24 .. +24]
  uint16_t dir = 0;       // 0: plus, 1: minus
  uint16_t portament = 0; // [0 .. 100]
} s_param;

static struct {
  uint32_t timer1 = 0;
  uint32_t timer2 = 0;
  int32_t pitch1 = 0;   // s7.24
  int32_t pitch2 = 0;   // s7.24
} s_state;

static const LCWOscWaveSource *s_waveSources[] = {
  &gLcwOscTriSource,
  &gLcwOscPulseSource,
  &gLcwOscSawSource
};

// return s15.16
__fast_inline int32_t myLookUp(
  int32_t t, int32_t dt, const LCWOscWaveSource *src)
{
    // 桁溢れ対策として、q16に変換
    uint32_t dt0 = dt >> (LCW_OSC_TIMER_BITS - 16);

    int32_t tmp[] = { 0, 0 };
    for (int32_t i=0; i<src->count; i++) {
      const int16_t *p = src->tables[i];
  
      // AAテーブルはdt=0.5までしか定義されていない
      uint32_t j = (dt0 * src->factors[i]) >> (16 - (LCW_AA_TABLE_BITS + 1));
      if ( (uint32_t)LCW_AA_TABLE_SIZE <= j ) {
        break;
      }

      int32_t gain = gLcwAntiAiliasingTable[j];
      uint32_t t0 = t >> LCW_OSC_FRAC_BITS;
      uint32_t t1 = (t0 + 1) & LCW_OSC_TABLE_MASK;

      // s15.16
      tmp[0] += ( (p[t0] * gain) >> (14 + LCW_AA_VALUE_BITS - 16) );
      tmp[1] += ( (p[t1] * gain) >> (14 + LCW_AA_VALUE_BITS - 16) );
    }

    // 端数14bitで補間
    const int32_t diff = tmp[1] - tmp[0];
    int32_t delta = (t & LCW_OSC_FRAC_MASK) >> (LCW_OSC_FRAC_BITS - 14);
    return tmp[0] + ((diff * delta) >> 14);
}

// inout/dst s7.24
// factor s7.24
__fast_inline void convergePitch(
  int32_t *inout, int32_t dst, int32_t portaParam)
{
  int32_t diff = dst - *inout;
  if ( diff < 0 ) {
    *inout = dst + (int32_t)( ((int64_t)-diff * portaParam) >> 24 );
  }
  else {
    *inout = dst - (int32_t)( ((int64_t)diff * portaParam) >> 24 );
  }
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  s_param.shape = 0;
  s_param.mixRatio = 0;
  s_param.wave = 0;

  s_state.timer1 = 0;
  s_state.timer2 = 0;
  s_state.pitch1 =
  s_state.pitch2 = (LCW_NOTE_NO_A4 << 24) / 12;
}

#define LCW_PITCH_DETUNE_RANGE ((1 << 20) / 18) // memo: 50centだと物足りない
void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  // s11.20に拡張してから、整数部がoctaveになるように加工
  int32_t pitch = (int32_t)params->pitch << 12;
  pitch = (pitch - (LCW_NOTE_NO_A4 << 20)) / 12;
  int32_t detune = ((int32_t)s_param.shape * LCW_PITCH_DETUNE_RANGE) >> 10;
  detune *= ( s_param.dir == 0 ) ? 1 : -1;
  detune += (params->shape_lfo >> (31 - 16));
  detune += ((int32_t)s_param.note << 20) / 12;

  // s11.20 -> s7.24
  pitch <<= 4;
  detune <<= 4;

  // Temporaries.
  uint32_t t1 = s_state.timer1;
  uint32_t t2 = s_state.timer2;
  int32_t pitch1 = s_state.pitch1;
  int32_t pitch2 = s_state.pitch2;

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  // Main Mix/Sub Mix, 8bit(= [0-256])
  const int32_t subVol = s_param.mixRatio >> 2;
  const int32_t mainVol = 0x100 - subVol;

  const int32_t portaParam = portaTable[s_param.portament];
  const LCWOscWaveSource *src = s_waveSources[s_param.wave];  
  for (; y != y_e; ) {
    // portament
    convergePitch(&pitch1, pitch, portaParam);
    convergePitch(&pitch2, pitch + detune, portaParam);

    // input: s7.24 -> s15.16
    const uint32_t dt1 = pitch_to_timer_delta( pitch1 >> 8 );
    const uint32_t dt2 = pitch_to_timer_delta( pitch2 >> 8 );

    // s15.16 -> s11.20
    // x 0.9375(= 15/16)
    int32_t out1 = myLookUp(t1, dt1, src) * 15;
    int32_t out2 = myLookUp(t2, dt2, src) * 15;

    // s11.20 -> s3.28
    int32_t out = (out1 * mainVol) + (out2 * subVol);
    out = clipminmaxi32( -0x10000000, out, 0x10000000 );

    // q28 -> q31
    *(y++) = out << (31 - 28);

    t1 = (t1 + dt1) & LCW_OSC_TIMER_MASK;
    t2 = (t2 + dt2) & LCW_OSC_TIMER_MASK;
  }

  s_state.timer1 = t1;
  s_state.timer2 = t2;
  s_state.pitch1 = pitch1;
  s_state.pitch2 = pitch2;
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
  return;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  return;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index) {
  case k_user_osc_param_shape:
    s_param.shape = value;
    break;
  case k_user_osc_param_shiftshape:
    s_param.mixRatio = (uint16_t)clipmaxu32(value, 0x400);
    break;
  case k_user_osc_param_id1:
    s_param.wave = (uint16_t)clipmaxu32(value, 2);
    break;
  case k_user_osc_param_id2:
    s_param.note = (int16_t)clipmaxu32(value, 48) - 24;
    break;
  case k_user_osc_param_id3:
    s_param.dir = value;
    break;
  case k_user_osc_param_id4:
    s_param.portament = (uint16_t)clipmaxu32(value, LCW_PORTA_TABLE_SIZE - 1);
    break;
  default:
    break;
  }
}
