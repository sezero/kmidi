/*
   kmidi 
   
   $Id$

   Copyright 1997 Bernd Johannes Wuebben math.cornell.edu

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../config.h"

#ifdef NOW_USING_SEMAPHORES
#ifdef HAVE_SYS_SEM_H
#include <sys/sem.h>
#endif
#endif

#ifdef _SCO_DS
#include <sys/socket.h>
#endif
#include <errno.h>

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"

#include "constants.h"
#include "ctl.h"

#ifdef NOW_USING_SEMAPHORES
#ifdef _SEM_SEMUN_UNDEFINED
   union semun
   {
     int val;
     struct semid_ds *buf;
     unsigned short int *array;
     struct seminfo *__buf;
   };
#endif
#endif

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct);
static void ctl_note(int v);
static void ctl_program(int ch, int val);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static int ctl_blocking_read(int32 *valp);

/*
void 	pipe_printf(char *fmt, ...);
void 	pipe_puts(char *str);
int 	pipe_gets(char *str, int maxlen);
*/

extern int output_device_open;
extern int cfg_select;
extern int read_config_file(char *name);
extern void clear_config(void);
extern void effect_activate( int iSwitch );
void pipe_int_write(int c);
void pipe_int_read(int *c);
void pipe_string_write(char *str);
void pipe_string_read(char *str);

void 	pipe_open();
void	pipe_error(char *st);
int 	pipe_read_ready();

/*
static int AppInit(Tcl_Interp *interp);
static int ExitAll(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
static int TraceCreate(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
static int TraceUpdate(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
static int TraceReset(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
static void trace_volume(int ch, int val);
static void trace_panning(int ch, int val);
static void trace_prog_init(int ch);
static void trace_prog(int ch, int val);
static void trace_bank(int ch, int val);
static void trace_sustain(int ch, int val);
*/

static void get_child(int sig);
static void shm_alloc(void);
static void shm_free(int sig);

/*
static void start_panel(void);
*/


/**********************************************/

#define ctl kmidi_control_mode

ControlMode ctl= 
{
  "kmidi qt  interface", 'q',
  1,1,0,
  ctl_open, ctl_pass_playing_list, ctl_close, ctl_read, cmsg,
  ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time, ctl_current_time, 
  ctl_note, 
  ctl_master_volume, ctl_program, ctl_volume, 
  ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};

PanelInfo *Panel;
static int songoffset = 0;

static int shmid;	/* shared memory id */
#ifdef NOW_USING_SEMAPHORES
static int semid;	/* semaphore id */
#endif
static int child_killed = 0;

static int pipeAppli[2],pipeMotif[2];
static int fpip_in, fpip_out;	
static int child_pid;

extern int Launch_KMidi_Process(int );

/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
	char local[2048];

#define TOO_LONG	2000
#define TOO_LONG2	2047

	va_list ap;
	if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	    ctl.verbosity<verbosity_level)
		return 0;
	       
	va_start(ap, fmt);
	if (strlen(fmt) > TOO_LONG)
		fmt[TOO_LONG] = 0;

	if (!ctl.opened) {
	  vfprintf(stderr, fmt, ap);
	  fprintf(stderr, "\n");
	}
 
	else if (type == CMSG_ERROR) {
	  /*	  int rc;
	  int32 val;*/
	  vsprintf(local, fmt, ap);
	  pipe_int_write(type);

	  /*	  printf("writing CMSG_ERROR of length %d\n",strlen(local));*/

	  if (strlen(local) > TOO_LONG2){
	    fprintf(stderr,"CSMG_ERROR STRING TOO LONG!\n");
	    printf("%s\n",local);
	    local[TOO_LONG2] = 0;
	  }
	  pipe_string_write(local);
	  /*	  while ((rc = ctl_blocking_read(&val)) != RC_NEXT)
			;*/
			
	} else {

		vsprintf(local, fmt, ap);

       		pipe_int_write(CMSG_MESSAGE);
       		pipe_int_write(type);

		/*printf("writing CMSG of length %d and type %d \n",strlen(local),type);*/

		if (strlen(local) > TOO_LONG2){
		fprintf(stderr,"CSMG STRING TOO LONG!\n");
		printf("%s\n",local);
		  local[TOO_LONG2] = 0;
		}

		/*		printf("WRITING=%s %d\n",local, strlen(local));*/

		pipe_string_write(local);

	}
	va_end(ap);
	return 0;
}

