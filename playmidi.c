/*
	$Id$

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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

    playmidi.c -- random stuff in need of rearrangement

*/

#include <stdio.h>
#ifndef __WIN32__
#include <unistd.h>
#endif
#include <stdlib.h>

#if (defined(SUN) && defined(SYSV)) || defined(__WIN32__)
# include <string.h>
#else
#include <strings.h>
#endif

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#ifndef ADAGIO
#include "readmidi.h"
#include <sys/time.h>
#else
#include <memory.h>
#endif /* ADAGIO */
#include "output.h"
#include "mix.h"
#include "controls.h"

#include "tables.h"
/*<95GUI_MODIF_BEGIN>*/
#ifdef CHANNEL_EFFECT
extern void effect_ctrl_change( MidiEvent* pCurrentEvent );
extern void effect_ctrl_reset( int idChannel ) ;
int opt_effect = 0;
#endif /* CHANNEL_EFFECT */
/*<95GUI_MODIF_END>*/

#ifdef tplus
#define PORTAMENTO_TIME_TUNING		(1.0 / 5000.0)
#define PORTAMENTO_CONTROL_RATIO	256	/* controls per sec */
#endif

#ifdef __WIN32__
extern int intr;
#endif

#ifdef ADAGIO

/* Minimal control mode */
extern ControlMode dumb_control_mode;
#define DEFAULT_CONTROL_MODE &dumb_control_mode

ControlMode *ctl_list[]={
  &dumb_control_mode,
  0
};
ControlMode *ctl=DEFAULT_CONTROL_MODE;

int free_instruments_afterwards=0;

/**static char def_instr_name[256]="";**/
static unsigned max_polyphony = 0;
#endif

#ifdef tplus
int opt_realtime_playing = 0;
int opt_modulation_wheel = 1;
int opt_portamento = 1;
int opt_channel_pressure = 1;
int opt_overlap_voice_allow = 1;
int reduce_quality_flag=0;
#endif

#ifndef ADAGIO
Channel channel[MAXCHAN];
char drumvolume[MAXCHAN][MAXNOTE];
char drumpanpot[MAXCHAN][MAXNOTE];
char drumreverberation[MAXCHAN][MAXNOTE];
char drumchorusdepth[MAXCHAN][MAXNOTE];
#else /* ADAGIO */
static void read_seq(unsigned char *from, unsigned char *to);
Channel channel[MAX_TONE_VOICES];
#endif /* ADAGIO */

Voice voice[MAX_VOICES];

int
    voices=DEFAULT_VOICES;

int32
    control_ratio=0,
    amplification=DEFAULT_AMPLIFICATION;

FLOAT_T
    master_volume;

int32 drumchannels=DEFAULT_DRUMCHANNELS;
int adjust_panning_immediately=0;

int GM_System_On=0;
int XG_System_On=0;
int GS_System_On=0;

int XG_System_reverb_type;
int XG_System_chorus_type;
int XG_System_variation_type;

static int32 lost_notes, cut_notes, played_notes;
#ifdef CHANNEL_EFFECT
int32 common_buffer[AUDIO_BUFFER_SIZE*2], /* stereo samples */
             *buffer_pointer;
static uint32 buffered_count;

MidiEvent *event_list, *current_event;
uint32 sample_count;
uint32 current_sample;
#else
static int32 common_buffer[AUDIO_BUFFER_SIZE*2], /* stereo samples */
             *buffer_pointer;
static uint32 buffered_count;

static MidiEvent *event_list, *current_event;
static uint32 sample_count, current_sample;
#endif

#ifdef tplus
static void update_portamento_controls(int ch);
#endif

static void kill_note(int i);

static void adjust_amplification(void)
{ 
  /* master_volume = (double)(amplification) / 100.0L; */
  master_volume = (double)(amplification) / 200.0L;
}

static void adjust_master_volume(int32 vol)
{ 
  /* master_volume = (double)(vol*amplification) / 1638400.0L; */
  master_volume = (double)(vol*amplification) / 3276800.0L;
}

static void reset_voices(void)
{
  int i;
  for (i=0; i<MAX_VOICES; i++)
    voice[i].status=VOICE_FREE;
}

/* Process the Reset All Controllers event */
static void reset_controllers(int c)
{
  channel[c].volume=90; /* Some standard says, although the SCC docs say 0. */
  channel[c].expression=127; /* SCC-1 does this. */
  channel[c].sustain=0;
  channel[c].pitchbend=0x2000;
  channel[c].pitchfactor=0; /* to be computed */
#ifdef tplus
  channel[c].modulation_wheel = 0;
  channel[c].portamento_time_lsb = 0;
  channel[c].portamento_time_msb = 0;
  channel[c].tuning_lsb = 64;
  channel[c].tuning_msb = 64;
  channel[c].porta_control_ratio = 0;
  channel[c].portamento = 0;
  channel[c].last_note_fine = -1;

  channel[c].vibrato_ratio = 0;
  channel[c].vibrato_depth = 0;
  channel[c].vibrato_delay = 0;
/*
  memset(channel[c].envelope_rate, 0, sizeof(channel[c].envelope_rate));
*/
  update_portamento_controls(c);
#endif
#ifdef CHANNEL_EFFECT
  if (opt_effect) effect_ctrl_reset( c ) ;
#endif /* CHANNEL_EFFECT */
}

#ifndef ADAGIO
static void redraw_controllers(int c)
{
  ctl->volume(c, channel[c].volume);
  ctl->expression(c, channel[c].expression);
  ctl->sustain(c, channel[c].sustain);
  ctl->pitch_bend(c, channel[c].pitchbend);
}
#endif

static void reset_midi(void)
{
  int i;
#ifndef ADAGIO
  for (i=0; i<MAXCHAN; i++)
#else /* ADAGIO */
  for (i=0; i<MAX_VOICES; i++)
#endif /* ADAGIO */
    {
      reset_controllers(i);
      /* The rest of these are unaffected by the Reset All Controllers event */
      channel[i].program=default_program;
      channel[i].panning=NO_PANNING;
      channel[i].pitchsens=2;
      channel[i].bank=0; /* tone bank or drum set */
      channel[i].harmoniccontent=64,
      channel[i].releasetime=64,
      channel[i].attacktime=64,
      channel[i].brightness=64,
      /*channel[i].kit=0;*/
      channel[i].sfx=0;
      /* channel[i].transpose and .kit are initialized in readmidi.c */
    }
  reset_voices();
}

static void select_sample(int v, Instrument *ip)
{
  int32 f, cdiff, diff, midfreq;
  int s,i;
  Sample *sp, *closest;

  s=ip->samples;
  sp=ip->sample;

  if (s==1)
    {
      voice[v].sample=sp;
      return;
    }

  f=voice[v].orig_frequency;
  for (i=0; i<s; i++)
    {
      if (sp->low_freq <= f && sp->high_freq >= f)
	{
	  voice[v].sample=sp;
	  return;
	}
      sp++;
    }

  /* 
     No suitable sample found! We'll select the sample whose root
     frequency is closest to the one we want. (Actually we should
     probably convert the low, high, and root frequencies to MIDI note
     values and compare those.) */

  cdiff=0x7FFFFFFF;
  closest=sp=ip->sample;
  midfreq = (sp->low_freq + sp->high_freq) / 2;
  for(i=0; i<s; i++)
    {
      diff=sp->root_freq - f;
  /*  But the root freq. can perfectly well lie outside the keyrange
   *  frequencies, so let's try:
   */
      /* diff=midfreq - f; */
      if (diff<0) diff=-diff;
      if (diff<cdiff)
	{
	  cdiff=diff;
	  closest=sp;
	}
      sp++;
    }
  voice[v].sample=closest;
  return;
}

static void select_stereo_samples(int v, InstrumentLayer *lp)
{
  Instrument *ip;
  InstrumentLayer *nlp, *bestvel;
  int diffvel, midvel, mindiff;

/* select closest velocity */
  bestvel = lp;
  mindiff = 500;
  for (nlp = lp; nlp; nlp = nlp->next) {
	midvel = (nlp->hi + nlp->lo)/2;
	if (!midvel) diffvel = 127;
	else if (voice[v].velocity < nlp->lo || voice[v].velocity > nlp->hi)
		diffvel = 200;
	else diffvel = voice[v].velocity - midvel;
	if (diffvel < 0) diffvel = -diffvel;
	if (diffvel < mindiff) {
		mindiff = diffvel;
		bestvel = nlp;
	}
  }
  ip = bestvel->instrument;

  if (ip->right_sample) {
    ip->sample = ip->right_sample;
    ip->samples = ip->right_samples;
    select_sample(v, ip);
    voice[v].right_sample = voice[v].sample;
  }
  else voice[v].right_sample = 0;

  ip->sample = ip->left_sample;
  ip->samples = ip->left_samples;
  select_sample(v, ip);
}

#ifndef tplus
static
#endif
void recompute_freq(int v)
{
  int 
    sign=(voice[v].sample_increment < 0), /* for bidirectional loops */
    pb=channel[voice[v].channel].pitchbend;
  double a;
  int32 tuning = 0;
  
  if (!voice[v].sample->sample_rate)
    return;

#ifdef tplus
  if(!opt_modulation_wheel)
      voice[v].modulation_wheel = 0;
  if(!opt_portamento)
      voice[v].porta_control_ratio = 0;
  voice[v].vibrato_control_ratio = voice[v].orig_vibrato_control_ratio;
 
  if(voice[v].modulation_wheel > 0)
      {
	  voice[v].vibrato_control_ratio =
	      (int32)(play_mode->rate * MODULATION_WHEEL_RATE
		      / (2.0 * VIBRATO_SAMPLE_INCREMENTS));
	  voice[v].vibrato_delay = 0;
      }
#endif

#ifdef tplus
  if(voice[v].vibrato_control_ratio || voice[v].modulation_wheel > 0)
#else
  if (voice[v].vibrato_control_ratio)
#endif
    {
      /* This instrument has vibrato. Invalidate any precomputed
         sample_increments. */

      int i=VIBRATO_SAMPLE_INCREMENTS;
      while (i--)
	voice[v].vibrato_sample_increment[i]=0;
    }


#ifdef tplus
  /* fine: [0..128] => [-256..256]
   * 1 coarse = 256 fine (= 1 note)
   * 1 fine = 2^5 tuning
   */
  tuning = (((int32)channel[voice[v].channel].tuning_lsb - 0x40)
	    + 64 * ((int32)channel[voice[v].channel].tuning_msb - 0x40)) << 7;
#endif

#ifdef tplus
  if(!voice[v].porta_control_ratio)
  {
#endif
  if (pb==0x2000 || pb<0 || pb>0x3FFF)
    voice[v].frequency=voice[v].orig_frequency;
  else
    {
      pb-=0x2000;
      if (!(channel[voice[v].channel].pitchfactor))
	{
	  /* Damn. Somebody bent the pitch. */
	  int32 i=pb*channel[voice[v].channel].pitchsens + tuning;
	  if (pb<0)
	    i=-i;
	  channel[voice[v].channel].pitchfactor=
	    bend_fine[(i>>5) & 0xFF] * bend_coarse[i>>13];
	}
      if (pb>0)
	voice[v].frequency=
	  (int32)(channel[voice[v].channel].pitchfactor *
		  (double)(voice[v].orig_frequency));
      else
	voice[v].frequency=
	  (int32)((double)(voice[v].orig_frequency) /
		  channel[voice[v].channel].pitchfactor);
    }
#ifdef tplus
  }
  else /* Portament */
  {
      int32 i;
      FLOAT_T pf;

      pb -= 0x2000;

      i = pb * channel[voice[v].channel].pitchsens + (voice[v].porta_pb << 5) + tuning;

      if(i >= 0)
	  pf = bend_fine[(i>>5) & 0xFF] * bend_coarse[(i>>13) & 0x7F];
      else
      {
	  i = -i;
	  pf = 1.0 / (bend_fine[(i>>5) & 0xFF] * bend_coarse[(i>>13) & 0x7F]);
      }
      voice[v].frequency = (int32)((double)(voice[v].orig_frequency) * pf);
      /*voice[v].cache = NULL;*/
  }
#endif

  a = FRSCALE(((double)(voice[v].sample->sample_rate) *
	      (double)(voice[v].frequency)) /
	     ((double)(voice[v].sample->root_freq) *
	      (double)(play_mode->rate)),
	     FRACTION_BITS);

  /* what to do if incr is 0? --gl */
  /* if (!a) a++; */
  a += 0.5;
  if (!(int32)(a)) a = 1;

  if (sign) 
    a = -a; /* need to preserve the loop direction */

  voice[v].sample_increment = (int32)(a);
}

