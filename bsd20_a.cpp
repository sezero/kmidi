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

    bsd20_a.c
	Written by Yamate Keiichiro <keiich-y@is.aist-nara.ac.jp>
*/

/*
 *  BSD/OS 2.0 audio
 */

#ifdef AU_BSDI

#ifdef ORIG_TIMPP
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <i386/isa/sblast.h>

#ifdef ORIG_TIMPP
#include "timidity.h"
#else
#include "config.h"
#endif

#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#ifdef ORIG_TIMPP
#include "timer.h"
#include "miditrace.h"
#endif

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
#ifdef ORIG_TIMPP
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);
#else
static int output_data(char *buf, uint32 count);
static int driver_output_data(unsigned char *buf, uint32 count);
static void flush_output(void);
static void purge_output(void);
static int output_count(uint32 ct);
#endif

/* export the playback mode */

#define dpm bsdi_play_mode

#ifdef ORIG_TIMPP
PlayMode dpm = {
    DEFAULT_RATE, PE_SIGNED|PE_MONO, PF_PCM_STREAM|PF_CAN_TRACE,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "BSD/OS sblast dsp", 'd',
    "/dev/sb_dsp",
    open_output,
    close_output,
    output_data,
    acntl
};
#else
PlayMode dpm = {
    DEFAULT_RATE, PE_SIGNED|PE_MONO,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "BSD/OS sblast dsp", 'd',
    "/dev/sb_dsp",
    open_output,
    close_output,
    output_data,
    driver_output_data,
    flush_output,
    purge_output,
    output_count
};
#endif

static int open_output(void)
{
    int fd, tmp, warnings=0;

    if ((fd=open(dpm.name, O_RDWR | O_NDELAY, 0)) < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }

    /* They can't mean these */
    dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);


    /* Set sample width to whichever the user wants. If it fails, try
       the other one. */

    if (dpm.encoding & PE_16BIT)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: sblast only 8bit",
		  dpm.name);
	close(fd);
	return -1;
    }

    tmp = ((~dpm.encoding) & PE_16BIT) ? PCM_8 : 0;
    ioctl(fd, DSP_IOCTL_COMPRESS, &tmp);
    dpm.encoding &= ~PE_SIGNED;

    /* Try stereo or mono, whichever the user wants. If it fails, try
       the other. */

    tmp=(dpm.encoding & PE_MONO) ? 0 : 1;
    ioctl(fd, DSP_IOCTL_STEREO, &tmp);

  /* Set the sample rate */

    tmp=dpm.rate * ((dpm.encoding & PE_MONO) ? 1 : 2);
    ioctl(fd, DSP_IOCTL_SPEED, &tmp);
    if (tmp != dpm.rate)
    {
	dpm.rate=tmp / ((dpm.encoding & PE_MONO) ? 1 : 2);;
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "Output rate adjusted to %d Hz", dpm.rate);
	warnings=1;
    }

    /* Older VoxWare drivers don't have buffer fragment capabilities */

    if (dpm.extra_param[0])
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s doesn't support buffer fragments", dpm.name);
	warnings=1;
    }

    tmp = 16384;
    ioctl(fd, DSP_IOCTL_BUFSIZE, &tmp);

    dpm.fd = fd;
    return warnings;
}

#ifdef ORIG_TIMPP
static int output_data(char *buf, int32 nbytes)
{
    while ((-1==write(dpm.fd, buf, nbytes)) && errno==EINTR)
	;
}
#else

static int driver_output_data(unsigned char *buf, uint32 count) {
  return snd_pcm_write(handle, buf, count);
}


static int output_count(uint32 ct)
{
  int samples = -1;
  int samples_queued, samples_sent = (int)ct;
  extern int b_out_count();

  samples = samples_sent = b_out_count();

  if (samples_sent) {
/* samples_queued is PM_REQ_GETFILLED */
      if (snd_pcm_playback_status(handle, &playback_status) == 0)
	samples_queued = playback_status.queue;
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
}

static void purge_output(void)
{
  b_out(dpm.id_character, dpm.fd, 0, -1);
  ioctl(dpm.fd, DSP_IOCTL_RESET);
}


#endif

static void close_output(void)
{
    if(dpm.fd != -1)
    {
	close(dpm.fd);
	dpm.fd = -1;
    }
}

#ifdef ORIG_TIMPP
static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
	return ioctl(dpm.fd, DSP_IOCTL_RESET);
    }
    return -1;
}
#endif

#endif /* AU_BSDI */
