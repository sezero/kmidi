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

    Functions to play sound on a HP's audio device

    Copyright 1997 Lawrence T. Hoff

*/

#ifdef AU_HPUX

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <sys/ioctl.h> 

#include <sys/audio.h>

#include <errno.h>

#include "config.h"
#include "output.h"
#include "controls.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int driver_output_data(unsigned char *buf, uint32 count);
static void output_data(int32 *buf, uint32 count);
static void flush_output(void);
static void purge_output(void);
static int output_count(uint32 ct);

/* export the playback mode */
#define DEFAULT_HP_ENCODING PE_16BIT|PE_SIGNED

#define dpm hpux_play_mode
PlayMode dpm = {
    DEFAULT_RATE, DEFAULT_HP_ENCODING,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "HPUX audio device", 'd',
    "/dev/audio",
    open_output,
    close_output,
    driver_output_data,
    output_data,
    flush_output,
    purge_output,
    output_count
};

static int open_output(void)
{

if (dpm.encoding & PE_ULAW)
  dpm.encoding &= ~PE_16BIT;

if (!(dpm.encoding & PE_16BIT))
  dpm.encoding &= ~PE_BYTESWAP;

dpm.fd = open(dpm.name, O_WRONLY|O_NONBLOCK, 0);
if(dpm.fd == -1)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
	   dpm.name, strerror(errno));
      return -1;
    }

(void) ioctl(dpm.fd, AUDIO_SET_SAMPLE_RATE, dpm.rate);

(void) ioctl(dpm.fd, AUDIO_SET_DATA_FORMAT, (dpm.encoding & PE_16BIT)?
	AUDIO_FORMAT_LINEAR16BIT: AUDIO_FORMAT_ULAW);

(void) ioctl(dpm.fd, AUDIO_SET_CHANNELS, (dpm.encoding & PE_MONO)?1:2);

/* set some reasonable buffer size */
(void) ioctl(dpm.fd, AUDIO_SET_TXBUFSIZE, 128*1024);

return 0;
}

#ifdef ORIG_TIMPP
static void output_data(int32 *buf, uint32 count)
{
    if (!(dpm.encoding & PE_MONO)) count*=2; /* Stereo samples */
    
    if (dpm.encoding & PE_ULAW)
	{/* ULAW encoding */
	    s32toulaw(buf, count);
	}    
    else  /* Linear encoding */
	if (dpm.encoding & PE_16BIT)
	    {  /* Convert data to signed 16-bit PCM */
		s32tos16(buf, count);
		count *= 2;
	    }
	else	/* Linear 8 bit */
	    if (dpm.encoding & PE_SIGNED)
		s32tos8(buf, count);
	    else 
		s32tou8(buf, count);	

    /* bytes_written = */ write(dpm.fd, buf, count);
#if 0
    /* Write DATA in the socket */
    if (( bytes_written = write( streamSocket, buf, count )) < 0 ) 
	{
	    ctl->cmsg(CMSG_ERROR,VERB_NORMAL,"Audio Socket Write Failed");
	    ctl->close();
	    exit(1);
	}
#endif
}
#else

static int driver_output_data(unsigned char *buf, uint32 count)
{
    return write(dpm.fd, buf, count);
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
#endif

static void close_output(void)
{
    if(dpm.fd != -1)
    {
	/* free resources */
	close(dpm.fd);
	dpm.fd = -1;
    }
}

static void flush_output(void) {
    output_data(0, 0);
}

static void purge_output(void) {

    b_out(dpm.id_character, dpm.fd, 0, -1);
    ioctl(dpm.fd, AUDIO_RESET, RESET_TX_BUF);

}

static int output_count(uint32 ct)
{
  return (int)ct;
}
#endif /* AU_HPUX */