static int vcurve[128] = {
0,0,18,29,36,42,47,51,55,58,
60,63,65,67,69,71,73,74,76,77,
79,80,81,82,83,84,85,86,87,88,
89,90,91,92,92,93,94,95,95,96,
97,97,98,99,99,100,100,101,101,102,
103,103,104,104,105,105,106,106,106,107,
107,108,108,109,109,109,110,110,111,111,
111,112,112,112,113,113,114,114,114,115,
115,115,116,116,116,116,117,117,117,118,
118,118,119,119,119,119,120,120,120,120,
121,121,121,122,122,122,122,123,123,123,
123,123,124,124,124,124,125,125,125,125,
126,126,126,126,126,127,127,127
};

static void recompute_amp(int v)
{
  int32 tempamp;
  int chan = voice[v].channel;
  int vol = channel[chan].volume;
  int vel = vcurve[voice[v].velocity];


  /* TODO: use fscale */

  if (channel[chan].kit)
   {
    int note = voice[v].sample->note_to_use;
    if (note && drumvolume[chan][note]>=0) vol = drumvolume[chan][note];
   }

  tempamp= (vel *
	    vol * 
	    channel[chan].expression); /* 21 bits */

  if (!(play_mode->encoding & PE_MONO))
    {
      /*if (voice[v].panning > 60 && voice[v].panning < 68)*/
      if (voice[v].panning > 62 && voice[v].panning < 66)
	{
	  voice[v].panned=PANNED_CENTER;

	  voice[v].left_amp=
	    FRSCALENEG((double)(tempamp) * voice[v].sample->volume * master_volume,
		      21);
	}
      else if (voice[v].panning<5)
	{
	  voice[v].panned = PANNED_LEFT;

	  voice[v].left_amp=
	    FRSCALENEG((double)(tempamp) * voice[v].sample->volume * master_volume,
		      20);
	}
      else if (voice[v].panning>123)
	{
	  voice[v].panned = PANNED_RIGHT;

	  voice[v].left_amp= /* left_amp will be used */
	    FRSCALENEG((double)(tempamp) * voice[v].sample->volume * master_volume,
		      20);
	}
      else
	{
	  voice[v].panned = PANNED_MYSTERY;

	  voice[v].left_amp=
	    FRSCALENEG((double)(tempamp) * voice[v].sample->volume * master_volume,
		      27);
	  voice[v].right_amp=voice[v].left_amp * (voice[v].panning);
	  voice[v].left_amp *= (double)(127-voice[v].panning);
	}
    }
  else
    {
      voice[v].panned=PANNED_CENTER;

      voice[v].left_amp=
	FRSCALENEG((double)(tempamp) * voice[v].sample->volume * master_volume,
		  21);
    }
}

#if 0
/* If I use this (from tplus), I have to rebalance my pseudo-reverb. --gl */
static void recompute_amp(int v)
{
    FLOAT_T tempamp;

    tempamp = ((FLOAT_T)master_volume *
	       voice[v].velocity *
	       voice[v].sample->volume *
	       channel[voice[v].channel].volume *
	       channel[voice[v].channel].expression); /* 21 bits */

    if(!(play_mode->encoding & PE_MONO))
    {
	if(voice[v].panning > 60 && voice[v].panning < 68)
	{
	    voice[v].panned = PANNED_CENTER;
	    voice[v].left_amp = FRSCALENEG(tempamp, 21);
	}
	else if (voice[v].panning < 5)
	{
	    voice[v].panned = PANNED_LEFT;
	    voice[v].left_amp = FRSCALENEG(tempamp, 20);
	}
	else if(voice[v].panning > 123)
	{
	    voice[v].panned = PANNED_RIGHT;
	    /* left_amp will be used */
	    voice[v].left_amp =  FRSCALENEG(tempamp, 20);
	}
	else
	{
	    voice[v].panned = PANNED_MYSTERY;
	    voice[v].left_amp = FRSCALENEG(tempamp, 27);
	    voice[v].right_amp = voice[v].left_amp * voice[v].panning;
	    voice[v].left_amp *= (FLOAT_T)(127 - voice[v].panning);
	}
    }
    else
    {
	voice[v].panned = PANNED_CENTER;
	voice[v].left_amp = FRSCALENEG(tempamp, 21);
    }
}
#endif

static int current_polyphony = 0;

#define STEREO_CLONE 1
#define REVERB_CLONE 2
#define CHORUS_CLONE 3


/* just a variant of note_on() */
static int vc_alloc(int j)
{
  int i=voices; 

  while (i--)
    {
      if (i == j) continue;
      if (voice[i].status == VOICE_FREE) return i;
    }

  return -1;
}

static void kill_others(MidiEvent *e, int i)
{
  int j=voices; 

  /*if (!voice[i].sample->exclusiveClass) return; */

  while (j--)
    {
      if (voice[j].status == VOICE_FREE) continue;
      if (voice[j].status == VOICE_OFF) continue;
      if (voice[j].status == VOICE_DIE) continue;
      if (i == j) continue;
      if (voice[i].channel != voice[j].channel) continue;
      if (voice[j].sample->note_to_use)
      {
    	/* if (voice[j].sample->note_to_use != voice[i].sample->note_to_use) continue; */
	if (!voice[i].sample->exclusiveClass) continue;
    	if (voice[j].sample->exclusiveClass != voice[i].sample->exclusiveClass) continue;
        kill_note(j);
      }
     /*
      else if (voice[j].note != (e->a &0x07f)) continue;
      kill_note(j);
      */
    }
}

extern int reverb_options;