static void ctl_refresh(void)
{
}

static void ctl_total_time(int tt)
{
  int centisecs=tt/(play_mode->rate/100);

  pipe_int_write(TOTALTIME_MESSAGE);
  pipe_int_write(centisecs);
}

static int change_in_volume = 0;
static int change_in_voices;

static void ctl_master_volume(int mv)
{
  change_in_volume = 0;
  pipe_int_write(MASTERVOL_MESSAGE);
  pipe_int_write(mv);

}

static void ctl_file_name(char *name)
{
    pipe_int_write(FILENAME_MESSAGE);
    pipe_string_write(name);
}

static void ctl_current_time(int ct)
{

    int i,v, flags=0;
#if defined(linux) || defined(__FreeBSD__) || defined(sun)
    int centisecs, realct;
    realct = current_sample_count();
    if (realct < 0) realct = 0;
    else realct += songoffset;
    centisecs = realct / (play_mode->rate/100);
#else
    int centisecs=ct/(play_mode->rate/100);
#endif

    if (!ctl.trace_playing) 
	return;

    Panel->buffer_state = output_buffer_full;
    if (dont_cspline) flags |= 1;
    if (dont_reverb) flags |= 2;
    if (dont_chorus) flags |= 4;

    Panel->various_flags = flags;
 
    v=0;
    i=voices;
    while (i--)
	if (voice[i].status!=VOICE_FREE) v++;

    pipe_int_write(CURTIME_MESSAGE);
    pipe_int_write(centisecs);
    pipe_int_write(v);
}


static void ctl_channel_note(int ch, int note, int vel, int start)
{
	int k, slot=-1, i=voices, v=0, totalvel=0, total_sustain=0;
	FLOAT_T total_amp = 0, total_amp_s = 0;

	if (start == -1) return;

	ch &= 0x1f;

	if (slot == -1 /* && start > 10 */)
	  for (k = 0; k < NQUEUE; k++)
	    if (Panel->ctime[k][ch] >= start && Panel->ctime[k][ch] < start + 8) {
		slot = k;
		break;
	    }

	if (slot == -1)
	  for (k = 0; k < NQUEUE; k++)
	    if (Panel->ctime[k][ch] == -1) {
		slot = k;
		break;
	    }

	/* if (slot == -1) { fprintf(stderr,"D"); return; } */
	if (slot == -1) return;


	Panel->ctime[slot][ch] = start;

	while (i--) if (!(voice[i].status&VOICE_FREE) && (voice[i].channel&0x1f) == ch) {
		FLOAT_T amp;
		if (voice[i].clone_type) continue;
		amp = voice[i].left_amp + voice[i].right_amp;
		v++;
		if ( !(voice[i].status&(VOICE_ON|VOICE_SUSTAINED))) amp /= 2;
		if ( (voice[i].sample->modes & MODES_SUSTAIN) &&
		     (voice[i].status&(VOICE_ON|VOICE_SUSTAINED)) )
		     total_amp_s += amp;
		else total_amp += amp;
	}

	if (v) {
		totalvel = (int)( 370.0 * total_amp / v );
		total_sustain = (int)( 370.0 * total_amp_s / v );
		if (totalvel > 127) totalvel = 127;
		if (total_sustain > 127) total_sustain = 127;
	}

	Panel->notecount[slot][ch] = v;

	Panel->ctotal[slot][ch] = totalvel;
	Panel->ctotal_sustain[slot][ch] = total_sustain;

	if (channel[ch].kit) Panel->c_flags[ch] |= FLAG_PERCUSSION;
}

