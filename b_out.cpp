/*
	$Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef AU_OSS

#ifdef __linux__
	#include <sys/ioctl.h> /* new with 1.2.0? Didn't need this under 1.1.64 */
	#include <linux/soundcard.h>
#endif

#if defined(__FreeBSD__) || defined(__bsdi__)
	#include <sys/soundcard.h>
#endif

#ifdef __NetBSD__
	#include <sys/ioctl.h>
	#include <soundcard.h>
#endif

#endif

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"

#define PRESUMED_FULLNESS 20

/* #define BB_SIZE (AUDIO_BUFFER_SIZE*128) */
/* #define BB_SIZE (AUDIO_BUFFER_SIZE*256) */
static unsigned char *bbuf = 0;
static int bboffset = 0, bbcount = 0;
static int outchunk = 0;
static int starting_up = 1, flushing = 0;
static int out_count = 0;
static int total_bytes = 0;
static int fragsize = 0;
static int fragstotal = 0;

/*
#if defined(AU_OSS) || defined(AU_SUN) || defined(AU_BSDI) || defined(AU_ESD)
#define WRITEDRIVER(fd,buf,cnt) write(fd,buf,cnt)
#else
*/
#define WRITEDRIVER(fd,buf,cnt) play_mode->driver_output_data(buf,cnt)
/*
#endif
*/

int b_out_count()
{
  return out_count;
}

void b_out(char id, int fd, int *buf, int ocount)
{
  int ret;
  uint32 ucount;

  if (ocount < 0) {
	out_count = bboffset = bbcount = outchunk = 0;
	starting_up = 1;
	flushing = 0;
	output_buffer_full = PRESUMED_FULLNESS;
	total_bytes = 0;
	return;
  }

  ucount = (uint32)ocount;

  if (!bbuf) {
    bbcount = bboffset = 0;
    bbuf = (unsigned char *)malloc(BB_SIZE);
    if (!bbuf) {
	    fprintf(stderr, "malloc output error");
	    #ifdef ADAGIO
	    X_EXIT
	    #else
	    exit(1);
	    #endif
    }
  }

  if (!total_bytes) {
#ifdef AU_OSS
#ifdef SNDCTL_DSP_GETOSPACE
    audio_buf_info info;
    if ( (id == 'd' || id == 'D') &&
         (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) != -1)) {
	fragstotal = info.fragstotal;
	fragsize = info.fragsize;
	total_bytes = fragstotal * fragsize;
	outchunk = fragsize;
    }
    else
#endif /* SNDCTL_DSP_GETOSPACE */
#endif
#ifdef AU_ALSA
    if (id == 's') {
	extern void alsa_tell(int *, int *);
	alsa_tell(&fragsize, &fragstotal);
	outchunk = fragsize;
	total_bytes = fragstotal * fragsize;
    }
    else
#endif
	total_bytes = AUDIO_BUFFER_SIZE*2;
  }

  if (ucount && !outchunk) outchunk = ucount;
  if (starting_up && ucount + bboffset + bbcount >= BB_SIZE) starting_up = 0;
  if (!ucount) { outchunk = starting_up = 0; flushing = 1; }
  else flushing = 0;

  if (starting_up || flushing) output_buffer_full = PRESUMED_FULLNESS;
  else {
	int samples_queued;
#ifdef AU_OSS
#ifdef SNDCTL_DSP_GETODELAY
        if ( (id == 'd' || id == 'D') &&
	     (ioctl(fd, SNDCTL_DSP_GETODELAY, &samples_queued) == -1))
#endif
#endif
	    samples_queued = 0;

	if (!samples_queued) output_buffer_full = PRESUMED_FULLNESS;
	else output_buffer_full = ((bbcount+samples_queued) * 100) / (BB_SIZE + total_bytes);
/* fprintf(stderr," %d",output_buffer_full); */
  }

  ret = 0;

  if (!starting_up)
  while (bbcount) {
    if (check_for_rc()) return;
    if (outchunk && bbcount >= outchunk)
        ret = WRITEDRIVER(fd, bbuf + bboffset, outchunk);
    else if (flushing) {
	if (fragsize) ret = bbcount;
	else ret = WRITEDRIVER(fd, bbuf + bboffset, bbcount);
    }
    if (bboffset) {
	memcpy(bbuf, bbuf + bboffset, bbcount);
	bboffset = 0;
    }
    if (ret < 0) {
	if (errno == EINTR) continue;
	else if (errno == EWOULDBLOCK) {
/*	    if (bboffset) {
		memcpy(bbuf, bbuf + bboffset, bbcount);
		bboffset = 0;
	    }  */
	/* fprintf(stderr, "BLOCK %d ", bbcount); */
	    if (ucount && bboffset + bbcount + ucount <= BB_SIZE && !flushing) break;
	    ctl->refresh();
#ifdef HAVE_USLEEP
	    usleep(100);
#else
	    sleep(1);
#endif
	}
	else {
	    perror("error writing to dsp device");
	    #ifdef ADAGIO
	    X_EXIT
	    #else
	    exit(1);
	    #endif
	}
    }
    else {
	if (!ret && bboffset + bbcount + ucount <= BB_SIZE && !flushing) break;
	if (!ret) usleep(250);
	else {
	    out_count += ret;
	    bboffset += ret;
	    bbcount -= ret;
	}
	/* fprintf(stderr, "[%d:%d:f=%d/%d]\n", bbcount, ucount, output_buffer_full, BB_SIZE); */
	if (!bbcount) bboffset = 0;
	else if (bbcount < 0 || bboffset + bbcount > BB_SIZE) {
	    fprintf(stderr, "b_out.c:Uh oh.\n");
	    fprintf(stderr, "output error\n");
	    bbcount = bboffset = 0;
	    return;
	}
        if (bbcount < outchunk && !flushing) break;
    }
  }

  if (!ucount) { flushing = 0; starting_up = 1; out_count = bbcount = bboffset = 0; return; }

  memcpy(bbuf + bboffset + bbcount, buf, ucount);
  bbcount += ucount;

#ifndef KMIDI
  if (starting_up) return;
#endif

  if (ret >= 0) while (bbcount) {
    if (outchunk && bbcount >= outchunk)
        ret = WRITEDRIVER(fd, bbuf + bboffset, outchunk);
    /*else if (fragsize)  ret = bbcount;*/
    else if (fragsize)  { ret = 0; break; }
    else
        ret = WRITEDRIVER(fd, bbuf + bboffset, bbcount);
    ctl->refresh();
    if (ret < 0) {
	if (errno == EINTR) continue;
	else if (errno == EWOULDBLOCK) {
	    /* fprintf(stderr, "block %d ", bbcount); */
	    break;
	}
	else {
	    perror("error writing to dsp device");
	    #ifdef ADAGIO
	    X_EXIT
	    #else
	    exit(1);
	    #endif
	}
    }
    else {
	if (!ret && bboffset + bbcount + ucount <= BB_SIZE && !flushing) break;
	out_count += ret;
	bboffset += ret;
	bbcount -= ret;
	/*fprintf(stderr, "{%d:%d:r=%d}\n", bbcount,ucount,ret); */
	if (!bbcount) bboffset = 0;
	else if (bbcount < 0 || bboffset + bbcount > BB_SIZE) {
	    fprintf(stderr, "b_out.c:output error\n");
	    bbcount = bboffset = 0;
	    return;
	}
    }
  }
}
/* #endif */