static void clone_voice(Instrument *ip, int v, MidiEvent *e, uint8 clone_type)
{
  int w, k, played_note, chorus, reverb;
  int chan = voice[v].channel;

  if (clone_type == STEREO_CLONE && !voice[v].right_sample) return;

  chorus = ip->sample->chorusdepth;
  reverb = ip->sample->reverberation;

  if (channel[chan].kit) {
/*
	if ((k=drumreverberation[chan][voice[v].note]) >= 0) reverb = (reverb+k)/2;
	if ((k=drumchorusdepth[chan][voice[v].note]) >= 0) chorus = (chorus+k)/2;
*/
	if ((k=drumreverberation[chan][voice[v].note]) >= 0) reverb = k;
	if ((k=drumchorusdepth[chan][voice[v].note]) >= 0) chorus = k;
  }
  else {
	if (channel[chan].chorusdepth) chorus = (chorus + channel[chan].chorusdepth)/2;
	if (channel[chan].reverberation) reverb = (reverb + channel[chan].reverberation)/2;
  }

  if (clone_type == REVERB_CLONE) chorus = 0;
  else if (clone_type == CHORUS_CLONE) reverb = 0;
  else if (clone_type == STEREO_CLONE) reverb = chorus = 0;

  if (reverb > 127) reverb = 127;
  if (chorus > 127) chorus = 127;

  if (clone_type == REVERB_CLONE) {
	 if ( (reverb_options & OPT_REVERB_EXTRA) && reverb < 90)
		reverb = 90;
	 if (reverb < 8 || reduce_quality_flag) return;
  }
  if (clone_type == CHORUS_CLONE) {
	 if ( (reverb_options & OPT_CHORUS_EXTRA) && chorus < 40)
		chorus = 40;
	 if (chorus < 4 || reduce_quality_flag) return;
  }

  if (!voice[v].right_sample) {
	if (voices - current_polyphony < 8) return;
	/*if ((reverb < 8 && chorus < 8) || !command_cutoff_allowed) return;*/
	/*if (!voice[v].vibrato_control_ratio) return;*/
	voice[v].right_sample = voice[v].sample;
  }
  /** else reverb = chorus = 0; **/

  /** if (!voice[v].right_sample) return; **/
  if ( (w = vc_alloc(v)) < 0 ) return;

  if (clone_type==STEREO_CLONE) voice[v].clone_voice = w;
  /* voice[w].clone_voice = -1; */
  voice[w].clone_voice = v;
  voice[w].clone_type = clone_type;
  voice[w].status = voice[v].status;
  voice[w].channel = voice[v].channel;
  voice[w].note = voice[v].note;
  voice[w].sample = voice[v].right_sample;
  voice[w].velocity= (e->b * (127 - voice[w].sample->attenuation)) / 127;
  voice[w].left_sample = voice[v].left_sample;
  voice[w].right_sample = voice[v].right_sample;
  voice[w].orig_frequency = voice[v].orig_frequency;
  voice[w].frequency = voice[v].frequency;
  voice[w].sample_offset = voice[v].sample_offset;
  voice[w].sample_increment = voice[v].sample_increment;
  voice[w].echo_delay = voice[v].echo_delay;
  voice[w].starttime = voice[v].starttime;
  voice[w].envelope_volume = voice[v].envelope_volume;
  voice[w].envelope_target = voice[v].envelope_target;
  voice[w].envelope_increment = voice[v].envelope_increment;
  voice[w].tremolo_sweep = voice[w].sample->tremolo_sweep_increment;
  voice[w].tremolo_sweep_position = voice[v].tremolo_sweep_position;
  voice[w].tremolo_phase = voice[v].tremolo_phase;
  voice[w].tremolo_phase_increment = voice[w].sample->tremolo_phase_increment;
  voice[w].vibrato_sweep = voice[w].sample->vibrato_sweep_increment;
  voice[w].vibrato_sweep_position = voice[v].vibrato_sweep_position;
  voice[w].left_mix = voice[v].left_mix;
  voice[w].right_mix = voice[v].right_mix;
  voice[w].left_amp = voice[v].left_amp;
  voice[w].right_amp = voice[v].right_amp;
  voice[w].tremolo_volume = voice[v].tremolo_volume;
  for (k = 0; k < VIBRATO_SAMPLE_INCREMENTS; k++)
    voice[w].vibrato_sample_increment[k] =
      voice[v].vibrato_sample_increment[k];
  for (k=0; k<6; k++)
    {
	voice[w].envelope_rate[k]=voice[v].envelope_rate[k];
	voice[w].envelope_offset[k]=voice[v].envelope_offset[k];
    }
  voice[w].vibrato_phase = voice[v].vibrato_phase;
  voice[w].vibrato_depth = voice[w].sample->vibrato_depth;
  voice[w].vibrato_control_ratio = voice[w].sample->vibrato_control_ratio;
  voice[w].vibrato_control_counter = voice[v].vibrato_control_counter;
  voice[w].envelope_stage = voice[v].envelope_stage;
  voice[w].control_counter = voice[v].control_counter;
  /*voice[w].panned = voice[v].panned;*/
  voice[w].panned = PANNED_RIGHT;
  voice[w].panning = 127;
  /*voice[w].panning = voice[w].sample->panning;*/
#ifdef tplus
  voice[w].porta_control_ratio = voice[v].porta_control_ratio;
  voice[w].porta_dpb = voice[v].porta_dpb;
  voice[w].porta_pb = voice[v].porta_pb;
  voice[w].vibrato_delay = 0;
#endif
  if (reverb) {
	int milli = play_mode->rate/1000;
	if (voice[w].panning < 64) voice[w].panning = 127;
	else voice[w].panning = 0;
	/*voice[w].velocity /= 2;*/
	voice[w].velocity = (voice[w].velocity * reverb) / 128;
	/**voice[w].velocity = (voice[w].velocity * reverb) / 256;**/
	/*voice[w].echo_delay = 50 * (play_mode->rate/1000);*/
	/**voice[w].echo_delay += (reverb>>2) * milli;**/
	voice[w].echo_delay += (reverb>>4) * milli;
/* 500, 250, 100, 50 too long; 25 pretty good */
	if (voice[w].echo_delay > 30*milli) voice[w].echo_delay = 30*milli;
	voice[w].envelope_rate[3] /= 2;
	if (XG_System_reverb_type >= 0) {
	    int subtype = XG_System_reverb_type & 0x07;
	    int rtype = XG_System_reverb_type >>3;
	    switch (rtype) {
		case 0: /* no effect */
		  break;
		case 1: /* hall */
		  if (subtype) voice[w].echo_delay += 3 * milli;
		  break;
		case 2: /* room */
		  voice[w].echo_delay /= 2;
		  break;
		case 3: /* stage */
		  voice[w].velocity = voice[v].velocity;
		  break;
		case 4: /* plate */
		  voice[w].panning = voice[v].panning;
		  break;
		case 16: /* white room */
		  voice[w].echo_delay = 0;
		  break;
		case 17: /* tunnel */
		  voice[w].echo_delay *= 2;
		  voice[w].velocity /= 2;
		  break;
		case 18: /* canyon */
		  voice[w].echo_delay *= 2;
		  break;
		case 19: /* basement */
		  voice[w].velocity /= 2;
		  break;
	        default: break;
	    }
	}
  }
  played_note = voice[w].sample->note_to_use;
  if (!played_note) played_note = e->a & 0x7f;
#ifdef ADAGIO
  if (voice[w].sample->freq_scale == 1024)
	played_note = played_note + voice[w].sample->freq_center - 60;
  else if (voice[w].sample->freq_scale)
#endif
    played_note = ( (played_note - voice[w].sample->freq_center) * voice[w].sample->freq_scale ) / 1024 +
		voice[w].sample->freq_center;
  voice[w].note = played_note;
  voice[w].orig_frequency = freq_table[played_note];
  if (chorus) {
	/*voice[w].orig_frequency += (voice[w].orig_frequency/128) * chorus;*/
/*fprintf(stderr, "voice %d v sweep from %ld (cr %d, depth %d)", w, voice[w].vibrato_sweep,
	 voice[w].vibrato_control_ratio, voice[w].vibrato_depth);*/
	voice[w].panning = voice[v].panning - 16;
	if (voice[w].panning < 0) voice[w].panning = 0;
	voice[v].panning += 16;
	if (voice[v].panning > 127) voice[v].panning = 127;

	if (!voice[w].vibrato_control_ratio) {
		voice[w].vibrato_control_ratio = 100;
		voice[w].vibrato_depth = 6;
		voice[w].vibrato_sweep = 74;
	}
	/*voice[w].velocity = (voice[w].velocity * chorus) / 128;*/
	voice[w].velocity = (voice[w].velocity * chorus) / (128+96);
	voice[v].velocity = voice[w].velocity;
	recompute_amp(v);
	voice[w].vibrato_sweep = chorus/2;
	voice[w].vibrato_depth /= 2;
	if (!voice[w].vibrato_depth) voice[w].vibrato_depth = 2;
	voice[w].vibrato_control_ratio /= 2;
	/*voice[w].vibrato_control_ratio += chorus;*/
/*fprintf(stderr, " to %ld (cr %d, depth %d).\n", voice[w].vibrato_sweep,
	 voice[w].vibrato_control_ratio, voice[w].vibrato_depth);
	voice[w].vibrato_phase = 20;*/
	if (XG_System_chorus_type >= 0) {
	    int subtype = XG_System_chorus_type & 0x07;
	    int chtype = XG_System_chorus_type >> 3;
	    switch (chtype) {
		case 0: /* no effect */
		  break;
		case 1: /* chorus */
		  if (subtype) voice[w].vibrato_depth *= 2;
		  break;
		case 2: /* celeste */
		  voice[w].orig_frequency += (voice[w].orig_frequency/128) * chorus;
		  voice[w].velocity = voice[v].velocity;
		  break;
		case 3: /* flanger */
		  voice[w].vibrato_sweep = chorus * 4;
		  break;
		case 4: /* symphonic : cf Children of the Night /128 bad, /1024 ok */
		  voice[w].orig_frequency += (voice[w].orig_frequency/512) * chorus;
		  voice[v].orig_frequency -= (voice[v].orig_frequency/512) * chorus;
		  recompute_freq(v);
		  break;
		case 8: /* phaser */
		  break;
	      default:
		  break;
	    }
	}
  }

  recompute_freq(w);
  recompute_amp(w);
  if (voice[w].sample->modes & MODES_ENVELOPE)
    {
      /* Ramp up from 0 */
      voice[w].envelope_stage=ATTACK;
      voice[w].envelope_volume=0;
      voice[w].control_counter=0;
      recompute_envelope(w);
      apply_envelope_to_amp(w);
    }
  else
    {
      voice[w].envelope_increment=0;
      apply_envelope_to_amp(w);
    }
}

static void start_note(MidiEvent *e, int i)
{
  InstrumentLayer *lp;
  Instrument *ip;
  int j, banknum;
  int played_note, drumpan=NO_PANNING;
  int32 rt;

  if (channel[e->channel].sfx) banknum=channel[e->channel].sfx;
  else banknum=channel[e->channel].bank;

  voice[i].velocity=e->b;

#ifndef ADAGIO
  if (channel[e->channel].kit)
    {
      if (!(lp=drumset[banknum]->tone[e->a].layer))
	{
	  if (!(lp=drumset[0]->tone[e->a].layer))
	    return; /* No instrument? Then we can't play. */
	}
      ip = lp->instrument;
      if (ip->type == INST_GUS && ip->samples != 1)
	{
	  ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, 
	       "Strange: percussion instrument with %d samples!", ip->samples);
	  return;
	}

      if (ip->sample->note_to_use) /* Do we have a fixed pitch? */
	{
	  voice[i].orig_frequency=freq_table[(int)(ip->sample->note_to_use)];
	  drumpan=drumpanpot[e->channel][(int)ip->sample->note_to_use];
	}
      else
	voice[i].orig_frequency=freq_table[e->a & 0x7F];
      
      /* drums are supposed to have only one sample */
      /* voice[i].sample=ip->sample; */
      /* voice[i].right_sample = ip->right_sample; */
    }
  else
    {
#endif
      if (channel[e->channel].program==SPECIAL_PROGRAM)
	lp=default_instrument;
      else if (!(lp=tonebank[banknum]->
		 tone[channel[e->channel].program].layer))
	{
	  if (!(lp=tonebank[0]->tone[channel[e->channel].program].layer))
	    return; /* No instrument? Then we can't play. */
	}
      ip = lp->instrument;

      if (ip->sample->note_to_use) /* Fixed-pitch instrument? */
	voice[i].orig_frequency=freq_table[(int)(ip->sample->note_to_use)];
      else
	voice[i].orig_frequency=freq_table[e->a & 0x7F];
      /* select_stereo_samples(i, lp); */
#ifndef ADAGIO
    } /* not drum channel */
#endif

    select_stereo_samples(i, lp);
    kill_others(e, i);

    voice[i].starttime = e->time;
    played_note = voice[i].sample->note_to_use;
    if (!played_note) played_note = e->a & 0x7f;
#ifdef ADAGIO
    if (voice[i].sample->freq_scale == 1024)
	played_note = played_note + voice[i].sample->freq_center - 60;
    else if (voice[i].sample->freq_scale)
#endif
    played_note = ( (played_note - voice[i].sample->freq_center) * voice[i].sample->freq_scale ) / 1024 +
		voice[i].sample->freq_center;
    voice[i].note = played_note;
    voice[i].orig_frequency = freq_table[played_note];

  if (voice[i].sample->modes & MODES_FAST_RELEASE) voice[i].status=VOICE_OFF;
  else
  voice[i].status=VOICE_ON;
  voice[i].channel=e->channel;
  voice[i].note=e->a;
  voice[i].velocity= (e->b * (127 - voice[i].sample->attenuation)) / 127;
#if 0
  voice[i].velocity=e->b;
#endif
  voice[i].sample_offset=0;
  voice[i].sample_increment=0; /* make sure it isn't negative */
#ifdef tplus
  voice[i].modulation_wheel=channel[e->channel].modulation_wheel;