static void ctl_note(int v)
{
	int ch, note, vel, start;

	if (!ctl.trace_playing) 
		return;

	if (voice[v].clone_type != 0) return;

	start = (voice[v].starttime + (voice[v].sample_offset>>FRACTION_BITS)) /(play_mode->rate/100);
	ch = voice[v].channel;
	ch &= 0x1f;
	if (ch < 0 || ch >= MAXCHAN) return;

	note = voice[v].note;
	vel = voice[v].velocity;
#if 0
	switch(voice[v].status)
	{
	    case VOICE_DIE:
	      vel /= 2;
	      start = -1;
	      break;
	    case VOICE_FREE: 
	      vel = 0;
	      /* start = -1; */
	      break;
	    case VOICE_ON:
	      break;
	    case VOICE_OFF:
	      /* start = -1; */
	    case VOICE_SUSTAINED:
	      /* start = -1; */
	      break;
	}
#endif
	ctl_channel_note(ch, note, vel, start);
}

static void ctl_program(int ch, int val)
{
#ifdef MISC_PANEL_UPDATE
	if (!ctl.trace_playing) 
		return;
	ch &= 0x1f;
	if (ch < 0 || ch >= MAXCHAN) return;
	/*Panel->channel[ch].program = val;*/
	Panel->c_flags[ch] |= FLAG_PROG;
#endif
}

static void ctl_volume(int ch, int val)
{
#ifdef MISC_PANEL_UPDATE
	if (!ctl.trace_playing)
		return;
	ch &= 0x1f;
	/* Panel->channel[ch].volume = val; */
	ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch], -1);
#endif
}

static void ctl_expression(int ch, int val)
{
#ifdef MISC_PANEL_UPDATE
	if (!ctl.trace_playing)
		return;
	ch &= 0x1f;
	/* Panel->channel[ch].expression = val; */
	ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch], -1);
#endif
}

static void ctl_panning(int ch, int val)
{
#ifdef MISC_PANEL_UPDATE
	if (!ctl.trace_playing) 
		return;
	ch &= 0x1f;
	/* Panel->channel[ch].panning = val; */
	Panel->c_flags[ch] |= FLAG_PAN;
#endif
}

static void ctl_sustain(int ch, int val)
{
#ifdef MISC_PANEL_UPDATE
	if (!ctl.trace_playing)
		return;
	ch &= 0x1f;
	/* Panel->channel[ch].sustain = val; */
	Panel->c_flags[ch] |= FLAG_SUST;
#endif
}

static void ctl_pitch_bend(int channel, int val)
{
}

static void ctl_reset(void)
{
	int i, j;

	/*	pipe_int_write(TOTALTIME_MESSAGE);*/ /* This is crap */
	/*	pipe_int_write( ctl.trace_playing);*/

	if (!ctl.trace_playing)
		return;

        Panel->buffer_state = 100;
        Panel->various_flags = 0;
	Panel->reset_panel = 10;


	for (i = 0; i < MAXDISPCHAN; i++) {
		ctl_program(i, channel[i].program);
		ctl_volume(i, channel[i].volume);
		ctl_expression(i, channel[i].expression);
		ctl_panning(i, channel[i].panning);
		ctl_sustain(i, channel[i].sustain);
		ctl_pitch_bend(i, channel[i].pitchbend);
		ctl_channel_note(i, Panel->cnote[i], 0, -1);
		Panel->cindex[i] = 0;
		/* Panel->mindex[i] = 0; */
		for (j = 0; j < NQUEUE; j++) {
			Panel->ctime[j][i] = -1;
			Panel->notecount[j][i] = 0;
			Panel->ctotal[j][i] = 0;
			Panel->ctotal_sustain[j][i] = 0;
		}
	}
}

