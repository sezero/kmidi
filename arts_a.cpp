
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

    linux_audio.c

    Functions to play sound on the VoxWare audio driver (Linux or FreeBSD)

*/

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"
#include "output.h"
#include "controls.h"

#include <artsc.h>

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static void output_data(int32 *buf, uint32 count);
static int driver_output_data(unsigned char *buf, uint32 count);
static void flush_output(void);
static void purge_output(void);
static int output_count(uint32 ct);

static arts_stream_t stream;

/* export the playback mode */

#undef dpm
#define dpm arts_play_mode

PlayMode dpm = {
  DEFAULT_RATE, PE_16BIT|PE_SIGNED,
  -1,
  {0}, /* default: get all the buffer fragments you can */
  "Arts device", 'A',
  "artsd",
  open_output,
  close_output,
  output_data,
  driver_output_data,
  flush_output,
  purge_output,
  output_count
};

/*************************************************************************/
/* We currently only honor the PE_MONO bit, the sample rate, and the
   number of buffer fragments. We try 16-bit signed data first, and
   then 8-bit unsigned if it fails. If you have a sound device that
   can't handle either, let me know. */

static int output_count(uint32 ct)
{
  return ct;
}

static int driver_output_data(unsigned char *buf, uint32 count)
{
  return arts_write(stream,buf,count);
}

static void output_data(int32 *buf, uint32 count)
{
  int ocount;

  if (!(dpm.encoding & PE_MONO)) count*=2; /* Stereo samples */
  ocount = (int)count;

  if (ocount) {
    if (dpm.encoding & PE_16BIT)
      {
        /* Convert data to signed 16-bit PCM */
        s32tos16(buf, count);
        ocount *= 2;
      }
    else
      {
        /* Convert to 8-bit unsigned and write out. */
        s32tou8(buf, count);
      }
  }

  b_out(dpm.id_character, dpm.fd, (int *)buf, ocount);
}


static void close_output(void)
{
  arts_close_stream(stream);
  arts_free();
}

static void flush_output(void)
{
  output_data(0, 0);
}

static void purge_output(void)
{
  b_out(dpm.id_character, dpm.fd, 0, -1);
}

static int open_output(void) /* 0=success, 1=warning, -1=fatal error */
{
  int sample_width, channels, rate;

  if (arts_init() < 0) return -1;

  /* They can't mean these */
  dpm.encoding &= ~(PE_ULAW|PE_BYTESWAP);

  /* Set sample width to whichever the user wants. */

  sample_width=(dpm.encoding & PE_16BIT) ? LE_LONG(16) : LE_LONG(8);

  if (dpm.encoding & PE_16BIT)
    dpm.encoding |= PE_SIGNED;
  else
    dpm.encoding &= ~PE_SIGNED;


  /* Try stereo or mono, whichever the user wants. */

  channels=(dpm.encoding & PE_MONO) ? 1 : 2;

  /* Set the sample rate */
  
  rate=dpm.rate;

  stream = arts_play_stream(rate, sample_width, channels, "artsctest");

  arts_stream_set (stream, ARTS_P_BLOCKING, 0);

  return 0;
}
