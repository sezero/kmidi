/*
	$Id$
*/

#if defined(linux) || defined(__FreeBSD__) || defined(sun)

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#ifdef linux
#include <sys/ioctl.h> /* new with 1.2.0? Didn't need this under 1.1.64 */
#include <linux/soundcard.h>
#endif

#ifdef __FreeBSD__
#include <machine/soundcard.h>
#endif

#include <malloc.h>
#include "config.h"
#include "output.h"

/* #define BB_SIZE (AUDIO_BUFFER_SIZE*128) */
#define BB_SIZE (AUDIO_BUFFER_SIZE*256)
static unsigned char *bbuf = 0;
static int bboffset = 0, bbcount = 0, outchunk = 0;
static int starting_up = 1, flushing = 0;
static int out_count = 0;
static int total_bytes = 0;

int b_out_count()
{
  return out_count;
}

void b_out(int fd, int *buf, int ocount)
{
  int ret;

  if (ocount < 0) {
	out_count = bboffset = bbcount = outchunk = 0;
	starting_up = 1;
	flushing = 0;
	output_buffer_full = 100;
	total_bytes = 0;
	return;
  }

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
#ifdef SNDCTL_DSP_GETOSPACE
    audio_buf_info info;
    if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) != -1) {
	total_bytes = info.fragstotal * info.fragsize;
	outchunk = info.fragsize;
    }
    else
#endif /* SNDCTL_DSP_GETOSPACE */
	total_bytes = AUDIO_BUFFER_SIZE*2;

  }

  if (ocount && !outchunk) outchunk = ocount;
  if (starting_up && ocount + bboffset + bbcount >= BB_SIZE) starting_up = 0;
  if (!ocount) { outchunk = starting_up = 0; flushing = 1; }
  else flushing = 0;

  if (starting_up || flushing) output_buffer_full = 100;
  else {
	int samples_queued;
#ifdef SNDCTL_DSP_GETODELAY
	if (ioctl(fd, SNDCTL_DSP_GETODELAY, &samples_queued) == -1)
#endif
	    samples_queued = 0;

	output_buffer_full = ((bbcount+samples_queued) * 100) / (BB_SIZE + total_bytes);
  }

  ret = 0;

  if (!starting_up)
  while (bbcount) {
    if (outchunk && bbcount >= outchunk)
        ret = write(fd, bbuf + bboffset, outchunk);
    else if (flushing)
        ret = write(fd, bbuf + bboffset, bbcount);
    if (ret < 0) {
	if (errno == EINTR) continue;
	else if (errno == EWOULDBLOCK) {
	    if (bboffset) {
		memcpy(bbuf, bbuf + bboffset, bbcount);
		bboffset = 0;
	    }
	/* fprintf(stderr, "BLOCK %d ", bbcount); */
	    if (ocount && bboffset + bbcount + ocount <= BB_SIZE && !flushing) break;
	    sleep(1);
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
	if (!ret && bboffset + bbcount + ocount <= BB_SIZE && !flushing) break;
	out_count += ret;
	bboffset += ret;
	bbcount -= ret;
	/* fprintf(stderr, "[%d:%d:f=%d/%d]\n", bbcount, ocount, output_buffer_full, BB_SIZE); */
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

  if (!ocount) { flushing = 0; starting_up = 1; out_count = bbcount = bboffset = 0; return; }

  memcpy(bbuf + bboffset + bbcount, buf, ocount);
  bbcount += ocount;

  if (starting_up) return;

  if (ret >= 0) while (bbcount) {
    if (outchunk && bbcount >= outchunk)
        ret = write(fd, bbuf + bboffset, outchunk);
    else
        ret = write(fd, bbuf + bboffset, bbcount);
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
	if (!ret && bboffset + bbcount + ocount <= BB_SIZE && !flushing) break;
	out_count += ret;
	bboffset += ret;
	bbcount -= ret;
	/*fprintf(stderr, "{%d:%d:r=%d}\n", bbcount,ocount,ret); */
	if (!bbcount) bboffset = 0;
	else if (bbcount < 0 || bboffset + bbcount > BB_SIZE) {
	    fprintf(stderr, "b_out.c:output error\n");
	    bbcount = bboffset = 0;
	    return;
	}
    }
  }
}
#endif