/***********************************************************************/
/* OPEN THE CONNECTION                                                */
/***********************************************************************/
static int ctl_open(int using_stdin, int using_stdout)
{
	int tcount = 10;
	shm_alloc();

        Panel->currentpatchset = cfg_select;
        Panel->buffer_state = 100;
        Panel->various_flags = 0;

	pipe_open();

	/*	if (child_pid == 0)
		start_panel();*/

	signal(SIGCHLD, get_child);
	signal(SIGTERM, shm_free);
	signal(SIGINT, shm_free);

	ctl.opened=1;

	while (!pipe_read_ready() && tcount) {
	    sleep(1);
	    tcount--;
	    if (!tcount)
		fprintf(stderr,"Internal error: no pipe open.\n");
	} 
	return 0;
}

/* Tells the window to disapear */
static void ctl_close(void)
{
	if (ctl.opened) {
	  pipe_int_write(CLOSE_MESSAGE);

	  /*kill(child_pid, SIGTERM);*/
	  /* wait((int *)0); */
	  shm_free(100);
	  ctl.opened=0;
	}
}


/* 
 * Read information coming from the window in a BLOCKING way
 */

/* commands are: PREV, NEXT, QUIT, STOP, LOAD, JUMP, VOLM */


static int ctl_blocking_read(int32 *valp)
{
  extern int reverb_options;
  int command;
  int new_volume;
  int new_centiseconds;
  int arg, argopt;
  int pausing = 0;

  pipe_int_read(&command);
  
  while (1)    /* Loop after pause sleeping to treat other buttons! */
      {

	  switch(command)
	      {
	      case MOTIF_CHANGE_VOLUME:
		  pipe_int_read(&new_volume);
		  if (!pausing) {
		      change_in_volume = *valp= new_volume - amplification ;
		      return RC_CHANGE_VOLUME;
		  }
		  amplification=new_volume;
		  break;
		  
	      case MOTIF_CHANGE_LOCATOR:
		  pipe_int_read(&new_centiseconds);
		  songoffset =
		  *valp= new_centiseconds*(play_mode->rate / 100) ;
		  ctl_reset();
		  return RC_JUMP;
		  
	      case MOTIF_QUIT:
		  return RC_QUIT;
		
	      case MOTIF_PLAY_FILE:
		  songoffset = 0;
		  ctl_reset();
		  return RC_LOAD_FILE;		  
		  
	      case MOTIF_NEXT:
		  songoffset = 0;
		  ctl_reset();
		  return RC_NEXT;
		  
	      case MOTIF_PREV:
		  songoffset = 0;
		  ctl_reset();
		  return RC_REALLY_PREVIOUS;
		  
	      case MOTIF_RESTART:
		  songoffset = 0;
		  ctl_reset();
		  return RC_RESTART;
/* not used
	      case MOTIF_FWD:
		  *valp=play_mode->rate;
		  ctl_reset();
		  return RC_FORWARD;
		  
	      case MOTIF_RWD:
		  *valp=play_mode->rate;
		  ctl_reset();
		  return RC_BACK;
*/
	      case MOTIF_PATCHSET:
		  pipe_int_read(&arg);
		  if (cfg_select == arg) return RC_NONE;
		  cfg_select = arg;
	/* fprintf(stderr,"ctl_blocking_read set #%d\n", cfg_select); */
		  return RC_PATCHCHANGE;

	      case MOTIF_EFFECTS:
		  pipe_int_read(&arg);
		  *valp= arg;
		  effect_activate(arg);
		  if (!pausing) return RC_NONE;
		  break;

	      case MOTIF_FILTER:
		  pipe_int_read(&arg);
		  *valp= arg;
		  command_cutoff_allowed = arg;
		  if (!pausing) return RC_NONE;
		  break;

	      case MOTIF_CHANGE_VOICES:
		  pipe_int_read(&arg);
		  change_in_voices =
		    *valp= arg;
		  return RC_CHANGE_VOICES;
		  
	      case MOTIF_CHECK_STATE:
		  pipe_int_read(&arg);
	/* arg >>4 = checkbox 0,1,2,3; arg&0x0f = 0 (off), 1 (no change), 2 (on) */
	/*fprintf(stderr, "box %d, state %d\n", arg>>4, arg&0x0f);*/
		  argopt = arg & 0x0f;
		  switch (arg>>4) {
		     case 0:
			/* stereo */
			if (!argopt) reverb_options &= ~(OPT_STEREO_VOICE | OPT_STEREO_EXTRA);
			else reverb_options |= OPT_STEREO_VOICE;
			if (argopt == 2) reverb_options |= OPT_STEREO_EXTRA;
			break;
		     case 1:
			/* reverb */
			if (!argopt) reverb_options &= ~(OPT_REVERB_VOICE | OPT_REVERB_EXTRA);
			else reverb_options |= OPT_REVERB_VOICE;
			if (argopt == 2) reverb_options |= OPT_REVERB_EXTRA;
			break;
		     case 2:
			/* chorus */
			if (!argopt) reverb_options &= ~(OPT_CHORUS_VOICE | OPT_CHORUS_EXTRA);
			else reverb_options |= OPT_CHORUS_VOICE;
			if (argopt == 2) reverb_options |= OPT_CHORUS_EXTRA;
			break;
		     case 3:
			/* verbosity */
			ctl.verbosity = argopt;
			break;
		  }
		  if (!pausing) return RC_NONE;
		  break;

	      case TRY_OPEN_DEVICE:
		  return RC_TRY_OPEN_DEVICE;
	      }
	  
	  
	  if (command==MOTIF_PAUSE)
	      {
		  if (pausing) {
		      pausing = 0;
		      return RC_NONE; /* Resume where we stopped */
		  }
		  ctl_reset();
		  pausing = 1;
	      }
	  if (pausing) pipe_int_read(&command);
	  else 
	      {
		  fprintf(stderr,"UNKNOWN RC_MESSAGE %i\n",command);
		  return RC_NONE;
	      }
      }
}

