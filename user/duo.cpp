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

#define LCW_PORTA_TABLE_SIZE (64)
// s11.20
static const int32_t portaTable[LCW_PORTA_TABLE_SIZE] = {
  0x08C8C3, // [ 0] 0.549014, 0.0002(sec), n:   11
  0x0D338D, // [ 1] 0.825085, 0.0007(sec), n:   35
  0x0E30B6, // [ 2] 0.886892, 0.0012(sec), n:   57
  0x0EB76F, // [ 3] 0.919784, 0.0017(sec), n:   82
  0x0F0B1C, // [ 4] 0.940212, 0.0023(sec), n:  112
  0x0F436D, // [ 5] 0.953961, 0.0031(sec), n:  146
  0x0F6B52, // [ 6] 0.963701, 0.0039(sec), n:  186
  0x0F88A1, // [ 7] 0.970857, 0.0049(sec), n:  233
  0x0F9EC2, // [ 8] 0.976259, 0.0060(sec), n:  287
  0x0FAFD4, // [ 9] 0.980426, 0.0073(sec), n:  349
  0x0FBD3B, // [10] 0.983698, 0.0088(sec), n:  420
  0x0FC7E8, // [11] 0.986305, 0.0104(sec), n:  500
  0x0FD085, // [12] 0.988408, 0.0123(sec), n:  592
  0x0FD78A, // [13] 0.990122, 0.0145(sec), n:  695
  0x0FDD51, // [14] 0.991533, 0.0169(sec), n:  812
  0x0FE21D, // [15] 0.992703, 0.0197(sec), n:  943
  0x0FE61F, // [16] 0.993682, 0.0227(sec), n: 1089
  0x0FE97E, // [17] 0.994505, 0.0261(sec), n: 1253
  0x0FEC59, // [18] 0.995202, 0.0299(sec), n: 1436
  0x0FEEC7, // [19] 0.995795, 0.0342(sec), n: 1639
  0x0FF0DB, // [20] 0.996302, 0.0388(sec), n: 1864
  0x0FF2A4, // [21] 0.996738, 0.0440(sec), n: 2114
  0x0FF42E, // [22] 0.997114, 0.0498(sec), n: 2390
  0x0FF583, // [23] 0.997440, 0.0561(sec), n: 2694
  0x0FF6AC, // [24] 0.997723, 0.0631(sec), n: 3029
  0x0FF7AF, // [25] 0.997970, 0.0708(sec), n: 3398
  0x0FF892, // [26] 0.998186, 0.0793(sec), n: 3804
  0x0FF958, // [27] 0.998375, 0.0885(sec), n: 4248
  0x0FFA07, // [28] 0.998542, 0.0986(sec), n: 4735
  0x0FFAA2, // [29] 0.998689, 0.1097(sec), n: 5267
  0x0FFB2A, // [30] 0.998820, 0.1219(sec), n: 5848
  0x0FFBA3, // [31] 0.998935, 0.1351(sec), n: 6483
  0x0FFC0F, // [32] 0.999038, 0.1495(sec), n: 7174
  0x0FFC6F, // [33] 0.999129, 0.1652(sec), n: 7927
  0x0FFCC4, // [34] 0.999210, 0.1822(sec), n: 8745
  0x0FFD11, // [35] 0.999283, 0.2007(sec), n: 9635
  0x0FFD55, // [36] 0.999349, 0.2208(sec), n:10600
  0x0FFD92, // [37] 0.999407, 0.2426(sec), n:11646
  0x0FFDC9, // [38] 0.999460, 0.2662(sec), n:12779
  0x0FFDFB, // [39] 0.999507, 0.2918(sec), n:14006
  0x0FFE28, // [40] 0.999550, 0.3194(sec), n:15333
  0x0FFE50, // [41] 0.999588, 0.3493(sec), n:16766
  0x0FFE75, // [42] 0.999623, 0.3815(sec), n:18314
  0x0FFE96, // [43] 0.999654, 0.4163(sec), n:19983
  0x0FFEB4, // [44] 0.999683, 0.4538(sec), n:21783
  0x0FFECF, // [45] 0.999709, 0.4942(sec), n:23721
  0x0FFEE7, // [46] 0.999732, 0.5377(sec), n:25808
  0x0FFEFE, // [47] 0.999754, 0.5844(sec), n:28053
  0x0FFF12, // [48] 0.999773, 0.6347(sec), n:30466
  0x0FFF25, // [49] 0.999791, 0.6887(sec), n:33058
  0x0FFF36, // [50] 0.999807, 0.7467(sec), n:35842
  0x0FFF45, // [51] 0.999822, 0.8089(sec), n:38828
  0x0FFF54, // [52] 0.999836, 0.8756(sec), n:42030
  0x0FFF61, // [53] 0.999848, 0.9471(sec), n:45462
  0x0FFF6D, // [54] 0.999859, 1.0237(sec), n:49138
  0x0FFF78, // [55] 0.999870, 1.1057(sec), n:53074
  0x0FFF82, // [56] 0.999879, 1.1934(sec), n:57284
  0x0FFF8B, // [57] 0.999888, 1.2872(sec), n:61786
  0x0FFF93, // [58] 0.999896, 1.3875(sec), n:66598
  0x0FFF9B, // [59] 0.999904, 1.4946(sec), n:71739
  0x0FFFA2, // [60] 0.999911, 1.6089(sec), n:77228
  0x0FFFA9, // [61] 0.999917, 1.7310(sec), n:83087
  0x0FFFAF, // [62] 0.999923, 1.8612(sec), n:89336
  0x0FFFB5, // [63] 0.999928, 2.0000(sec), n:96000
};

static struct {
  uint16_t shape = 0;
  uint16_t mixRatio = 0;  // = shiftshape, [0 - 0x400]
  uint16_t wave = 0;      // [0 .. 2]
  int16_t note = 0;       // [-24 .. +24]
  uint16_t dir = 0;       // 0: plus, 1: minus
  uint16_t portament = 0; // [0 .. 63]
} s_param;

static struct {
  uint32_t timer1 = 0;
  uint32_t timer2 = 0;
  int32_t pitch1 = 0;   // s11.20
  int32_t pitch2 = 0;   // s11.20
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

// inout/dst s11.20
// factor s11.20
__fast_inline void convergePitch(
  int32_t *inout, int32_t dst, int32_t portaParam)
{
  int32_t diff = dst - *inout;
  if ( diff < 0 ) {
    *inout = dst + (int32_t)( ((int64_t)-diff * portaParam) >> 20 );
  }
  else {
    *inout = dst - (int32_t)( ((int64_t)diff * portaParam) >> 20 );
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
  s_state.pitch2 = (LCW_NOTE_NO_A4 << 16) / 20;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  // s11.20に拡張してから、整数部がoctaveになるように加工
  int32_t pitch = (int32_t)params->pitch << 12;
  pitch = (pitch - (LCW_NOTE_NO_A4 << 20)) / 12;

  int32_t detune = ((s_param.dir == 0) ? 1 : -1) * ((int32_t)s_param.shape << 6);
  detune += (params->shape_lfo >> (31 - 14));
  detune += ((int32_t)s_param.note << 20) / 12;

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

    // input: s11.20 -> s15.16
    const uint32_t dt1 = pitch_to_timer_delta( pitch1 >> 4 );
    const uint32_t dt2 = pitch_to_timer_delta( pitch2 >> 4 );

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