#endif

  voice[i].tremolo_phase=0;
  voice[i].tremolo_phase_increment=voice[i].sample->tremolo_phase_increment;
  voice[i].tremolo_sweep=voice[i].sample->tremolo_sweep_increment;
  voice[i].tremolo_sweep_position=0;

  voice[i].vibrato_sweep=voice[i].sample->vibrato_sweep_increment;
  voice[i].vibrato_depth=voice[i].sample->vibrato_depth;
  voice[i].vibrato_sweep_position=0;
  voice[i].vibrato_control_ratio=voice[i].sample->vibrato_control_ratio;
#ifdef tplus
  if(channel[e->channel].vibrato_ratio && voice[i].vibrato_depth > 0)
  {
      voice[i].vibrato_control_ratio = channel[e->channel].vibrato_ratio;
      voice[i].vibrato_depth = channel[e->channel].vibrato_depth;
      voice[i].vibrato_delay = channel[e->channel].vibrato_delay;
  }
  else voice[i].vibrato_delay = 0;
#endif
  voice[i].vibrato_control_counter=voice[i].vibrato_phase=0;
  for (j=0; j<VIBRATO_SAMPLE_INCREMENTS; j++)
    voice[i].vibrato_sample_increment[j]=0;


#ifdef RATE_ADJUST_DEBUG
{
int e_debug=0, f_debug=0;
int32 r;
if (channel[e->channel].releasetime!=64) e_debug=1;
if (channel[e->channel].attacktime!=64) f_debug=1;
if (e_debug) printf("ADJ release time by %d on %d\n", channel[e->channel].releasetime -64, e->channel);
if (f_debug) printf("ADJ attack time by %d on %d\n", channel[e->channel].attacktime -64, e->channel);
#endif

  for (j=ATTACK; j<MAXPOINT; j++)
    {
	voice[i].envelope_rate[j]=voice[i].sample->envelope_rate[j];
	voice[i].envelope_offset[j]=voice[i].sample->envelope_offset[j];
#ifdef RATE_ADJUST_DEBUG
if (f_debug) printf("\trate %d = %ld; offset = %ld\n", j,
voice[i].envelope_rate[j],
voice[i].envelope_offset[j]);
#endif
    }

  voice[i].echo_delay=voice[i].envelope_rate[DELAY];

#ifdef RATE_ADJUST_DEBUG
if (e_debug) {
printf("(old rel time = %ld)\n",
(voice[i].envelope_offset[2] - voice[i].envelope_offset[3]) / voice[i].envelope_rate[3]);
r = voice[i].envelope_rate[3];
r = r + ( (64-channel[e->channel].releasetime)*r ) / 100;
voice[i].envelope_rate[3] = r;
printf("(new rel time = %ld)\n",
(voice[i].envelope_offset[2] - voice[i].envelope_offset[3]) / voice[i].envelope_rate[3]);
}
}
#endif

  if (channel[e->channel].attacktime!=64)
    {
	rt = voice[i].envelope_rate[1];
	rt = rt + ( (64-channel[e->channel].attacktime)*rt ) / 100;
	if (rt > 1000) voice[i].envelope_rate[1] = rt;
    }
  if (channel[e->channel].releasetime!=64)
    {
	rt = voice[i].envelope_rate[3];
	rt = rt + ( (64-channel[e->channel].releasetime)*rt ) / 100;
	if (rt > 1000) voice[i].envelope_rate[3] = rt;
    }

  if (channel[e->channel].panning != NO_PANNING)
    voice[i].panning=channel[e->channel].panning;
  else
    voice[i].panning=voice[i].sample->panning;
  if (drumpan != NO_PANNING)
    voice[i].panning=drumpan;
/* for now, ... */
  /*if (voice[i].right_sample) voice[i].panning = voice[i].sample->panning;*/
  if (voice[i].right_sample) voice[i].panning = 0;

#ifdef tplus
  if(channel[e->channel].portamento && !channel[e->channel].porta_control_ratio)
      update_portamento_controls(e->channel);
  if(channel[e->channel].porta_control_ratio)
  {
      if(channel[e->channel].last_note_fine == -1)
      {
	  /* first on */
	  channel[e->channel].last_note_fine = voice[i].note * 256;
	  channel[e->channel].porta_control_ratio = 0;
      }
      else
      {
	  voice[i].porta_control_ratio = channel[e->channel].porta_control_ratio;
	  voice[i].porta_dpb = channel[e->channel].porta_dpb;
	  voice[i].porta_pb = channel[e->channel].last_note_fine -
	      voice[i].note * 256;
	  if(voice[i].porta_pb == 0)
	      voice[i].porta_control_ratio = 0;
      }
  }

  /* What "cnt"? if(cnt == 0) 
      channel[e->channel].last_note_fine = voice[i].note * 256; */
#endif

  if (reverb_options & OPT_STEREO_EXTRA) {
    int pan = voice[i].panning;
    int disturb = 0;
    /* If they're close up (no reverb) and you are behind the pianist,
     * high notes come from the right, so we'll spread piano etc. notes
     * out horizontally according to their pitches.
     */
    if (channel[e->channel].program < 21) {
	    int n = voice[i].velocity - 32;
	    if (n < 0) n = 0;
	    if (n > 64) n = 64;
	    pan = pan/2 + n;
	}
    /* For other types of instruments, the music sounds more alive if
     * notes come from slightly different directions.  However, instruments
     * do drift around in a sometimes disconcerting way, so the following
     * might not be such a good idea.
     */
    else disturb = (voice[i].velocity/32 % 8) +
	(voice[i].note % 8); /* /16? */

    if (pan < 64) pan += disturb;
    else pan -= disturb;
    if (pan < 0) pan = 0;
    else if (pan > 127) pan = 127;
    voice[i].panning = pan;
  }

  recompute_freq(i);
  recompute_amp(i);
  if (voice[i].sample->modes & MODES_ENVELOPE)
    {
      /* Ramp up from 0 */
      voice[i].envelope_stage=ATTACK;
      voice[i].envelope_volume=0;
      voice[i].control_counter=0;
      recompute_envelope(i);
      apply_envelope_to_amp(i);
    }
  else
    {
      voice[i].envelope_increment=0;
      apply_envelope_to_amp(i);
    }
  voice[i].clone_voice = -1;
  voice[i].clone_type = 0;
  if (reverb_options & OPT_STEREO_VOICE) clone_voice(ip, i, e, STEREO_CLONE);
  if (reverb_options & OPT_REVERB_VOICE) clone_voice(ip, i, e, REVERB_CLONE);
  if (reverb_options & OPT_CHORUS_VOICE) clone_voice(ip, i, e, CHORUS_CLONE);
  rt = voice[i].status;
  voice[i].status=VOICE_ON;
  ctl->note(i);
  voice[i].status=rt;
  played_notes++;
}

static void kill_note(int i)
{
  voice[i].status=VOICE_DIE;
  if (voice[i].clone_voice >= 0)
	voice[ voice[i].clone_voice ].status=VOICE_DIE;
  ctl->note(i);
}

/* Only one instance of a note can be playing on a single channel. */
static void note_on(MidiEvent *e)
{
  int i=voices, lowest=-1; 
  int32 lv=0x7FFFFFFF, v;

  current_polyphony = 0;

  reduce_quality_flag = (output_buffer_full < 1);

  if (!reduce_quality_flag)
  while (i--)
    {
      if (voice[i].status == VOICE_FREE)
	lowest=i; /* Can't get a lower volume than silence */
	else current_polyphony++;
    }

#ifdef tplus
  /* reduce_quality_flag = (current_polyphony > voices / 2); */
#endif

#ifdef ADAGIO
  if (current_polyphony <= max_polyphony)
#endif
  if (lowest != -1)
    {
      /* Found a free voice. */
      start_note(e,lowest);
      return;
    }
 
  /* Look for the decaying note with the lowest volume */
  i=voices;
  while (i--)
    {
      if ((voice[i].status!=VOICE_ON) &&
	  (voice[i].status!=VOICE_FREE) &&
	  (voice[i].status!=VOICE_DIE))
	{
	  v=voice[i].left_mix;
	  if ((voice[i].panned==PANNED_MYSTERY) && (voice[i].right_mix>v))
	    v=voice[i].right_mix;
	  if (v<lv)
	    {
	      lv=v;
	      lowest=i;
	    }
	}
    }
 
  /* Look for the decaying note with the lowest volume */
  if (lowest==-1)
   {
   i=voices;
   while (i--)
    {
      if ((voice[i].status!=VOICE_ON) &&
	  (!voice[i].clone_type))
	{
	  v=voice[i].left_mix;
	  if ((voice[i].panned==PANNED_MYSTERY) && (voice[i].right_mix>v))
	    v=voice[i].right_mix;
	  if (v<lv)
	    {
	      lv=v;
	      lowest=i;
	    }
	}
    }
   }

  if (lowest != -1)
    {
      int cl = voice[lowest].clone_voice;
      /* This can still cause a click, but if we had a free voice to
	 spare for ramping down this note, we wouldn't need to kill it
	 in the first place... Still, this needs to be fixed. Perhaps
	 we could use a reserve of voices to play dying notes only. */
      
      cut_notes++;
      voice[lowest].status=VOICE_FREE;
      if (cl >= 0) {
	/** if (voice[lowest].velocity == voice[cl].velocity) **/
	if (voice[cl].clone_type==STEREO_CLONE || (!voice[cl].clone_type && voice[lowest].clone_type==STEREO_CLONE))
	   voice[cl].status=VOICE_FREE;
	else if (voice[cl].clone_voice==lowest) voice[cl].clone_voice=-1;
      }
      ctl->note(lowest);
#ifdef ADAGIO
  if (current_polyphony <= max_polyphony)
#endif
      start_note(e,lowest);
#ifdef ADAGIO
      else cut_notes++;
#endif
    }
  else
    lost_notes++;
}

static void finish_note(int i)
{
  if (voice[i].sample->modes & MODES_FAST_RELEASE) return;
  if (voice[i].sample->modes & MODES_ENVELOPE)
  /**if ((voice[i].sample->modes & MODES_ENVELOPE) && !(voice[i].sample->modes & MODES_FAST_RELEASE)) **/
    {
      /* We need to get the envelope out of Sustain stage */
     if (voice[i].envelope_stage < RELEASE)
      voice[i].envelope_stage=RELEASE;
      voice[i].status=VOICE_OFF;
      recompute_envelope(i);
      apply_envelope_to_amp(i);
      ctl->note(i);
    }
  else
    {
      /* Set status to OFF so resample_voice() will let this voice out
         of its loop, if any. In any case, this voice dies when it
         hits the end of its data (ofs>=data_length). */
      voice[i].status=VOICE_OFF;
    }
  { int v;
    if ( (v=voice[i].clone_voice) >= 0)
      {
	voice[i].clone_voice = -1;
	voice[v].clone_voice = -1;
        finish_note(v);
      }
  }
}