/* 
 * Read information coming from the window in a non blocking way
 */
static int ctl_read(int32 *valp)
{
	int num;

	/* We don't wan't to lock on reading  */
	num=pipe_read_ready(); 

	if (num==0)
		return RC_NONE;
  
	return(ctl_blocking_read(valp));
}

static void ctl_pass_playing_list(int number_of_files, char *list_of_files[])
{

    int i=0;
    char file_to_play[1000];
    int command;
    int32 val;

    Panel->currentpatchset = cfg_select;

    /* Pass the list to the interface */

    pipe_int_write(FILE_LIST_MESSAGE);

    pipe_int_write(number_of_files);
    for (i=0;i<number_of_files;i++)
	pipe_string_write(list_of_files[i]);

    /* Ask the interface for a filename to play -> begin to play automatically */
    pipe_int_write(NEXT_FILE_MESSAGE);
    
    command = ctl_blocking_read(&val);

    /* Main Loop */
    for (;;)
	{ 
	    if (command==RC_LOAD_FILE)
		{
		    /* Read a LoadFile command */
		    pipe_string_read(file_to_play);
		    command=play_midi_file(file_to_play);
		}
	    else
		{
		    if (command==RC_QUIT)
			return;
		    if (command==RC_ERROR)
			command=RC_TUNE_END; /* Launch next file */
	    

		    switch(command)
			{
			case RC_TRY_OPEN_DEVICE:
			  if( output_device_open == 0){
			  if (play_mode->open_output()<0){
			    output_device_open = 0;
			    pipe_int_write(DEVICE_NOT_OPEN);
			  }
                          else{
			    output_device_open = 1;
			    pipe_int_write(DEVICE_OPEN);
			  }
			  }
			 break;
      			case RC_CHANGE_VOLUME:
			    if (change_in_volume>0 || amplification > -change_in_volume)
	  			amplification += change_in_volume;
			    else amplification = 0;
			    change_in_volume = 0;
			    if (amplification > MAX_AMPLIFICATION)
	  			amplification=MAX_AMPLIFICATION;
			    ctl_master_volume(amplification);
			    break;
			case RC_NEXT:
			    pipe_int_write(NEXT_FILE_MESSAGE);
			    break;
			case RC_REALLY_PREVIOUS:
			    pipe_int_write(PREV_FILE_MESSAGE);
			    break;
			case RC_TUNE_END:
			    pipe_int_write(TUNE_END_MESSAGE);
			    break;
			case RC_PATCHCHANGE:
			/* fprintf(stderr,"Changing to patch #%d\n", cfg_select); */
		  	    free_instruments();
		  	    end_soundfont();
		  	    clear_config();
			/* what if read_config fails?? */
	          	    if (!read_config_file(CONFIG_FILE))
		  	        Panel->currentpatchset = cfg_select;
			/* else fprintf(stderr,"couldn't read config file\n"); */
			    pipe_int_write(PATCH_CHANGED_MESSAGE);
			    break;
		  	case RC_CHANGE_VOICES:
			    voices = change_in_voices;
			    break;
			case RC_NONE:
			    break;
			default:
			    printf("PANIC: UNKNOWN COMMAND ERROR: %i\n",command);
			}
		    
		    command = ctl_blocking_read(&val);
		}
	}
}



