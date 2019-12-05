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
  uint16_t subMix = 0;  // = shiftshape, [0-0x400]
  uint16_t wave = 0;    // [0-2]
} s_param;

static struct {
  uint32_t timer = 0;
  uint32_t timerSub = 0;
} s_state;

static const LCWOscWaveSource *s_waveSources[] = {
  &gLcwOscTriSource,
  &gLcwOscPulseSource,
  &gLcwOscSawSource
};

// return s3.24
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

    // 端数8bitで補間して、24bitに変換
    const int32_t diff = tmp[1] - tmp[0];
    uint32_t delta = (t & LCW_OSC_FRAC_MASK) >> (LCW_OSC_FRAC_BITS - 8);
    return (tmp[0] << 8) + (diff * (int32_t)delta);
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  s_param.shape = 0;
  s_param.subMix = 0;
  s_param.wave = 0;

  s_state.timer = 0;
  s_state.timerSub = 0;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  // s15.16になるように8bit拡張してから、整数部がoctaveになるように加工
  int32_t pitch = (int32_t)params->pitch << 8;
  pitch = (pitch - (LCW_NOTE_NO_A4 << 16)) / 12;

  int32_t shape = (int32_t)(s_param.shape << 2) + (params->shape_lfo >> (31 - 10));
  const uint32_t dt1 = pitch_to_timer_delta( pitch );
  const uint32_t dt2 = pitch_to_timer_delta( pitch + shape );

  // Temporaries.
  uint32_t t1 = s_state.timer;
  uint32_t t2 = s_state.timerSub;
  
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  // Main Mix/Sub Mix, 6bit(= [0-64])
  const int32_t subVol = s_param.subMix >> 4;
  const int32_t mainVol = 0x40 - subVol;

  const LCWOscWaveSource *src = s_waveSources[s_param.wave];  
  for (; y != y_e; ) {
    int32_t out1 = (myLookUp(t1, dt1, src) * 7) >> 3; // x0.875
    int32_t out2 = (myLookUp(t2, dt2, src) * 7) >> 3; // x0.875

    int32_t out = (out1 * mainVol) + (out2 * subVol);
    out = clipminmaxi32( -0x1000000, (out >> 6), 0x1000000 );

    // q24 -> q31
    *(y++) = out << (31 - 24);

    t1 = (t1 + dt1) & LCW_OSC_TIMER_MASK;
    t2 = (t2 + dt2) & LCW_OSC_TIMER_MASK;
  }

  s_state.timer = t1;
  s_state.timerSub = t2;
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
    s_param.subMix = (uint16_t)clipmaxu32(value, 0x400);
    break;
  case k_user_osc_param_id1:
    s_param.wave = (uint16_t)clipmaxu32(value, 2);
    break;
  default:
    break;
  }
}