static void note_off(MidiEvent *e)
{
  int i=voices;
  while (i--)
    if (voice[i].status==VOICE_ON &&
	voice[i].channel==e->channel &&
	voice[i].note==e->a)
      {
	if (channel[e->channel].sustain)
	  {
	    voice[i].status=VOICE_SUSTAINED;
	    ctl->note(i);
	  }
	else
	  finish_note(i);
/** #ifndef ADAGIO
	return; **/
/** #endif **/
      }
}

#ifndef ADAGIO
/* Process the All Notes Off event */
static void all_notes_off(int c)
{
  int i=voices;
  ctl->cmsg(CMSG_INFO, VERB_DEBUG, "All notes off on channel %d", c);
  while (i--)
    if (voice[i].status==VOICE_ON &&
	voice[i].channel==c)
      {
	if (channel[c].sustain) 
	  {
	    voice[i].status=VOICE_SUSTAINED;
	    ctl->note(i);
	  }
	else
	  finish_note(i);
      }
}
#endif /* ADAGIO */

#ifndef ADAGIO
/* Process the All Sounds Off event */
static void all_sounds_off(int c)
{
  int i=voices;
  while (i--)
    if (voice[i].channel==c && 
	voice[i].status != VOICE_FREE &&
	voice[i].status != VOICE_DIE)
      {
	kill_note(i);
      }
}
#endif

static void adjust_pressure(MidiEvent *e)
{
  int i=voices;
  while (i--)
    if (voice[i].status==VOICE_ON &&
	voice[i].channel==e->channel &&
	voice[i].note==e->a)
      {
	voice[i].velocity=e->b;
	recompute_amp(i);
	apply_envelope_to_amp(i);
      }
}

#ifdef tplus
static void adjust_channel_pressure(MidiEvent *e)
{
    if(opt_channel_pressure)
    {
	int i=voices;
	int ch, pressure;

	ch = e->channel;
	pressure = e->a;
        while (i--)
	    if(voice[i].status == VOICE_ON && voice[i].channel == ch)
	    {
		voice[i].velocity = pressure;
		recompute_amp(i);
		apply_envelope_to_amp(i);
	    }
    }
}
#endif

static void adjust_panning(int c)
{
  int i=voices;
  while (i--)
    if ((voice[i].channel==c) &&
	(voice[i].status==VOICE_ON || voice[i].status==VOICE_SUSTAINED))
      {
	if (voice[i].right_sample) continue;
	voice[i].panning=channel[c].panning;
	recompute_amp(i);
	apply_envelope_to_amp(i);
      }
}

#ifndef ADAGIO
static void drop_sustain(int c)
{
  int i=voices;
  while (i--)
    if (voice[i].status==VOICE_SUSTAINED && voice[i].channel==c)
      finish_note(i);
}
#endif /* ADAGIO */

static void adjust_pitchbend(int c)
{
  int i=voices;
  while (i--)
    if (voice[i].status!=VOICE_FREE && voice[i].channel==c)
      {
	recompute_freq(i);
      }
}

static void adjust_volume(int c)
{
  int i=voices;
  while (i--)
    if (voice[i].channel==c &&
	(voice[i].status==VOICE_ON || voice[i].status==VOICE_SUSTAINED))
      {
	recompute_amp(i);
	apply_envelope_to_amp(i);
      }
}

#ifdef tplus
static int32 midi_cnv_vib_rate(int rate)
{
    return (int32)((double)play_mode->rate * MODULATION_WHEEL_RATE
		   / (midi_time_table[rate] *
		      2.0 * VIBRATO_SAMPLE_INCREMENTS));
}

static int midi_cnv_vib_depth(int depth)
{
    return (int)(depth * VIBRATO_DEPTH_TUNING);
}

static int32 midi_cnv_vib_delay(int delay)
{
    return (int32)(midi_time_table[delay]);
}
#endif

#ifndef ADAGIO
static int xmp_epoch = -1;
static unsigned xxmp_epoch = 0;
static unsigned time_expired = 0;
static unsigned last_time_expired = 0;
extern int gettimeofday(struct timeval *, struct timezone *);
static struct timeval tv;
static struct timezone tz;
static void time_sync(int32 resync)
{
	unsigned jiffies;

	if (resync >= 0) {
		last_time_expired = resync;
		xmp_epoch = -1;
		xxmp_epoch = 0; 
		time_expired = 0;
		/*return;*/
	}
	gettimeofday (&tv, &tz);
	if (xmp_epoch < 0) {
		xxmp_epoch = tv.tv_sec;
		xmp_epoch = tv.tv_usec;
	}
	jiffies = (tv.tv_sec - xxmp_epoch)*100 + (tv.tv_usec - xmp_epoch)/10000;
	time_expired = (jiffies * play_mode->rate)/100 + last_time_expired;
}


static void show_markers(int32 until_time)
{
    static struct meta_text_type *meta;
    char buf[1024];
    int i, j, len;

    if (!meta_text_list) return;

    if (until_time >= 0) {
	time_sync(until_time);
	for (meta = meta_text_list; meta && meta->time < until_time; meta = meta->next) ;
	return;
    }

    if (!time_expired) meta = meta_text_list;

    time_sync(-1);

    buf[0] = '\0';
    len = 0;
    while (meta)
	if (meta->time <= time_expired) {
	    len += strlen(meta->text);
	    if (len < 100) strcat(buf, meta->text);
	    if (!meta->next) strcat(buf, " \n");
	    meta = meta->next;
	}
	else break;
    len = strlen(buf);
    for (i = 0, j = 0; j < 1024 && j < len; j++)
	if (buf[j] == '\n') {
	    buf[j] = '\0';
	    if (j - i > 0) ctl->cmsg(CMSG_INFO, VERB_ALWAYS, buf + i);
	    else ctl->cmsg(CMSG_INFO, VERB_ALWAYS, " ");
	    i = j + 1;
	}
    if (i < j) ctl->cmsg(CMSG_INFO, VERB_ALWAYS, "~%s", buf + i);
}
#endif

#ifndef ADAGIO
static void seek_forward(int32 until_time)
{
  reset_voices();
  show_markers(until_time);
  while (current_event->time < until_time)
    {
      switch(current_event->type)
	{
	  /* All notes stay off. Just handle the parameter changes. */

	case ME_PITCH_SENS:
	  channel[current_event->channel].pitchsens=
	    current_event->a;
	  channel[current_event->channel].pitchfactor=0;
	  break;
	  
	case ME_PITCHWHEEL:
	  channel[current_event->channel].pitchbend=
	    current_event->a + current_event->b * 128;
	  channel[current_event->channel].pitchfactor=0;
	  break;
#ifdef tplus
	    case ME_MODULATION_WHEEL:
	      channel[current_event->channel].modulation_wheel =
		    midi_cnv_vib_depth(current_event->a);
	      break;

            case ME_PORTAMENTO_TIME_MSB:
	      channel[current_event->channel].portamento_time_msb = current_event->a;
	      break;

            case ME_PORTAMENTO_TIME_LSB:
	      channel[current_event->channel].portamento_time_lsb = current_event->a;
	      break;

            case ME_PORTAMENTO:
	      channel[current_event->channel].portamento = (current_event->a >= 64);
	      break;

            case ME_FINE_TUNING:
	      channel[current_event->channel].tuning_lsb = current_event->a;
	      break;

            case ME_COARSE_TUNING:
	      channel[current_event->channel].tuning_msb = current_event->a;
	      break;

      	    case ME_CHANNEL_PRESSURE:
	      /*adjust_channel_pressure(current_event);*/
	      break;

#endif
	  
	case ME_MAINVOLUME:
	  channel[current_event->channel].volume=current_event->a;
	  break;
	  
	case ME_MASTERVOLUME:
	  adjust_master_volume(current_event->a + (current_event->b <<7));
	  break;
	  
	case ME_PAN:
	  channel[current_event->channel].panning=current_event->a;
	  break;
	      
	case ME_EXPRESSION:
	  channel[current_event->channel].expression=current_event->a;
	  break;
	  
	case ME_PROGRAM:
  	  if (channel[current_event->channel].kit)
	    /* Change drum set */
	    channel[current_event->channel].bank=current_event->a;
	  else
	    channel[current_event->channel].program=current_event->a;
	  break;

	case ME_SUSTAIN:
	  channel[current_event->channel].sustain=current_event->a;
	  break;

	case ME_REVERBERATION:
#ifdef CHANNEL_EFFECT
	 if (opt_effect)
	  effect_ctrl_change( current_event ) ;
	 else
#endif
	  channel[current_event->channel].reverberation=current_event->a;
	  break;

	case ME_CHORUSDEPTH:
#ifdef CHANNEL_EFFECT
	 if (opt_effect)
	  effect_ctrl_change( current_event ) ;
	 else
#endif
	  channel[current_event->channel].chorusdepth=current_event->a;
	  break;

#ifdef CHANNEL_EFFECT
	case ME_CELESTE:
	 if (opt_effect)
	  effect_ctrl_change( current_event ) ;
	  break;
	case ME_PHASER:
	 if (opt_effect)
	  effect_ctrl_change( current_event ) ;
	  break;
#endif

	case ME_HARMONICCONTENT:
	  channel[current_event->channel].harmoniccontent=current_event->a;
	  break;

	case ME_RELEASETIME:
	  channel[current_event->channel].releasetime=current_event->a;
	  break;

	case ME_ATTACKTIME:
	  channel[current_event->channel].attacktime=current_event->a;
	  break;
#ifdef tplus
	case ME_VIBRATO_RATE:
	  channel[current_event->channel].vibrato_ratio=midi_cnv_vib_rate(current_event->a);
	  break;
	case ME_VIBRATO_DEPTH:
	  channel[current_event->channel].vibrato_depth=midi_cnv_vib_depth(current_event->a);
	  break;
	case ME_VIBRATO_DELAY:
	  channel[current_event->channel].vibrato_delay=midi_cnv_vib_delay(current_event->a);
	  break;
#endif
	case ME_BRIGHTNESS:
	  channel[current_event->channel].brightness=current_event->a;
	  break;

	case ME_RESET_CONTROLLERS:
	  reset_controllers(current_event->channel);
	  break;
	      
	case ME_TONE_BANK:
	  channel[current_event->channel].bank=current_event->a;
	  break;
	  
	case ME_TONE_KIT:
	  if (current_event->a==SFX_BANKTYPE)
		{
		    channel[current_event->channel].sfx=SFXBANK;
		    channel[current_event->channel].kit=0;
		}
	  else
		{
		    channel[current_event->channel].sfx=0;
		    channel[current_event->channel].kit=current_event->a;
		}
	  break;
	  
	case ME_EOT:
	  current_sample=current_event->time;
	  return;
	}
      current_event++;
    }
  /*current_sample=current_event->time;*/
  if (current_event != event_list)
    current_event--;
  current_sample=until_time;
}
#endif /* ADAGIO */