void pipe_open()
{
    int res;
    
    res=pipe(pipeAppli);
    if (res!=0) pipe_error("PIPE_APPLI CREATION");
    
    res=pipe(pipeMotif);
    if (res!=0) pipe_error("PIPE_KMIDI CREATION");
 
    if ((child_pid=fork())==0)   /*child*/
	{
	    close(pipeMotif[1]); 
	    close(pipeAppli[0]);
 
	    fpip_in=pipeMotif[0];
	    fpip_out= pipeAppli[1];

	    Launch_KMidi_Process(fpip_in);
	    exit(0);
	}
    
    close(pipeMotif[0]);
    close(pipeAppli[1]);
    
    fpip_in= pipeAppli[0];
    fpip_out= pipeMotif[1];
}



/***********************************************************************/
/* PIPE COMUNICATION                                                   */
/***********************************************************************/

void pipe_error(char *st)
{
    fprintf(stderr,"Kmidi:Problem with %s due to:%s\n",
	    st,
	    sys_errlist[errno]);
    exit(1);
}

/*
void pipe_printf(char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	pipe_puts(buf);
}

void pipe_puts(char *str)
{
	int len;
	char lf = '\n';
	len = strlen(str);
	write(fpip_out, str, len);
	write(fpip_out, &lf, 1);
}

int pipe_gets(char *str, int maxlen)
{

	char *p;
	int len;

	
	len = 0;
	for (p = str; ; p++) {
		read(fpip_in, p, 1);
		if (*p == '\n')
			break;
		len++;
	}
	*p = 0;
	return len;
}
*/

void pipe_int_write(int c)
{
    int len;


#ifdef DEBUGPIPE
    int code=INT_CODE;

    len = write(fpip_out,&code,sizeof(code)); 
    if (len!=sizeof(code))
	pipe_error("PIPE_INT_WRITE");
#endif

    len = write(fpip_out,&c,sizeof(c)); 
    if (len!=sizeof(int))
	pipe_error("PIPE_INT_WRITE");
}

void pipe_int_read(int *c)
{
    int len;
    
#ifdef DEBUGPIPE
    int code;

    len = read(fpip_in,&code,sizeof(code)); 
    if (len!=sizeof(code))
	pipe_error("PIPE_INT_READ");
    if (code!=INT_CODE)	
	fprintf(stderr,"BUG ALERT ON INT PIPE %i\n",code);
#endif

    len = read(fpip_in,c, sizeof(int)); 
    if (len!=sizeof(int)) pipe_error("PIPE_INT_READ");
}



