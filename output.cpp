/* 

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

    output.c
    
    Audio output (to file / device) functions.
*/

#include "config.h"
#include "output.h"
#include "tables.h"
#include <sys/types.h>


/* These are very likely mutually exclusive.. */
/* #ifdef __osf__ */
#ifdef AU_DEC
extern PlayMode dec_play_mode;
#define DEFAULT_PLAY_MODE &dec_play_mode
#endif

/* #if defined(__linux__) || defined(__FreeBSD__) || defined(__bsdi__) */
#ifdef AU_OSS
extern PlayMode linux_play_mode;
extern PlayMode linux_play_mode_two;
#define DEFAULT_PLAY_MODE &linux_play_mode
#endif

/* #if defined(hpux) || defined(__hpux) */
#if defined(AU_HPUX_ALIB) || defined(AU_HPUX)
extern PlayMode hpux_play_mode;
#define DEFAULT_PLAY_MODE &hpux_play_mode
#endif

/* #if defined(sun) */
#ifdef AU_SUN
extern PlayMode sun_play_mode;
#define DEFAULT_PLAY_MODE &sun_play_mode
#endif

#ifdef AU_ALSA
extern PlayMode alsa_play_mode;
#define DEFAULT_PLAY_MODE &alsa_play_mode
#endif

#ifdef AU_NAS
extern PlayMode nas_play_mode;
#define DEFAULT_PLAY_MODE &nas_play_mode
#endif

#ifdef AU_ESD
extern PlayMode esd_play_mode;
#ifndef DEFAULT_PLAY_MODE
#define DEFAULT_PLAY_MODE &esd_play_mode
#endif
#endif

#ifdef AU_WIN32
extern PlayMode win32_play_mode;
#define DEFAULT_PLAY_MODE &win32_play_mode
#endif

/* These are always compiled in. */
#ifndef ADAGIO
extern PlayMode raw_play_mode, wave_play_mode;
extern PlayMode arts_play_mode;
#endif


PlayMode *play_mode_list[] = {
#ifdef DEFAULT_PLAY_MODE
  DEFAULT_PLAY_MODE,
#endif
#ifdef AU_OSS
  &linux_play_mode_two,
#endif
#ifdef AU_ESD
  &esd_play_mode,
#endif
#ifndef ADAGIO
  &wave_play_mode,
  &raw_play_mode,
#endif
  &arts_play_mode,
  0
};

#ifdef DEFAULT_PLAY_MODE
  PlayMode *play_mode=DEFAULT_PLAY_MODE;
#else
  PlayMode *play_mode=&wave_play_mode;
#endif

/*****************************************************************/
/* Some functions to convert signed 32-bit data to other formats */

int output_clips = 0;
int output_buffer_full = 50;

void s32tos8(int32 *lp, uint32 c)
{
  int8 *cp=(int8 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-8-GUARD_BITS);
      if (l>127) { l=127; output_clips++; }
      else if (l<-128) { l=-128; output_clips++; }
      *cp++ = (int8) (l);
    }
}

void s32tou8(int32 *lp, uint32 c)
{
  uint8 *cp=(uint8 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-8-GUARD_BITS);
      if (l>127) { l=127; output_clips++; }
      else if (l<-128) { l=-128; output_clips++; }
      *cp++ = 0x80 ^ ((uint8) l);
    }
}

void s32tos16(int32 *lp, uint32 c)
{
  int16 *sp=(int16 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-16-GUARD_BITS);
      if (l > 32767) { l=32767; output_clips++; }
      else if (l<-32768) { l=-32768; output_clips++; }
      *sp++ = (int16)(l);
    }
}

void s32tou16(int32 *lp, uint32 c)
{
  uint16 *sp=(uint16 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-16-GUARD_BITS);
      if (l > 32767) { l=32767; output_clips++; }
      else if (l<-32768) { l=-32768; output_clips++; }
      *sp++ = 0x8000 ^ (uint16)(l);
    }
}

void s32tos16x(int32 *lp, uint32 c)
{
  int16 *sp=(int16 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-16-GUARD_BITS);
      if (l > 32767) { l=32767; output_clips++; }
      else if (l<-32768) { l=-32768; output_clips++; }
      *sp++ = XCHG_SHORT((int16)(l));
    }
}

void s32tou16x(int32 *lp, uint32 c)
{
  uint16 *sp=(uint16 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-16-GUARD_BITS);
      if (l > 32767) { l=32767; output_clips++; }
      else if (l<-32768) { l=-32768; output_clips++; }
      *sp++ = XCHG_SHORT(0x8000 ^ (uint16)(l));
    }
}

void s32toulaw(int32 *lp, uint32 c)
{
  uint8 *up=(uint8 *)(lp);
  int32 l;
  while (c--)
    {
      l=(*lp++)>>(32-13-GUARD_BITS);
      if (l > 4095) { l=4095; output_clips++; }
      else if (l<-4096) { l=-4096; output_clips++; }
      *up++ = _l2u[l];
    }
}