static void skip_to(int32 until_time)
{
  if (current_sample > until_time)
    current_sample=0;

  reset_midi();
  buffered_count=0;
  buffer_pointer=common_buffer;
  current_event=event_list;

#ifndef ADAGIO
  if (until_time)
    seek_forward(until_time);
  else show_markers(until_time);
  ctl->reset();
#endif /* ADAGIO */
}

#ifdef ADAGIO
/* excerpted from below */
void change_amplification(int val)
{ int i;
  amplification = val;
  if (amplification > MAX_AMPLIFICATION)
    amplification=MAX_AMPLIFICATION;
  adjust_amplification();
  for (i=0; i<voices; i++)
    if (voice[i].status != VOICE_FREE) {
      recompute_amp(i);
      apply_envelope_to_amp(i);
  }
}
#endif

#ifndef ADAGIO
static int apply_controls(void)
{
  int rc, i, did_skip=0;
  int32 val;
  /* ASCII renditions of CD player pictograms indicate approximate effect */
  do
    switch(rc=ctl->read(&val))
      {
      case RC_QUIT: /* [] */
      case RC_LOAD_FILE:	  
      case RC_NEXT: /* >>| */
      case RC_REALLY_PREVIOUS: /* |<< */
#ifdef KMIDI
      case RC_PATCHCHANGE:
      case RC_CHANGE_VOICES:
#endif
        play_mode->flush_output();
	play_mode->purge_output();
	return rc;
	
      case RC_CHANGE_VOLUME:
	if (val>0 || amplification > -val)
	  amplification += val;
	else 
	  amplification=0;
	if (amplification > MAX_AMPLIFICATION)
	  amplification=MAX_AMPLIFICATION;
	adjust_amplification();
	for (i=0; i<voices; i++)
	  if (voice[i].status != VOICE_FREE)
	    {
	      recompute_amp(i);
	      apply_envelope_to_amp(i);
	    }
	ctl->master_volume(amplification);
	break;

      case RC_PREVIOUS: /* |<< */
	play_mode->purge_output();
	if (current_sample < 2*play_mode->rate)
	  return RC_REALLY_PREVIOUS;
	return RC_RESTART;

      case RC_RESTART: /* |<< */
	skip_to(0);
	play_mode->purge_output();
	did_skip=1;
	break;
	
      case RC_JUMP:
	play_mode->purge_output();
	if (val >= sample_count)
	  return RC_NEXT;
	skip_to(val);
	return rc;
	
      case RC_FORWARD: /* >> */
	/*play_mode->purge_output();*/
	if (val+current_sample >= sample_count)
	  return RC_NEXT;
	skip_to(val+current_sample);
	did_skip=1;
	break;
	
      case RC_BACK: /* << */
	/*play_mode->purge_output();*/
	if (current_sample > val)
	  skip_to(current_sample-val);
	else
	  skip_to(0); /* We can't seek to end of previous song. */
	did_skip=1;
	break;
      }
  while (rc!= RC_NONE);
 
  /* Advertise the skip so that we stop computing the audio buffer */
  if (did_skip)
    return RC_JUMP; 
  else
    return rc;
}
#endif /* ADAGIO */

#ifdef CHANNEL_EFFECT
extern void (*do_compute_data)(uint32) ;
#else
static void do_compute_data(uint32 count)
{
  int i;
  if (!count) return; /* (gl) */
  memset(buffer_pointer, 0, 
	 (play_mode->encoding & PE_MONO) ? (count * 4) : (count * 8));
  for (i=0; i<voices; i++)
    {
      if(voice[i].status != VOICE_FREE)
	{
	  if (!voice[i].sample_offset && voice[i].echo_delay)
	    {
		if (voice[i].echo_delay >= count) voice[i].echo_delay -= count;
		else
		  {
	            mix_voice(buffer_pointer+voice[i].echo_delay, i, count-voice[i].echo_delay);
		    voice[i].echo_delay = 0;
		  }
	    }
	  else mix_voice(buffer_pointer, i, count);
	}
    }
  current_sample += count;
}
#endif /*CHANNEL_EFFECT*/

/* count=0 means flush remaining buffered data to output device, then
   flush the device itself */
static int compute_data(uint32 count)
{
#ifndef ADAGIO
  int rc;
#endif /* not ADAGIO */
  if (!count)
    {
      if (buffered_count)
	play_mode->output_data(common_buffer, buffered_count);
      play_mode->flush_output();
      buffer_pointer=common_buffer;
      buffered_count=0;
      return RC_NONE;
    }

  while ((count+buffered_count) >= AUDIO_BUFFER_SIZE)
    {
      do_compute_data(AUDIO_BUFFER_SIZE-buffered_count);
      count -= AUDIO_BUFFER_SIZE-buffered_count;
      play_mode->output_data(common_buffer, AUDIO_BUFFER_SIZE);
      buffer_pointer=common_buffer;
      buffered_count=0;
      
      ctl->current_time(current_sample);
#ifndef ADAGIO
      show_markers(-1);
      if ((rc=apply_controls())!=RC_NONE)
	return rc;
#endif /* not ADAGIO */
    }
  if (count>0)
    {
      do_compute_data(count);
      buffered_count += count;
      buffer_pointer += (play_mode->encoding & PE_MONO) ? count : count*2;
    }
  return RC_NONE;
}

#ifdef tplus
static void update_modulation_wheel(int ch, int val)
{
    int i, uv = /*upper_*/voices;
    for(i = 0; i < uv; i++)
	if(voice[i].status != VOICE_FREE && voice[i].channel == ch && voice[i].modulation_wheel != val)
	{
	    /* Set/Reset mod-wheel */
	    voice[i].modulation_wheel = val;
	    voice[i].vibrato_delay = 0;
	    recompute_freq(i);
	}
}

static void drop_portamento(int ch)
{
    int i, uv = /*upper_*/voices;

    channel[ch].porta_control_ratio = 0;
    for(i = 0; i < uv; i++)
	if(voice[i].status != VOICE_FREE &&
	   voice[i].channel == ch &&
	   voice[i].porta_control_ratio)
	{
	    voice[i].porta_control_ratio = 0;
	    recompute_freq(i);
	}
    channel[ch].last_note_fine = -1;
}

static void update_portamento_controls(int ch)
{
    if(!channel[ch].portamento ||
       (channel[ch].portamento_time_msb | channel[ch].portamento_time_lsb)
       == 0)
	drop_portamento(ch);
    else
    {
	double mt, dc;
	int d;

	mt = midi_time_table[channel[ch].portamento_time_msb & 0x7F] *
	    midi_time_table2[channel[ch].portamento_time_lsb & 0x7F] *
		PORTAMENTO_TIME_TUNING;
	dc = play_mode->rate * mt;
	d = (int)(1.0 / (mt * PORTAMENTO_CONTROL_RATIO));
	d++;
	channel[ch].porta_control_ratio = (int)(d * dc + 0.5);
	channel[ch].porta_dpb = d;
    }
}

static void update_portamento_time(int ch)
{
    int i, uv = /*upper_*/voices;
    int dpb;
    int32 ratio;

    update_portamento_controls(ch);
    dpb = channel[ch].porta_dpb;
    ratio = channel[ch].porta_control_ratio;

    for(i = 0; i < uv; i++)
    {
	if(voice[i].status != VOICE_FREE &&
	   voice[i].channel == ch &&
	   voice[i].porta_control_ratio)
	{
	    voice[i].porta_control_ratio = ratio;
	    voice[i].porta_dpb = dpb;
	    recompute_freq(i);
	}
    }
}

static void update_channel_freq(int ch)
{
    int i, uv = /*upper_*/voices;
    for(i = 0; i < uv; i++)
	if(voice[i].status != VOICE_FREE && voice[i].channel == ch)
	    recompute_freq(i);
}
#endif