/*****************************************
 *              STRINGS                  *
 *****************************************/

void pipe_string_write(char *str)
{
   int len, slen;

#ifdef DEBUGPIPE
   int code=STRING_CODE;

   len = write(fpip_out,&code,sizeof(code)); 
   if (len!=sizeof(code))	pipe_error("PIPE_STRING_WRITE");
#endif
  
   slen=strlen(str);
   len = write(fpip_out,&slen,sizeof(slen)); 
   if (len!=sizeof(slen)) pipe_error("PIPE_STRING_WRITE");

   len = write(fpip_out,str,slen); 
   if (len!=slen) pipe_error("PIPE_STRING_WRITE on string part");
}

void pipe_string_read(char *str)
{
    int len, slen;

#ifdef DEBUGPIPE
    int code;

    len = read(fpip_in,&code,sizeof(code)); 
    if (len!=sizeof(code)) pipe_error("PIPE_STRING_READ");
    if (code!=STRING_CODE) fprintf(stderr,"BUG ALERT ON STRING PIPE %i\n",code);
#endif

    len = read(fpip_in,&slen,sizeof(slen)); 
    if (len!=sizeof(slen)) pipe_error("PIPE_STRING_READ");
    
    len = read(fpip_in,str,slen); 
    if (len!=slen) pipe_error("PIPE_STRING_READ on string part");
    /*if (len!=slen) str[0]='\0';*/
    str[slen]='\0';		/* Append a terminal 0 */
}

#ifdef tplus

#if defined(sgi)
#include <sys/time.h>
#endif

#if defined(SOLARIS)
#include <sys/filio.h>
#endif

int pipe_read_ready(void)
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

#else
int pipe_read_ready()
{
    int num;
    
    ioctl(fpip_in,FIONREAD,&num); /* see how many chars in buffer. */
    return num;
}
#endif

/*----------------------------------------------------------------
 * shared memory handling
 *----------------------------------------------------------------*/

static void get_child(int sig)
{
	child_killed = 1;
}

static void shm_alloc(void)
{
	shmid = shmget(IPC_PRIVATE, sizeof(PanelInfo),
		       IPC_CREAT|0600);
	if (shmid < 0) {
		fprintf(stderr, "can't allocate shared memory\n");
		exit(1);
	}

#ifdef NOW_USING_SEMAPHORES
	semid = semget(IPC_PRIVATE, 1, IPC_CREAT|0600);
	if (semid < 0) {
	    perror("semget");
	    shmctl(shmid, IPC_RMID,NULL);
	    exit(1);
	}

	/* bin semaphore: only call once at first */
#if 0
	semaphore_V(semid);
#endif
#endif

	if ((Panel = (PanelInfo *)shmat (shmid, (char *)0, SHM_RND)) == (PanelInfo *) -1) {
		fprintf(stderr, "can't address shared memory\n");
		exit(1);
	}

	Panel->reset_panel = 10;
}

static void shm_free(int sig)
{
	int status;
#ifdef NOW_USING_SEMAPHORES
#if defined(linux) || defined(__FreeBSD__)
	union semun dmy;
#else /* Solaris 2.x, BSDI, OSF/1, HPUX */
	void *dmy;
#endif
#endif

	kill(child_pid, SIGTERM);
	while(wait(&status) != child_pid)
	    ;
#ifdef NOW_USING_SEMAPHORES
	memset(&dmy, 0, sizeof(dmy)); /* Shut compiler warning up :-) */
	semctl(semid, 0, IPC_RMID, dmy);
#endif
	shmctl(shmid, IPC_RMID, NULL);
	shmdt((char *)Panel);
	if (sig != 100)
		exit(0);
}

