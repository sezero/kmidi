/*
	$Id$

    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    resample.c
*/

#include <math.h>
#include <stdio.h>
#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "resample.h"

#ifndef tplus
static int dont_cspline = 0;
#endif

#if defined(CSPLINE_INTERPOLATION)
# define INTERPVARS      int32   ofsd, v0, v1, v2, v3, temp;
# define RESAMPLATION \
        v1 = (int32)src[(ofs>>FRACTION_BITS)]; \
        v2 = (int32)src[(ofs>>FRACTION_BITS)+1]; \
	if(dont_cspline || \
	   ((ofs-(1L<<FRACTION_BITS))<ls)||((ofs+(2L<<FRACTION_BITS))>le)){ \
                *dest++ = (sample_t)(v1 + (((v2-v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS)); \
	}else{ \
		ofsd=ofs; \
                v0 = (int32)src[(ofs>>FRACTION_BITS)-1]; \
                v3 = (int32)src[(ofs>>FRACTION_BITS)+2]; \
                ofs &= FRACTION_MASK; \
                temp=v2; \
		v2 = (6*v2 + \
		      ((((((5*v3 - 11*v2 + 7*v1 - v0)* \
		       ofs)>>FRACTION_BITS)*ofs)>>(FRACTION_BITS+2))-1))*ofs; \
                ofs = (1L << FRACTION_BITS) - ofs; \
		v1 = (6*v1 + \
		      ((((((5*v0 - 11*v1 + 7*temp - v3)* \
		       ofs)>>FRACTION_BITS)*ofs)>>(FRACTION_BITS+2))-1))*ofs; \
		v1 = (v1 + v2)/(6L<<FRACTION_BITS); \
		*dest++ = (v1 > 32767)? 32767: ((v1 < -32768)? -32768: v1); \
		ofs=ofsd; \
	}
#elif defined(LAGRANGE_INTERPOLATION)
# define INTERPVARS      int32   ofsd, v0, v1, v2, v3;
# define RESAMPLATION \
        v1 = (int32)src[(ofs>>FRACTION_BITS)]; \
        v2 = (int32)src[(ofs>>FRACTION_BITS)+1]; \
	if(dont_cspline || \
	   ((ofs-(1L<<FRACTION_BITS))<ls)||((ofs+(2L<<FRACTION_BITS))>le)){ \
                *dest++ = (sample_t)(v1 + (((v2-v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS)); \
	}else{ \
                v0 = (int32)src[(ofs>>FRACTION_BITS)-1]; \
                v3 = (int32)src[(ofs>>FRACTION_BITS)+2]; \
                ofsd = (ofs & FRACTION_MASK) + (1L << FRACTION_BITS); \
                v1 = v1*ofsd>>FRACTION_BITS; \
                v2 = v2*ofsd>>FRACTION_BITS; \
                v3 = v3*ofsd>>FRACTION_BITS; \
                ofsd -= (1L << FRACTION_BITS); \
                v0 = v0*ofsd>>FRACTION_BITS; \
                v2 = v2*ofsd>>FRACTION_BITS; \
                v3 = v3*ofsd>>FRACTION_BITS; \
                ofsd -= (1L << FRACTION_BITS); \
                v0 = v0*ofsd>>FRACTION_BITS; \
                v1 = v1*ofsd>>FRACTION_BITS; \
                v3 = v3*ofsd; \
                ofsd -= (1L << FRACTION_BITS); \
                v0 = (v3 - v0*ofsd)/(6L << FRACTION_BITS); \
                v1 = (v1 - v2)*ofsd>>(FRACTION_BITS+1); \
		v1 += v0; \
		*dest++ = (v1 > 32767)? 32767: ((v1 < -32768)? -32768: v1); \
	}
#elif defined(LINEAR_INTERPOLATION)
# if defined(LOOKUP_HACK) && defined(LOOKUP_INTERPOLATION)
#   define RESAMPLATION \
       v1=src[ofs>>FRACTION_BITS];\
       v2=src[(ofs>>FRACTION_BITS)+1];\
       *dest++ = (sample_t)(v1 + (iplookup[(((v2-v1)<<5) & 0x03FE0) | \
           ((ofs & FRACTION_MASK) >> (FRACTION_BITS-5))]));
# else
#   define RESAMPLATION \
      v1=src[ofs>>FRACTION_BITS];\
      v2=src[(ofs>>FRACTION_BITS)+1];\
      *dest++ = (sample_t)(v1 + (((v2-v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
# endif
#  define INTERPVARS int32 v1, v2;
#else
/* Earplugs recommended for maximum listening enjoyment */
#  define RESAMPLATION *dest++=src[ofs>>FRACTION_BITS];
#  define INTERPVARS
#endif

#define FINALINTERP if (ofs == le) *dest++=src[(ofs>>FRACTION_BITS)-1]/2;
/* So it isn't interpolation. At least it's final. */

static sample_t resample_buffer[AUDIO_BUFFER_SIZE];
static uint32 resample_buffer_offset;
static sample_t *vib_resample_voice(int, uint32 *, int);
static sample_t *normal_resample_voice(int, uint32 *, int);


/*************** resampling with fixed increment *****************/

static sample_t *rs_plain(int v, uint32 *countptr)
{
  /* Play sample until end, then free the voice. */
  INTERPVARS
  Voice
    *vp=&voice[v];
  sample_t
    *dest=resample_buffer+resample_buffer_offset,
    *src=vp->sample->data;
  int32
    ofs=vp->sample_offset,
    incr=vp->sample_increment,
#if defined(LAGRANGE_INTERPOLATION) || defined(CSPLINE_INTERPOLATION)
    ls=0,
#endif /* LAGRANGE_INTERPOLATION */
    le=vp->sample->data_length,
    count=*countptr;
#ifdef PRECALC_LOOPS
  int32 i, j;
#endif

  if (!incr) return resample_buffer; /* --gl */

#ifdef PRECALC_LOOPS
  if (incr<0) incr = -incr; /* In case we're coming out of a bidir loop */

  /* Precalc how many times we should go through the loop.
     NOTE: Assumes that incr > 0 and that ofs <= le */
  i = (le - ofs) / incr + 1;

  if (i > count)
    {
      i = count;
      count = 0;
    }
  else count -= i;

  for(j = 0; j < i; j++)
    {
      RESAMPLATION;
      ofs += incr;
    }

  if (ofs >= le)
    {
      FINALINTERP;
      vp->status=VOICE_FREE;
      ctl->note(v);
      *countptr-=count+1;
    }
#else /* PRECALC_LOOPS */
    while (count--)
    {
      RESAMPLATION;
      ofs += incr;
      if (ofs >= le)
	{
	  FINALINTERP;
	  vp->status=VOICE_FREE;
 	  ctl->note(v);
	  *countptr-=count+1;
	  break;
	}
    }
#endif /* PRECALC_LOOPS */

  vp->sample_offset=ofs; /* Update offset */
  return resample_buffer+resample_buffer_offset;
}

static sample_t *rs_loop(Voice *vp, uint32 count)
{
  /* Play sample until end-of-loop, skip back and continue. */
  INTERPVARS
  int32
    ofs=vp->sample_offset,
    incr=vp->sample_increment,
    le=vp->sample->loop_end,
#if defined(LAGRANGE_INTERPOLATION) || defined(CSPLINE_INTERPOLATION)
    ls=vp->sample->loop_start,
#endif /* LAGRANGE_INTERPOLATION */
    ll=le - vp->sample->loop_start;
  sample_t
    *dest=resample_buffer+resample_buffer_offset,
    *src=vp->sample->data;
#ifdef PRECALC_LOOPS
  int32 i, j;
#endif

#ifdef PRECALC_LOOPS
  if (incr < 1) incr = 1; /* fixup --gl */
  while (count)
    {
      while (ofs >= le)
	ofs -= ll;
      /* Precalc how many times we should go through the loop */
      i = (le - ofs) / incr + 1;
      if (i > count)
	{
	  i = count;
	  count = 0;
	}
      else count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
    }
#else
  while (count--)
    {
      RESAMPLATION;
      ofs += incr;
      if (ofs>=le)
	ofs -= ll; /* Hopefully the loop is longer than an increment. */
    }
#endif

  vp->sample_offset=ofs; /* Update offset */
  return resample_buffer+resample_buffer_offset;
}

static sample_t *rs_bidir(Voice *vp, uint32 count)
{
  INTERPVARS
  int32
    ofs=vp->sample_offset,
    incr=vp->sample_increment,
    le=vp->sample->loop_end,
    ls=vp->sample->loop_start;
  sample_t
    *dest=resample_buffer+resample_buffer_offset,
    *src=vp->sample->data;

#ifdef PRECALC_LOOPS
  int32
    le2 = le<<1,
    ls2 = ls<<1,
    i, j;
  /* Play normally until inside the loop region */

  if (ofs <= ls)
    {
      /* NOTE: Assumes that incr > 0, which is NOT always the case
	 when doing bidirectional looping.  I have yet to see a case
	 where both ofs <= ls AND incr < 0, however. */
      if (incr < 0) i = ls - ofs;
	else
      i = (ls - ofs) / incr + 1;
      if (i > count)
	{
	  i = count;
	  count = 0;
	}
      else count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
    }

  /* Then do the bidirectional looping */

  while(count)
    {
      /* Precalc how many times we should go through the loop */
      i = ((incr > 0 ? le : ls) - ofs) / incr + 1;
      if (i > count)
	{
	  i = count;
	  count = 0;
	}
      else count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
      if (ofs>=le)
	{
	  /* fold the overshoot back in */
	  ofs = le2 - ofs;
	  incr *= -1;
	}
      else if (ofs <= ls)
	{
	  ofs = ls2 - ofs;
	  incr *= -1;
	}
    }

#else /* PRECALC_LOOPS */
  /* Play normally until inside the loop region */

  if (ofs < ls)
    {
      while (count--)
	{
	  RESAMPLATION;
	  ofs += incr;
	  if (ofs>=ls)
	    break;
	}
    }

  /* Then do the bidirectional looping */

  if (count>0)
    while (count--)
      {
	RESAMPLATION;
	ofs += incr;
	if (ofs>=le)
	  {
	    /* fold the overshoot back in */
	    ofs = le - (ofs - le);
	    incr = -incr;
	  }
	else if (ofs <= ls)
	  {
	    ofs = ls + (ls - ofs);
	    incr = -incr;
	  }
      }
#endif /* PRECALC_LOOPS */
  vp->sample_increment=incr;
  vp->sample_offset=ofs; /* Update offset */
  return resample_buffer+resample_buffer_offset;
}

/*********************** vibrato versions ***************************/

/* We only need to compute one half of the vibrato sine cycle */
static int vib_phase_to_inc_ptr(int phase)
{
  if (phase < VIBRATO_SAMPLE_INCREMENTS/2)
    return VIBRATO_SAMPLE_INCREMENTS/2-1-phase;
  else if (phase >= 3*VIBRATO_SAMPLE_INCREMENTS/2)
    return 5*VIBRATO_SAMPLE_INCREMENTS/2-1-phase;
  else
    return phase-VIBRATO_SAMPLE_INCREMENTS/2;
}

static int32 update_vibrato(Voice *vp, int sign)
{
  int32 depth;
  int phase, pb;
  double a;

  if(vp->vibrato_delay > 0)
  {
      vp->vibrato_delay -= vp->vibrato_control_ratio;
      if(vp->vibrato_delay > 0)
	  return vp->sample_increment;
  }

  if (vp->vibrato_phase++ >= 2*VIBRATO_SAMPLE_INCREMENTS-1)
    vp->vibrato_phase=0;
  phase=vib_phase_to_inc_ptr(vp->vibrato_phase);

  if (vp->vibrato_sample_increment[phase])
    {
      if (sign)
	return -vp->vibrato_sample_increment[phase];
      else
	return vp->vibrato_sample_increment[phase];
    }

  /* Need to compute this sample increment. */

  depth = vp->vibrato_depth;
  if(depth < vp->modulation_wheel)
      depth = vp->modulation_wheel;
  depth <<= 7;

  if (vp->vibrato_sweep && !vp->modulation_wheel)
    {
      /* Need to update sweep */
      vp->vibrato_sweep_position += vp->vibrato_sweep;
      if (vp->vibrato_sweep_position >= (1<<SWEEP_SHIFT))
	vp->vibrato_sweep=0;
      else
	{
	  /* Adjust depth */
	  depth *= vp->vibrato_sweep_position;
	  depth >>= SWEEP_SHIFT;
	}
    }

  pb=(int)((sine(vp->vibrato_phase *
			(SINE_CYCLE_LENGTH/(2*VIBRATO_SAMPLE_INCREMENTS)))
	    * (double)(depth) * VIBRATO_AMPLITUDE_TUNING));

  a = FRSCALE(((double)(vp->sample->sample_rate) *
		  (double)(vp->frequency)) /
		 ((double)(vp->sample->root_freq) *
		  (double)(play_mode->rate)),
		 FRACTION_BITS);
  if(pb<0)
  {
      pb = -pb;
      a /= bend_fine[(pb>>5) & 0xFF] * bend_coarse[pb>>13];
  }
  else
      a *= bend_fine[(pb>>5) & 0xFF] * bend_coarse[pb>>13];
  a += 0.5;

  /* If the sweep's over, we can store the newly computed sample_increment */
  if (!vp->vibrato_sweep || vp->modulation_wheel)
    vp->vibrato_sample_increment[phase]=(int32) a;

  if (sign)
    a = -a; /* need to preserve the loop direction */

  return (int32) a;
}

static sample_t *rs_vib_plain(int v, uint32 *countptr)
{

  /* Play sample until end, then free the voice. */

  INTERPVARS
  Voice *vp=&voice[v];
  sample_t
    *dest=resample_buffer+resample_buffer_offset,
    *src=vp->sample->data;
  int32
#if defined(LAGRANGE_INTERPOLATION) || defined(CSPLINE_INTERPOLATION)
    ls=0,
#endif /* LAGRANGE_INTERPOLATION */
    le=vp->sample->data_length,
    ofs=vp->sample_offset,
    incr=vp->sample_increment;
  uint32
    count=*countptr;
  int
    cc=vp->vibrato_control_counter;

  /* This has never been tested */

  if (incr<0) incr = -incr; /* In case we're coming out of a bidir loop */

  while (count--)
    {
      if (!cc--)
	{
	  cc=vp->vibrato_control_ratio;
	  incr=update_vibrato(vp, 0);
	}
      RESAMPLATION;
      ofs += incr;
      if (ofs >= le)
	{
	  FINALINTERP;
	  vp->status=VOICE_FREE;
 	  ctl->note(v);
	  *countptr-=count+1;
	  break;
	}
    }

  vp->vibrato_control_counter=cc;
  vp->sample_increment=incr;
  vp->sample_offset=ofs; /* Update offset */
  return resample_buffer+resample_buffer_offset;
}

static sample_t *rs_vib_loop(Voice *vp, uint32 count)
{
  /* Play sample until end-of-loop, skip back and continue. */
  INTERPVARS
  int32
    ofs=vp->sample_offset,
    incr=vp->sample_increment,
#if defined(LAGRANGE_INTERPOLATION) || defined(CSPLINE_INTERPOLATION)
    ls=vp->sample->loop_start,
#endif /* LAGRANGE_INTERPOLATION */
    le=vp->sample->loop_end,
    ll=le - vp->sample->loop_start;
  sample_t
    *dest=resample_buffer+resample_buffer_offset,
    *src=vp->sample->data;
  int
    cc=vp->vibrato_control_counter;

#ifdef PRECALC_LOOPS
  int32 i, j;
  int
    vibflag=0;

  if (!incr) return resample_buffer+resample_buffer_offset;

  while (count)
    {
      /* Hopefully the loop is longer than an increment */
      while(ofs >= le)
	ofs -= ll;
      /* Precalc how many times to go through the loop, taking
	 the vibrato control ratio into account this time. */
      i = (le - ofs) / incr + 1;
      if(i > count) i = count;
      if(i > cc)
	{
	  i = cc;
	  vibflag = 1;
	}
      else cc -= i;
      count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
      if(vibflag)
	{
	  cc = vp->vibrato_control_ratio;
	  incr = update_vibrato(vp, 0);
	  vibflag = 0;
	}
    }

#else /* PRECALC_LOOPS */
  while (count--)
    {
      if (!cc--)
	{
	  cc=vp->vibrato_control_ratio;
	  incr=update_vibrato(vp, 0);
	}
      RESAMPLATION;
      ofs += incr;
      if (ofs>=le)
	ofs -= ll; /* Hopefully the loop is longer than an increment. */
    }
#endif /* PRECALC_LOOPS */

  vp->vibrato_control_counter=cc;
  vp->sample_increment=incr;
  vp->sample_offset=ofs; /* Update offset */
  return resample_buffer+resample_buffer_offset;
}

static sample_t *rs_vib_bidir(Voice *vp, uint32 count)
{
  INTERPVARS
  int32
    ofs=vp->sample_offset,
    incr=vp->sample_increment,
    le=vp->sample->loop_end,
    ls=vp->sample->loop_start;
  sample_t
    *dest=resample_buffer+resample_buffer_offset,
    *src=vp->sample->data;
  int
    cc=vp->vibrato_control_counter;

#ifdef PRECALC_LOOPS
  int32
    le2=le<<1,
    ls2=ls<<1,
    i, j;
  int
    vibflag = 0;

  /* Play normally until inside the loop region */
  while (count && (ofs <= ls))
    {
      i = (ls - ofs) / incr + 1;
      if (i > count) i = count;
      if (i > cc)
	{
	  i = cc;
	  vibflag = 1;
	}
      else cc -= i;
      count -= i;
      for(j = 0; j < i; j++)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
      if (vibflag)
	{
	  cc = vp->vibrato_control_ratio;
	  incr = update_vibrato(vp, 0);
	  vibflag = 0;
	}
    }

  /* Then do the bidirectional looping */

  while (count)
    {
      /* Precalc how many times we should go through the loop */
      i = ((incr > 0 ? le : ls) - ofs) / incr + 1;
      if(i > count) i = count;
      if(i > cc)
	{
	  i = cc;
	  vibflag = 1;
	}
      else cc -= i;
      count -= i;
      while (i--)
	{
	  RESAMPLATION;
	  ofs += incr;
	}
      if (vibflag)
	{
	  cc = vp->vibrato_control_ratio;
	  incr = update_vibrato(vp, (incr < 0));
	  vibflag = 0;
	}
      if (ofs >= le)
	{
	  /* fold the overshoot back in */
	  ofs = le2 - ofs;
	  incr *= -1;
	}
      else if (ofs <= ls)
	{
	  ofs = ls2 - ofs;
	  incr *= -1;
	}
    }

#else /* PRECALC_LOOPS */
  /* Play normally until inside the loop region */

  if (ofs < ls)
    {
      while (count--)
	{
	  if (!cc--)
	    {
	      cc=vp->vibrato_control_ratio;
	      incr=update_vibrato(vp, 0);
	    }
	  RESAMPLATION;
	  ofs += incr;
	  if (ofs>=ls)
	    break;
	}
    }

  /* Then do the bidirectional looping */

  if (count>0)
    while (count--)
      {
	if (!cc--)
	  {
	    cc=vp->vibrato_control_ratio;
	    incr=update_vibrato(vp, (incr < 0));
	  }
	RESAMPLATION;
	ofs += incr;
	if (ofs>=le)
	  {
	    /* fold the overshoot back in */
	    ofs = le - (ofs - le);
	    incr = -incr;
	  }
	else if (ofs <= ls)
	  {
	    ofs = ls + (ls - ofs);
	    incr = -incr;
	  }
      }
#endif /* PRECALC_LOOPS */

  vp->vibrato_control_counter=cc;
  vp->sample_increment=incr;
  vp->sample_offset=ofs; /* Update offset */
  return resample_buffer+resample_buffer_offset;
}

static int rs_update_porta(int v)
{
    Voice *vp=&voice[v];
    int32 d;

    d = vp->porta_dpb;
    if(vp->porta_pb < 0)
    {
	if(d > -vp->porta_pb)
	    d = -vp->porta_pb;
    }
    else
    {
	if(d > vp->porta_pb)
	    d = -vp->porta_pb;
	else
	    d = -d;
    }

    vp->porta_pb += d;
    if(vp->porta_pb == 0)
    {
	vp->porta_control_ratio = 0;
	vp->porta_pb = 0;
    }
    recompute_freq(v);
    return vp->porta_control_ratio;
}

static sample_t *porta_resample_voice(int v, uint32 *countptr, int mode)
{
    Voice *vp=&voice[v];
    uint32 n = *countptr;
    int32 i;
    sample_t *(* resampler)(int, uint32 *, int);
    int cc = vp->porta_control_counter;
    int loop;

    if(vp->vibrato_control_ratio)
	resampler = vib_resample_voice;
    else
	resampler = normal_resample_voice;
    if(mode != 1)
	loop = 1;
    else
	loop = 0;

    /* vp->cache = NULL; */
    resample_buffer_offset = 0;
    while(resample_buffer_offset < n)
    {
	if(cc == 0)
	{
	    if((cc = rs_update_porta(v)) == 0)
	    {
		i = n - resample_buffer_offset;
		resampler(v, &i, mode);
		resample_buffer_offset += i;
		break;
	    }
	}

	i = n - resample_buffer_offset;
	if(i > cc)
	    i = cc;
	resampler(v, &i, mode);
	resample_buffer_offset += i;

	if(!loop && vp->status == VOICE_FREE)
	    break;
	cc -= i;
    }
    *countptr = resample_buffer_offset;
    resample_buffer_offset = 0;
    vp->porta_control_counter = cc;
    return resample_buffer;
}

static sample_t *vib_resample_voice(int v, uint32 *countptr, int mode)
{
    Voice *vp = &voice[v];

    /* vp->cache = NULL; */
    if(mode == 0)
	return rs_vib_loop(vp, *countptr);
    if(mode == 1)
	return rs_vib_plain(v, countptr);
    return rs_vib_bidir(vp, *countptr);
}

static sample_t *normal_resample_voice(int v, uint32 *countptr, int mode)
{
    Voice *vp = &voice[v];
    if(mode == 0)
	return rs_loop(vp, *countptr);
    if(mode == 1)
	return rs_plain(v, countptr);
    return rs_bidir(vp, *countptr);
}

sample_t *real_resample_voice(int v, uint32 *countptr)
{
    Voice *vp=&voice[v];
    int mode;

    if(!(vp->sample->sample_rate))
    {
	int32 ofs;

	/* Pre-resampled data -- just update the offset and check if
	   we're out of data. */
	ofs=vp->sample_offset >> FRACTION_BITS; /* Kind of silly to use
						   FRACTION_BITS here... */
	if(*countptr >= (vp->sample->data_length>>FRACTION_BITS) - ofs)
	{
	    /* Note finished. Free the voice. */
	    vp->status = VOICE_FREE;
	    ctl->note(v);

	    /* Let the caller know how much data we had left */
	    *countptr = (vp->sample->data_length>>FRACTION_BITS) - ofs;
	}
	else
	    vp->sample_offset += *countptr << FRACTION_BITS;
	return vp->sample->data+ofs;
    }

    mode = vp->sample->modes;
    if((mode & MODES_LOOPING) &&
       ((mode & MODES_ENVELOPE) ||
	(vp->status & (VOICE_ON | VOICE_SUSTAINED))))
    {
	if(mode & MODES_PINGPONG)
	{
	    /* vp->cache = NULL; */
	    mode = 2;
	}
	else
	    mode = 0;
    }
    else
	mode = 1;

    if(vp->porta_control_ratio)
	return porta_resample_voice(v, countptr, mode);

    if(vp->vibrato_control_ratio)
	return vib_resample_voice(v, countptr, mode);

    return normal_resample_voice(v, countptr, mode);
}

#ifdef LOOKUP_HACK
#define MAX_DATAVAL 127
#define MIN_DATAVAL -128
#else
#define MAX_DATAVAL 32767
#define MIN_DATAVAL -32768
#endif

static void do_nlowpass(uint32 srate, sample_t *buf, uint32 count, int32 freq, FLOAT_T resonance)
{
	double A, B, C;
	sample_t pv1, pv2;
	int32 i;

	if (freq > srate * 2) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Lowpass: center must be < data rate*2");
		return;
	}
	A = 2.0 * M_PI * freq * 2.5 / srate;
	B = exp(-A / srate);
/* 900-1500 clips
	A *= 0.8;
	B *= 0.8;
*/
	C = 0;

	/* */
	if (resonance) {
		double a, b, c;
		int32 width;
		width = freq / 5;
		c = exp(-2.0 * M_PI * width / srate);
		b = -4.0 * c / (1+c) * cos(2.0 * M_PI * freq / srate);
		a = sqrt(1 - b * b / (4 * c)) * (1 - c);
		b = -b; c = -c;

		A += a * resonance;
		B += b;
		C = c;
	}
	/* */

/* some clipping w. 0.30, 0.25, 0.20, 0.15 */
#if 0
/* no reduction: 150-1500 clips  */
	A *= 0.10;
	B *= 0.10;
	C *= 0.10;
#endif
/* 0-66 clips
	A *= 0.30;
	B *= 0.30;
	C *= 0.30;
*/
/* no clips */
	A *= 0.25;
	B *= 0.25;
	C *= 0.25;

	pv1 = 0;
	pv2 = 0;
	for (i = 0; i < count; i++) {
		sample_t l = *buf;
		double d = A * l + B * pv1 + C * pv2;
		if (d > MAX_DATAVAL) {
			output_clips++;
			d = MAX_DATAVAL;
		}
		else if (d < MIN_DATAVAL) {
			output_clips++;
			d = MIN_DATAVAL;
		}
		pv2 = pv1;
		pv1 = *buf++ = (sample_t)d;
	}
}

sample_t *resample_voice(int v, uint32 *countptr)
{
    Voice *vp=&voice[v];
    sample_t *rs;

    rs = real_resample_voice(v, countptr);
    if(!(vp->sample->sample_rate) ||
	 !vp->sample->cutoff_freq ||
	 (dont_filter && !vp->sample->note_to_use) ) return rs;

    do_nlowpass(vp->sample->sample_rate, rs, *countptr,
	vp->sample->cutoff_freq, vp->sample->resonance);

    return rs;
}

void pre_resample(Sample * sp)
{
  double a, xdiff;
  int32 incr, ofs, newlen, count;
  int16 *newdata, *dest, *src = (int16 *)sp->data, *vptr;
  int32 v1, v2, v3, v4, i;
  static const char note_name[12][3] =
  {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };

  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " * pre-resampling for note %d (%s%d)",
	    sp->note_to_use,
	    note_name[sp->note_to_use % 12], (sp->note_to_use & 0x7F) / 12);

  if (sp->sample_rate == play_mode->rate && sp->root_freq == freq_table[(int)(sp->note_to_use)]) {
  	sp->sample_rate = 0;
	return;
  }

  a = ((double) (sp->sample_rate) * freq_table[(int) (sp->note_to_use)]) /
    ((double) (sp->root_freq) * play_mode->rate);
  /* if (a<1.0) return; */
  if(sp->data_length / a >= 0x7fffffffL)
  {
      /* Too large to compute */
      ctl->cmsg(CMSG_INFO, VERB_DEBUG, " *** Can't pre-resampling for note %d",
		sp->note_to_use);
      return;
  }
  newlen = (int32)(sp->data_length / a);
  count = (newlen >> FRACTION_BITS) - 1;
  ofs = incr = (sp->data_length - (1 << FRACTION_BITS)) / count;

  if((double)newlen + incr >= 0x7fffffffL)
  {
      /* Too large to compute */
      ctl->cmsg(CMSG_INFO, VERB_DEBUG, " *** Can't pre-resampling for note %d",
		sp->note_to_use);
      return;
  }

  dest = newdata = (int16 *)safe_malloc((newlen >> (FRACTION_BITS - 1)) + 2);

  if (--count)
    *dest++ = src[0];

  /* Since we're pre-processing and this doesn't have to be done in
     real-time, we go ahead and do the full sliding cubic interpolation. */
  count--;
  for(i = 0; i < count; i++)
    {
#ifdef tplussliding
      int32 v, v5;
#endif
      vptr = src + (ofs >> FRACTION_BITS);
      v1 = *(vptr - 1);
      v2 = *vptr;
      v3 = *(vptr + 1);
      v4 = *(vptr + 2);
#ifdef tplussliding
      v5 = v2 - v3;
      xdiff = FRSCALENEG(ofs & FRACTION_MASK, FRACTION_BITS);
/* this looks a little strange: v1 - v2 - v5 = v1 + v3 */
      v = (int32)(v2 + xdiff * (1.0/6.0) * (3 * (v3 - v5) - 2 * v1 - v4 +
       xdiff * (3 * (v1 - v2 - v5) + xdiff * (3 * v5 + v4 - v1))));
      if(v < -32768)
	  *dest++ = -32768;
      else if(v > 32767)
	  *dest++ = 32767;
      else
	  *dest++ = (int16)v;
#else
      xdiff = FRSCALENEG(ofs & FRACTION_MASK, FRACTION_BITS);
      *dest++ = v2 + (xdiff / 6.0) * (-2 * v1 - 3 * v2 + 6 * v3 - v4 +
      xdiff * (3 * (v1 - 2 * v2 + v3) + xdiff * (-v1 + 3 * (v2 - v3) + v4)));
#endif
      ofs += incr;
    }

  if (ofs & FRACTION_MASK)
    {
      v1 = src[ofs >> FRACTION_BITS];
      v2 = src[(ofs >> FRACTION_BITS) + 1];
      *dest++ = (int16)(v1 + (((v2 - v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
    }
  else
    *dest++ = src[ofs >> FRACTION_BITS];
  *dest++ = *(dest - 1) / 2;
  *dest++ = *(dest - 1) / 2;

  sp->data_length = newlen;
  sp->loop_start = (int32)(sp->loop_start / a);
  sp->loop_end = (int32)(sp->loop_end / a);
  free(sp->data);
  sp->data = (sample_t *) newdata;
  sp->sample_rate = 0;
}