#ifndef ADAGIO
int play_midi(MidiEvent *eventlist, uint32 events, uint32 samples)
{
  int rc;
#ifdef KMIDI
  int32 whensaytime;
#endif


  adjust_amplification();

  sample_count=samples;
  event_list=eventlist;
  lost_notes=cut_notes=played_notes=output_clips=0;

  skip_to(0);
#ifdef KMIDI
  whensaytime = current_event->time;
#endif

  for (;;)
    {
#ifdef __WIN32__
		/* check break signals */ 
		if (intr)
			return RC_QUIT;
#endif
      /* Handle all events that should happen at this time */
      while (current_event->time <= current_sample)
	{
#ifdef KMIDI
	  if (whensaytime + 400 > current_event->time && current_event->time + 400 < current_sample)
	  {
		ctl->current_time(current_event->time);
		whensaytime = current_event->time;
	  }
#endif
	  switch(current_event->type)
	    {

	      /* Effects affecting a single note */

	    case ME_NOTEON:
#ifdef tplus
#if 0
	      if (channel[current_event->channel].portamento &&
		    current_event->b &&
			(channel[current_event->channel].portamento_time_msb|
			 channel[current_event->channel].portamento_time_lsb )) break;
#endif
#endif
	      current_event->a += channel[current_event->channel].transpose;
	      if (!(current_event->b)) /* Velocity 0? */
		note_off(current_event);
	      else
		note_on(current_event);
	      break;

	    case ME_NOTEOFF:
	      current_event->a += channel[current_event->channel].transpose;
	      note_off(current_event);
	      break;

	    case ME_KEYPRESSURE:
	      adjust_pressure(current_event);
	      break;

	      /* Effects affecting a single channel */

	    case ME_PITCH_SENS:
	      channel[current_event->channel].pitchsens=
		current_event->a;
	      channel[current_event->channel].pitchfactor=0;
	      break;
	      
	    case ME_PITCHWHEEL:
	      channel[current_event->channel].pitchbend=
		current_event->a + current_event->b * 128;
	      channel[current_event->channel].pitchfactor=0;
	      /* Adjust pitch for notes already playing */
	      adjust_pitchbend(current_event->channel);
	      ctl->pitch_bend(current_event->channel, 
			      channel[current_event->channel].pitchbend);
	      break;
#ifdef tplus
	    case ME_MODULATION_WHEEL:
	      channel[current_event->channel].modulation_wheel =
		    midi_cnv_vib_depth(current_event->a);
	      update_modulation_wheel(current_event->channel, channel[current_event->channel].modulation_wheel);
		/*ctl_mode_event(CTLE_MOD_WHEEL, 1, current_event->channel, channel[current_event->channel].modulation_wheel);*/
		/*ctl->cmsg(CMSG_INFO, VERB_NORMAL, "~m%u", midi_cnv_vib_depth(current_event->a));*/
	      break;

            case ME_PORTAMENTO_TIME_MSB:
	      channel[current_event->channel].portamento_time_msb = current_event->a;
	      update_portamento_time(current_event->channel);
	      break;

            case ME_PORTAMENTO_TIME_LSB:
	      channel[current_event->channel].portamento_time_lsb = current_event->a;
	      update_portamento_time(current_event->channel);
	      break;

            case ME_PORTAMENTO:
	      channel[current_event->channel].portamento = (current_event->a >= 64);
	      if(!channel[current_event->channel].portamento) drop_portamento(current_event->channel);
		/*ctl->cmsg(CMSG_INFO, VERB_NORMAL, "~P%d",current_event->a);*/
	      break;

            case ME_FINE_TUNING:
	      channel[current_event->channel].tuning_lsb = current_event->a;
		/*ctl->cmsg(CMSG_INFO, VERB_NORMAL, "~t");*/
	      break;

            case ME_COARSE_TUNING:
	      channel[current_event->channel].tuning_msb = current_event->a;
		/*ctl->cmsg(CMSG_INFO, VERB_NORMAL, "~T");*/
	      break;

      	    case ME_CHANNEL_PRESSURE:
	      adjust_channel_pressure(current_event);
	      break;

#endif
	    case ME_MAINVOLUME:
	      channel[current_event->channel].volume=current_event->a;
	      adjust_volume(current_event->channel);
	      ctl->volume(current_event->channel, current_event->a);
	      break;
	      
	    case ME_MASTERVOLUME:
	      adjust_master_volume(current_event->a + (current_event->b <<7));
	      break;
	      
	    case ME_REVERBERATION:
#ifdef CHANNEL_EFFECT
	 if (opt_effect)
	      effect_ctrl_change( current_event ) ;
	 else
#endif
	      channel[current_event->channel].reverberation=current_event->a;
	      break;

	    case ME_CHORUSDEPTH:
#ifdef CHANNEL_EFFECT
	 if (opt_effect)
	      effect_ctrl_change( current_event ) ;
	 else
#endif
	      channel[current_event->channel].chorusdepth=current_event->a;
	      break;

#ifdef CHANNEL_EFFECT
	case ME_CELESTE:
	 if (opt_effect)
	  effect_ctrl_change( current_event ) ;
	  break;
	case ME_PHASER:
	 if (opt_effect)
	  effect_ctrl_change( current_event ) ;
	  break;
#endif

	    case ME_PAN:
	      channel[current_event->channel].panning=current_event->a;
	      if (adjust_panning_immediately)
		adjust_panning(current_event->channel);
	      ctl->panning(current_event->channel, current_event->a);
	      break;
	      
	    case ME_EXPRESSION:
	      channel[current_event->channel].expression=current_event->a;
	      adjust_volume(current_event->channel);
	      ctl->expression(current_event->channel, current_event->a);
	      break;

	    case ME_PROGRAM:
  	      if (channel[current_event->channel].kit)
		{
		  /* Change drum set */
		  /* if (channel[current_event->channel].kit==126)
		  	channel[current_event->channel].bank=57;
		  else */ channel[current_event->channel].bank=current_event->a;
		}
	      else
		{
		  channel[current_event->channel].program=current_event->a;
		}
	      ctl->program(current_event->channel, current_event->a);
	      break;

	    case ME_SUSTAIN:
	      channel[current_event->channel].sustain=current_event->a;
	      if (!current_event->a)
		drop_sustain(current_event->channel);
	      ctl->sustain(current_event->channel, current_event->a);
	      break;
	      
	    case ME_RESET_CONTROLLERS:
	      reset_controllers(current_event->channel);
	      redraw_controllers(current_event->channel);
	      break;

	    case ME_ALL_NOTES_OFF:
	      all_notes_off(current_event->channel);
	      break;
	      
	    case ME_ALL_SOUNDS_OFF:
	      all_sounds_off(current_event->channel);
	      break;

	    case ME_HARMONICCONTENT:
	      channel[current_event->channel].harmoniccontent=current_event->a;
	      break;

	    case ME_RELEASETIME:
	      channel[current_event->channel].releasetime=current_event->a;
/*
if (current_event->a != 64)
printf("release time %d on channel %d\n", channel[current_event->channel].releasetime,
current_event->channel);
*/
	      break;

	    case ME_ATTACKTIME:
	      channel[current_event->channel].attacktime=current_event->a;
/*
if (current_event->a != 64)
printf("attack time %d on channel %d\n", channel[current_event->channel].attacktime,
current_event->channel);
*/
	      break;
#ifdef tplus
	case ME_VIBRATO_RATE:
	  channel[current_event->channel].vibrato_ratio=midi_cnv_vib_rate(current_event->a);
	  update_channel_freq(current_event->channel);
	  break;
	case ME_VIBRATO_DEPTH:
	  channel[current_event->channel].vibrato_depth=midi_cnv_vib_depth(current_event->a);
	  update_channel_freq(current_event->channel);
	/*ctl->cmsg(CMSG_INFO, VERB_NORMAL, "~v%u", midi_cnv_vib_depth(current_event->a));*/
	  break;
	case ME_VIBRATO_DELAY:
	  channel[current_event->channel].vibrato_delay=midi_cnv_vib_delay(current_event->a);
	  update_channel_freq(current_event->channel);
	  break;
#endif

	    case ME_BRIGHTNESS:
	      channel[current_event->channel].brightness=current_event->a;
	      break;
	      
	    case ME_TONE_BANK:
	      channel[current_event->channel].bank=current_event->a;
	      break;

	    case ME_TONE_KIT:
	      if (current_event->a==SFX_BANKTYPE)
		{
		    channel[current_event->channel].sfx=SFXBANK;
		    channel[current_event->channel].kit=0;
		}
	      else
		{
		    channel[current_event->channel].sfx=0;
		    channel[current_event->channel].kit=current_event->a;
		}
	      break;

	    case ME_EOT:
	      /* Give the last notes a couple of seconds to decay  */
	      compute_data(play_mode->rate * 2);
	      compute_data(0); /* flush buffer to device */
	      ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Playing time: ~%d seconds",
		   current_sample/play_mode->rate+2);
	      ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Notes played: %d", played_notes);
	      ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Samples clipped: %d", output_clips);
	      ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Notes cut: %d", cut_notes);
	      ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Notes lost totally: %d", lost_notes);
	      return RC_TUNE_END;
	    }
	  current_event++;
	}

      rc=compute_data(current_event->time - current_sample);
      /*current_sample=current_event->time;*/
      ctl->refresh();
      if ( (rc!=RC_NONE) && (rc!=RC_JUMP))
	  return rc;
    }
}
#endif

#ifdef ADAGIO
static int i_am_open = 0;
static int i_have_done_it = 0;
static int tim_init();

int load_gm(char *name, int gm_num, int prog, int tpgm, int reverb, int main_volume)
{
  ToneBank *bank=0;

  if (!i_have_done_it) {
    add_to_pathlist(GUSPATDIR);
    tonebank[0]=safe_malloc(sizeof(ToneBank));
    memset(tonebank[0], 0, sizeof(ToneBank));
    if (tracing) ctl->verbosity += 2;
    else if (very_verbose) {
	ctl->verbosity++;
    } else if (!verbose) ctl->verbosity--;
    i_have_done_it = 1;
    tim_init();
  }
  else if (gm_num < 0) {
    free_instruments();
    if (!tonebank[0]) tonebank[0]=safe_malloc(sizeof(ToneBank));
    memset(tonebank[0], 0, sizeof(ToneBank));
  }

  if (gm_num < 0) return 0;

  bank=tonebank[0];

  bank->tone[prog].name=
	strcpy((bank->tone[prog].name=safe_malloc(strlen(name)+1)),name);
  bank->tone[prog].note=bank->tone[prog].amp=bank->tone[prog].pan=
	  bank->tone[prog].strip_loop=bank->tone[prog].strip_envelope=
	    bank->tone[prog].strip_tail=-1;
/* initialize font_type? */
  bank->tone[prog].gm_num = gm_num;
  bank->tone[prog].tpgm = tpgm;
  bank->tone[prog].reverb = reverb;
  bank->tone[prog].main_volume = main_volume;
  bank->tone[prog].layer=MAGIC_LOAD_INSTRUMENT;
  load_missing_instruments();
  return 0;
}
#endif /* ADAGIO */


#ifdef ADAGIO
static int p_rate;

static int tim_init()
{

  ctl->open(0,0);

  /* Set play mode parameters */
  play_mode->rate = setting_dsp_rate;  /*output_rate;*/
  if (setting_dsp_channels == 1) play_mode->encoding |= PE_MONO;

  init_tables();

/* for some reason, have to do this before loading instruments */
  if (!control_ratio)
	{
	  control_ratio = play_mode->rate / CONTROLS_PER_SECOND;
	  if(control_ratio<1)
		 control_ratio=1;
	  else if (control_ratio > MAX_CONTROL_RATIO)
		 control_ratio=MAX_CONTROL_RATIO;
	}
/**
  if (*def_instr_name)
	set_default_instrument(def_instr_name);
**/
  if (play_mode->open_output()<0) {
	fprintf(stderr, "Couldn't open %s\n", play_mode->id_name);
	dsp_dev = -2;
  }
  if (play_mode->encoding & PE_MONO) setting_dsp_channels = 1;
  p_rate = setting_dsp_rate = play_mode->rate;  /*output_rate;*/
  return 0;
}

int play_midi(unsigned char *from, unsigned char *to, int vrbose)
{
  if (from >= to)
  {
    if (i_am_open) {
	if (from == to) {
	  compute_data(play_mode->rate/* * 2*/);
	}
	compute_data(0); /* flush buffer to device */
	if (interactive) {
	    if (from > to) {
		sample_count=0; /* unknown */
		lost_notes=cut_notes=output_clips=0;
		skip_to(0); /* sets current_sample to 0 */
		channel[0].program = 0;
		current_event->channel = 0;
	    }
	    return(0);
	}
	free(event_list);
	free_instruments();
	play_mode->close_output();
        ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Playing time: ~%d seconds",
		   current_sample/play_mode->rate+2);
        ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Notes cut: %d", cut_notes);
        ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		   "Notes lost totally: %d", lost_notes);
	i_am_open = 0;
    }
    return 0;
  }
  if (!i_am_open) {
	voices = setting_dsp_voices;
	max_polyphony = voices;
	voices += 8;
	if (voices >= MAX_VOICES) voices = MAX_VOICES - 1;
	amplification = setting_dsp_volume;
        adjust_amplification();
        /** if (!i_have_done_it) load_gm("acpiano", 0, 0, 0, 0, 0); **/
	event_list = safe_malloc(sizeof(MidiEvent));
	sample_count=0; /* unknown */
	lost_notes=cut_notes=output_clips=0;
	skip_to(0); /* sets current_sample to 0 */
	channel[0].program = 0;
	current_event->channel = 0;
	i_am_open = 1;
	if (from >= to) return 0;
  }
  read_seq(from, to);
  return 0;
}

