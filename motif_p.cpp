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

    motif_pipe.c: written by Vincent Pagel (pagel@loria.fr) 10/4/95
   
    pipe communication between motif interface and sound generator

    */
#ifdef MOTIF

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#ifdef SOLARIS
#include <sys/filio.h>
#endif

#include "config.h"
#include "controls.h"
#include "motif.h"

int pipeAppli[2],pipeMotif[2]; /* Pipe for communication with MOTIF process   */
int fpip_in, fpip_out;	/* in and out depends in which process we are */
int pid;	               /* Pid for child process */

/* DATA VALIDITY CHECK */
#define INT_CODE 214
#define STRING_CODE 216

#define DEBUGPIPE

/***********************************************************************/
/* PIPE COMUNICATION                                                   */
/***********************************************************************/

void motif_pipe_error(char *st)
{
    fprintf(stderr,"CONNECTION PROBLEM WITH MOTIF PROCESS IN %s BECAUSE:%s\n",
	    st,
	    sys_errlist[errno]);
    exit(1);
}


/*****************************************
 *              INT                      *
 *****************************************/

void motif_pipe_int_write(int c)
{
    int len;
    int code=INT_CODE;

#ifdef DEBUGPIPE
    len = write(fpip_out,&code,sizeof(code)); 
    if (len!=sizeof(code))
	motif_pipe_error("PIPE_INT_WRITE");
#endif

    len = write(fpip_out,&c,sizeof(c)); 
    if (len!=sizeof(int))
	motif_pipe_error("PIPE_INT_WRITE");
}

void motif_pipe_int_read(int *c)
{
    int len;
    
#ifdef DEBUGPIPE
    int code;

    len = read(fpip_in,&code,sizeof(code)); 
    if (len!=sizeof(code))
	motif_pipe_error("PIPE_INT_READ");
    if (code!=INT_CODE)	
	fprintf(stderr,"BUG ALERT ON INT PIPE %i\n",code);
#endif

    len = read(fpip_in,c, sizeof(int)); 
    if (len!=sizeof(int)) motif_pipe_error("PIPE_INT_READ");
}



/*****************************************
 *              STRINGS                  *
 *****************************************/

void motif_pipe_string_write(const char *str)
{
   int len, slen;

#ifdef DEBUGPIPE
   int code=STRING_CODE;

   len = write(fpip_out,&code,sizeof(code)); 
   if (len!=sizeof(code))	motif_pipe_error("PIPE_STRING_WRITE");
#endif
  
   slen=strlen(str);
   len = write(fpip_out,&slen,sizeof(slen)); 
   if (len!=sizeof(slen)) motif_pipe_error("PIPE_STRING_WRITE");

   len = write(fpip_out,str,slen); 
   if (len!=slen) motif_pipe_error("PIPE_STRING_WRITE on string part");
}

void motif_pipe_string_read(char *str)
{
    int len, slen;

#ifdef DEBUGPIPE
    int code;

    len = read(fpip_in,&code,sizeof(code)); 
    if (len!=sizeof(code)) motif_pipe_error("PIPE_STRING_READ");
    if (code!=STRING_CODE) fprintf(stderr,"BUG ALERT ON STRING PIPE %i\n",code);
#endif

    len = read(fpip_in,&slen,sizeof(slen)); 
    if (len!=sizeof(slen)) motif_pipe_error("PIPE_STRING_READ");
    
    len = read(fpip_in,str,slen); 
    if (len!=slen) motif_pipe_error("PIPE_STRING_READ on string part");
    str[slen]='\0';		/* Append a terminal 0 */
}

#if defined(sgi)
#include <sys/time.h>
#endif

#if defined(SOLARIS)
#include <sys/filio.h>
#endif

int motif_pipe_read_ready(void)
{
#if defined(sgi)
    fd_set fds;
    int cnt;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(fpip_in, &fds);
    timeout.tv_sec = timeout.tv_usec = 0;
    if((cnt = select(fpip_in + 1, &fds, NULL, NULL, &timeout)) < 0)
    {
	perror("select");
	return -1;
    }

    return cnt > 0 && FD_ISSET(fpip_in, &fds) != 0;
#else
    int num;
    

    if(ioctl(fpip_in,FIONREAD,&num) < 0) /* see how many chars in buffer. */
    {
	perror("ioctl: FIONREAD");
	return -1;
    }
    return num;
#endif
}

void motif_pipe_open()
{
    int res;
    
    res=pipe(pipeAppli);
    if (res!=0) motif_pipe_error("PIPE_APPLI CREATION");
    
    res=pipe(pipeMotif);
    if (res!=0) motif_pipe_error("PIPE_MOTIF CREATION");
    
    if ((pid=fork())==0)   /*child*/
	{
	    close(pipeMotif[1]); 
	    close(pipeAppli[0]);
	    
	    fpip_in=pipeMotif[0];
	    fpip_out= pipeAppli[1];
	    
	    Launch_Motif_Process(fpip_in);
	    /* Won't come back from here */
	    fprintf(stderr,"WARNING: come back from MOTIF\n");
	    exit(0);
	}
    
    close(pipeMotif[0]);
    close(pipeAppli[1]);
    
    fpip_in= pipeAppli[0];
    fpip_out= pipeMotif[1];
}

#endif /* MOTIF */
