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

static struct {
  uint16_t shape = 0;
  uint16_t mixRatio = 0;// = shiftshape, [0 - 0x400]
  uint16_t wave = 0;    // [0 .. 2]
  int16_t note = 0;     // [-24 .. +24]
  uint16_t dir = 0;     // 0: plus, 1: minus
} s_param;

static struct {
  uint32_t timer1 = 0;
  uint32_t timer2 = 0;
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

void OSC_INIT(uint32_t platform, uint32_t api)
{
  s_param.shape = 0;
  s_param.mixRatio = 0;
  s_param.wave = 0;

  s_state.timer1 = 0;
  s_state.timer2 = 0;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  // s15.16になるように8bit拡張してから、整数部がoctaveになるように加工
  int32_t pitch = (int32_t)params->pitch << 8;
  pitch = (pitch - (LCW_NOTE_NO_A4 << 16)) / 12;

  int32_t detune = ((s_param.dir == 0) ? 1 : -1) * ((int32_t)s_param.shape << 2);
  detune += (params->shape_lfo >> (31 - 10));
  detune += ((int32_t)s_param.note << 16) / 12;

  const uint32_t dt1 = pitch_to_timer_delta( pitch );
  const uint32_t dt2 = pitch_to_timer_delta( pitch + detune );

  // Temporaries.
  uint32_t t1 = s_state.timer1;
  uint32_t t2 = s_state.timer2;
  
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  // Main Mix/Sub Mix, 8bit(= [0-256])
  const int32_t subVol = s_param.mixRatio >> 2;
  const int32_t mainVol = 0x100 - subVol;

  const LCWOscWaveSource *src = s_waveSources[s_param.wave];  
  for (; y != y_e; ) {
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
  default:
    break;
  }
}