static void read_seq(unsigned char *from, unsigned char *to)
{
  static uint32 ptime = 0, stime = 0, posttime = 8;
  static int lastbehind = 0;
  int temp, behind, ahead, rtime = 0;
  unsigned char type, vc, arg, vel;
  short p1, p2;
  unsigned char *next;
  extern current_sample_count();
  extern time_expired;

  if (!current_sample) ptime = stime = posttime = lastbehind = 0;

  for ( ; from < to; from = next ) {

    type = from[0];
    next = from + 4;


    if (type == SEQ_WAIT) {
	ptime = from[1]|(from[2]<<8)|(from[3]<<16);
	#ifdef DEBUG_SEQ
	printf("\nt=%d\t", ptime);
	#endif
	stime = (ptime * p_rate)/100; /* ptime is in centisec. */

#if 0
#if defined(AU_LINUX) || defined(AU_SUN)
#define POST_V 50
#define MIN_POLYPHONY 8
#define T_BEHIND -10
      if (gus_dev >= 0 || sb_dev >= 0)
	if (!interactive && stime > current_sample && ptime > posttime * POST_V) {
		posttime = 1 + ptime/POST_V;
		rtime = ( 100 * current_sample_count() ) / p_rate;
		if (!(play_mode->encoding & PE_MONO)) rtime /= 2;
		if (play_mode->encoding & PE_16BIT) rtime /= 2;
		behind = time_expired - rtime;
		lastbehind = (9*lastbehind + behind)/10;
		ahead = ptime - rtime;
		if (very_verbose && !(posttime % 4))
		    printf("behind %d; ahead %d; rate %d; poly %d; lag %d\n",
			 behind, ahead, p_rate, max_polyphony, dev_sequencer_lag);
		if (behind > T_BEHIND + 5) {
			p_rate -= 1;
			dev_sequencer_lag++;
			posttime += 2;
		}
		else if (behind < T_BEHIND - 5) {
			p_rate += 1;
			dev_sequencer_lag--;
			posttime += 2;
		}
		if ( max_polyphony > MIN_POLYPHONY && ahead < 150 ) {
			max_polyphony -= 2;
			posttime += 2;
		}
		if (ahead > 200 && max_polyphony < voices)
			max_polyphony++;
		if (very_verbose && !(posttime % 4))
		    printf("behind %d; ahead %d; poly %d\n", behind, ahead, max_polyphony);
		if ( max_polyphony > MIN_POLYPHONY &&
		   (ahead < 150 || (behind > 20 && behind - lastbehind > 2))) {
			max_polyphony -= 2;
			posttime += 2;
		}
		if (ahead > 200 && behind < 6 && max_polyphony < voices)
			max_polyphony++;
		if (behind + lastbehind > 5) {
			/** dev_sequencer_lag += (behind + lastbehind)/8; **/
			dev_sequencer_lag++;
			/** lastbehind -= 10; **/
			posttime += 1;
		}
		else if (behind + lastbehind < -5 && dev_sequencer_lag) {
			dev_sequencer_lag--;
			/** lastbehind += 10; **/
			posttime += 2;
		}
		lastbehind = (2*lastbehind + behind)/3;
	}
#endif
#endif

	if (stime > current_sample) compute_data(stime - current_sample);
		/* now current_sample == stime */
	current_event->time = stime;
	continue;
    }
#ifdef DEBUG_SEQ
    else printf("\n");
#endif

    if (type & 0x80) {
	next += 4;
	if (type == SEQ_EXTENDED) {
	    type = from[1];
	    /* dev in from[2] should be 0 */
	}
	else if (type == SEQ_PRIVATE) {
	    /* dev in from[1] should be 0 */
            type = from[2];
	    vc = from[3];
	    p1 = (short)from[4]|(from[5]<<8);
	    p2 = (short)from[6]|(from[7]<<8);
	    switch (type) {
		case _GUS_NUMVOICES:
			#ifdef DEBUG_SEQ
			printf("_GUS_NUMVOICES,");
			#endif
			break;
		case _GUS_VOICEBALA:
			#ifdef DEBUG_SEQ
			printf("_GUS_VOICEBALA,");
			#endif
	  		channel[vc].panning = (p1 + 1)*8 - 4;
			adjust_panning(vc);
			break;
		case _GUS_VOICEVOL:
			#ifdef DEBUG_SEQ
			printf("_GUS_VOICEVOL,");
			#endif
			break;
		case _GUS_VOICEVOL2:
			#ifdef DEBUG_SEQ
			printf("_GUS_VOICEVOL2,");
			#endif
			break;
		default: /* not known */ break;
            }
	    #ifdef DEBUG_SEQ
	    printf("vc=%d p1=%d p2=%d ",vc,p1,p2);
	    #endif
	    continue;
	} else {
	  fprintf(stderr,"bad sequencer event\n");
	  continue;
	}
	from += 2;
    }

	/* interpret both 4 and 8 byte SEQ events */

    if ((vc=from[1]) >= MAX_VOICES) {
	  fprintf(stderr,"warning bad voice\n");
	  vc = MAX_VOICES - 1;
    }
    #ifdef DEBUG_SEQ
    printf("vc=%d ",vc);
    #endif
    current_event->type = type;
    current_event->channel = vc;
    channel[vc].bank = 0; /* necessary ?? */
    current_event->a = arg = from[2];
    current_event->b = vel = from[3];

    switch (type) {
      case SEQ_NOTEOFF:
	#ifdef DEBUG_SEQ
	printf("SEQ_NOTEOFF,");
	#endif
	note_off(current_event);
	break;
      case SEQ_NOTEON:
	#ifdef DEBUG_SEQ
	printf("SEQ_NOTEON,");
	#endif
	note_on(current_event);
	break;
      case SEQ_PGMCHANGE:
	#ifdef DEBUG_SEQ
	printf("SEQ_PGMCHANGE,");
	#endif
	channel[vc].program = arg;
	break;
      case SEQ_AFTERTOUCH:
	#ifdef DEBUG_SEQ
	printf("SEQ_AFTERTOUCH,");
	#endif
	/* arg is amount of touch: changes vibrato on fm in HS driver */
	adjust_pressure(current_event);
	break;
      case SEQ_CONTROLLER:
	p1 = (short)from[3]|(from[4]<<8);
	#ifdef DEBUG_SEQ
	printf("param=%d ",p1);
	#endif
	switch (arg) {
	    case CTL_BANK_SELECT:
		#ifdef DEBUG_SEQ
		printf("CTL_BANK_SELECT,");
		#endif
		break;
	    case CTL_MODWHEEL:
		#ifdef DEBUG_SEQ
		printf("CTL_MODWHEEL,");
		#endif
		break;
	    case CTL_BREATH:
		#ifdef DEBUG_SEQ
		printf("CTL_BREATH,");
		#endif
		break;
	    case CTL_FOOT:
		#ifdef DEBUG_SEQ
		printf("CTL_FOOT,");
		#endif
		break;
	    case CTL_MAIN_VOLUME:
		#ifdef DEBUG_SEQ
		printf("CTL_MAIN_VOLUME,");
		#endif
		break;
	    case CTL_PAN:
		#ifdef DEBUG_SEQ
		printf("CTL_PAN,");break;
		#endif
	    case CTL_EXPRESSION:
		#ifdef DEBUG_SEQ
		printf("CTL_EXPRESSION,");
		#endif
		break;
		/* I don't use the above; for the rest, p1 is the amount.*/
	    case CTRL_PITCH_BENDER:
		#ifdef DEBUG_SEQ
		printf("CTRL_PITCH_BENDER,");
		#endif
		temp = p1 + 0x2000;
		if (temp < 0) temp = 0;
		if (temp > 0x3FFF) temp = 0x3FFF;
		channel[vc].pitchbend = temp;
		channel[vc].pitchfactor = 0;
		adjust_pitchbend(vc);
		break;
	    case CTRL_PITCH_BENDER_RANGE:
		#ifdef DEBUG_SEQ
		printf("CTRL_PITCH_BENDER_RANGE,");
		#endif
		break;
	    case CTRL_EXPRESSION:
		#ifdef DEBUG_SEQ
		printf("CTRL_EXPRESSION,");
		#endif
		channel[vc].expression = p1;
		adjust_volume(vc);
		break;
	    case CTRL_MAIN_VOLUME:
		#ifdef DEBUG_SEQ
		printf("CTRL_MAIN_VOLUME,");
		#endif
		channel[vc].volume = p1;
		adjust_volume(vc);
		break;
	    default:
		#ifdef DEBUG_SEQ
		printf("CTL_??, ");
		#endif
		break;
	  } /* switch */
	#ifdef DEBUG_SEQ
	printf("param=%d ",p1);
	#endif
	break;
      default:fprintf(stderr,"?0x%x", type);break;
    } /* switch */
    #ifdef DEBUG_SEQ
    if (type != SEQ_CONTROLLER) printf("note %d vel %d",arg,vel);
    #endif
  } /* for */

}
#endif ADAGIO


#ifndef ADAGIO
int play_midi_file(char *fn)
{
  MidiEvent *event;
  uint32 events, samples;
  int rc;
  FILE *fp;

  ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "MIDI file: %s", fn);

  if (!strcmp(fn, "-"))
    {
      fp=stdin;
      strcpy(current_filename, "(stdin)");
    }
  else if (!(fp=open_file(fn, 1, OF_VERBOSE)))
    return RC_ERROR;

  ctl->file_name(current_filename);

  event=read_midi_file(fp, &events, &samples);
  
  if (fp != stdin)
      close_file(fp);
  
  if (!event)
      return RC_ERROR;

  ctl->cmsg(CMSG_INFO, VERB_NOISY, 
	    "%d supported events, %d samples", events, samples);

  ctl->total_time(samples);
  ctl->master_volume(amplification);

  load_missing_instruments();
#ifdef tplus
  reduce_quality_flag = 0;
#endif
  rc=play_midi(event, events, samples);
  if (free_instruments_afterwards)
      free_instruments();
  
  free(event);
#ifdef KMIDI
/*if (rc==RC_PATCHCHANGE) fprintf(stderr,"kernel returning PATCHCHANGE\n");*/
#endif
  return rc;
}
#endif

void dumb_pass_playing_list(int number_of_files, char *list_of_files[])
{
#ifndef ADAGIO
    int i=0;

    for (;;)
	{
	  switch(play_midi_file(list_of_files[i]))
	    {
	    case RC_REALLY_PREVIOUS:
		if (i>0)
		    i--;
		break;
			
	    default: /* An error or something */
	    case RC_NEXT:
		if (i<number_of_files-1)
		    {
			i++;
			break;
		    }
		/* else fall through */
		
	    case RC_QUIT:
		play_mode->close_output();
		ctl->close();
		return;
	    }
	}
#endif /* not ADAGIO */
}
