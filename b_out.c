/*
	$Id$
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include "config.h"
#include "output.h"

int output_buffer_full = 100;

/* #define BB_SIZE AUDIO_BUFFER_SIZE*8 */
#define BB_SIZE (AUDIO_BUFFER_SIZE*128)
static unsigned char *bbuf = 0;
static int bboffset = 0, bbcount = 0, outchunk = 0;
static int starting_up = 1, flushing = 0;

void b_out(int fd, int *buf, int ocount)
{
  int ret;

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


  if (ocount && !outchunk) outchunk = ocount;
  if (starting_up && ocount + bboffset + bbcount >= BB_SIZE) starting_up = 0;
  if (!ocount) { outchunk = starting_up = 0; flushing = 1; }
  else flushing = 0;

  if (starting_up || flushing) output_buffer_full = 100;
  else output_buffer_full = (bbcount * 100) / BB_SIZE;

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
	fprintf(stderr, "BLOCK %d ", bbcount);
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
	bboffset += ret;
	bbcount -= ret;
	/* fprintf(stderr, "[%d:%d:f=%d/%d]\n", bbcount, ocount, output_buffer_full, BB_SIZE); */
	if (!bbcount) bboffset = 0;
	else if (bbcount < 0 || bboffset + bbcount > BB_SIZE) {
	   fprintf(stderr, "Uh oh.\n");
	}
        if (bbcount < outchunk && !flushing) break;
    }
  }

  if (!ocount) { flushing = 0; starting_up = 1; bbcount = bboffset = 0; return; }

  memcpy(bbuf + bboffset + bbcount, buf, ocount);
  bbcount += ocount;

  if (starting_up) return;

  if (ret >= 0) while (bbcount) {
    /* if (ocount && bbcount >= ocount)
        ret = write(fd, bbuf + bboffset, ocount); */
    if (outchunk && bbcount >= outchunk)
        ret = write(fd, bbuf + bboffset, outchunk);
    else
        ret = write(fd, bbuf + bboffset, bbcount);
    if (ret < 0) {
	if (errno == EINTR) continue;
	else if (errno == EWOULDBLOCK) {
	    fprintf(stderr, "block %d ", bbcount);
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
/* Debug: Are we getting output? */
#if 0
int i;
fprintf(stderr,"[%d]",ret);
for (i = 0; i < 20 && i < ret; i++)
fprintf(stderr," %d", (int)*(bbuf+bboffset+i) );
fprintf(stderr,"\n");
#endif
	if (!ret && bboffset + bbcount + ocount <= BB_SIZE && !flushing) break;
	bboffset += ret;
	bbcount -= ret;
/*fprintf(stderr, "{%d:%d:r=%d}\n", bbcount,ocount,ret); */
	if (!bbcount) bboffset = 0;
	else if (bbcount < 0 || bboffset + bbcount > BB_SIZE) {
	    fprintf(stderr, "output error");
	    bbcount = bboffset = 0;
	}
    }
  }
}
