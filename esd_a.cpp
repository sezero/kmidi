/*

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

    esd_a.c by Avatar <avatar@deva.net>
    based on linux_a.c

    Functions to play sound through EsounD
*/

#ifdef AU_ESD

#ifdef ORIG_TIMPP
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <errno.h>

#include <esd.h>

#ifdef ORIG_TIMPP
#include "timidity.h"
#else
#include "config.h"
#endif
#include "common.h"
#include "output.h"
#include "controls.h"
#ifdef ORIG_TIMPP
#include "timer.h"
#endif
#include "instrum.h"
#include "playmidi.h"
#ifdef ORIG_TIMPP
#include "miditrace.h"
#endif

#ifdef ORIG_TIMPP
static int acntl(int request, void *arg);
#endif
static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static void output_data(int32 *buf, uint32 count);
static int driver_output_data(unsigned char *buf, uint32 count);
static void flush_output(void);
static void purge_output(void);
static int output_count(uint32 ct);

/* export the playback mode */

#define dpm esd_play_mode

#ifdef ORIG_TIMPP
PlayMode dpm = {
    DEFAULT_RATE,
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_CAN_TRACE|PF_BUFF_FRAGM_OPT,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "Enlightened sound daemon", 'e',
    "/dev/dsp",
    open_output,
    close_output,
    output_data,
    acntl
};
#else
PlayMode dpm = {
    DEFAULT_RATE,
    PE_16BIT|PE_SIGNED,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "Enlightened sound daemon", 'e',
    "/dev/dsp",
    open_output,
    close_output,
    output_data,
    driver_output_data,
    flush_output,
    purge_output,
    output_count
};
#endif

#define PE_ALAW 	0x20

/*************************************************************************/
/* We currently only honor the PE_MONO bit, and the sample rate. */

static int open_output(void)
{
    int fd, /*tmp, i,*/ warnings = 0;
    /* int include_enc, exclude_enc; */
    esd_format_t esdformat;

#if 0
    include_enc = 0;
    exclude_enc = PE_ULAW|PE_ALAW|PE_BYTESWAP; /* They can't mean these */
    if(dpm.encoding & PE_16BIT)
	include_enc |= PE_SIGNED;
    else
	exclude_enc |= PE_SIGNED;
    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);
#endif

    /* Open the audio device */
    esdformat = (dpm.encoding & PE_16BIT) ? ESD_BITS16 : ESD_BITS8;
    esdformat |= (dpm.encoding & PE_MONO) ? ESD_MONO : ESD_STEREO;
    fd = esd_play_stream_fallback(esdformat,dpm.rate,NULL,"timidity");
    if(fd < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }


    dpm.fd = fd;

    return warnings;
}

static int driver_output_data(unsigned char *buf, uint32 count)
{
    return write(dpm.fd, buf, count);
}

#ifdef ORIG_TMPP
static int output_data(char *buf, uint32 count)
{
    int n;

    while(count > 0)
    {
	if((n = write(dpm.fd, buf, count)) == -1)
	{
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "%s: %s", dpm.name, strerror(errno));
	    if(errno == EWOULDBLOCK)
	    {
		/* It is possible to come here because of bug of the
		 * sound driver.
		 */
		continue;
	    }
	    return -1;
	}
	buf += n;
	count -= n;
    }

    return 0;
}
#endif

#ifndef ORIG_TIMPP

static int output_count(uint32 ct)
{
  int samples = -1;
  int samples_queued = 0, samples_sent = (int)ct;
  extern int b_out_count();

  samples = samples_sent = b_out_count();

  if (samples_sent) {
/* samples_queued is PM_REQ_GETFILLED */
#if 0
      if (snd_pcm_playback_status(handle, &playback_status) == 0)
	samples_queued = playback_status.queue;
#endif
      samples -= samples_queued;
  }
  if (!(dpm.encoding & PE_MONO)) samples >>= 1;
  if (dpm.encoding & PE_16BIT) samples >>= 1;
  return samples;
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


static void flush_output(void)
{
  output_data(0, 0);
  esd_audio_flush();
}

static void purge_output(void)
{
  b_out(dpm.id_character, dpm.fd, 0, -1);
}

#endif


static void close_output(void)
{
    if(dpm.fd == -1)
	return;
    close(dpm.fd);
    dpm.fd = -1;
}

#ifdef ORIG_TIMPP
static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
        esd_audio_flush();
	return 0;

      case PM_REQ_RATE: {
	  /* not implemented yet */
          break;
	}
    }
    return -1;
}
#endif

#endif /* AU_ESD */
