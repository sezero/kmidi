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

    xaw_c.c - XAW Interface from
	Tomokazu Harada <harada@prince.pe.u-tokyo.ac.jp>
	Yoshishige Arai <ryo2@on.rim.or.jp>
*/
#ifdef IA_XAW

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */


#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "xaw.h"
static void ctl_lyric(int lyricid);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, const char *fmt, ...);
static void ctl_pass_playing_list(int number_of_files, const char *list_of_files[]);

static void a_pipe_open(void);
static int a_pipe_ready(void);
static void ctl_master_volume(int);
int a_pipe_read(char *,int);
void a_pipe_write(const char *);
static void a_pipe_write_msg(char *msg, int lyric);

static void ctl_refresh(void);
static void ctl_current_time(uint32 ct);
static void ctl_total_time(uint32 tt);
static void ctl_file_name(char *name);
static void ctl_xaw_note(int status, int ch, int note, int velocity);
static void ctl_note(int v);
static void ctl_program( int ch, int val, const char *name);
static void ctl_volume(int ch, int val);
static void ctl_expression(int ch, int val);
static void ctl_panning(int ch, int val);
static void ctl_sustain(int ch, int val);
static void ctl_pitch_bend(int ch, int val);
static void ctl_reset(void);
static void update_indicator(void);
static void set_otherinfo(int ch, int val, char c);
static void xaw_add_midi_file(char *additional_path);
static void xaw_delete_midi_file(int delete_num);
static void xaw_output_flist(void);
static int ctl_blocking_read(int32 *valp);
static void shuffle(int n,int *a);
static int check_midi_file(char *path);

static int indicator_last_update = 0;
#define EXITFLG_QUIT 1
#define EXITFLG_AUTOQUIT 2
static int exitflag=0,randomflag=0,repeatflag=0,selectflag=0;
static int xaw_ready=0;
static int number_of_files;
static char **list_of_files;
static char **titles;
static int *file_table;
extern int amplitude;


extern MidiEvent *current_event;

typedef struct {
  uint8 status, channel, note, velocity, voices;
  uint32 time;
} OldVoice;
#define MAX_OLD_NOTES 500
static OldVoice old_note[MAX_OLD_NOTES];
static int leading_pointer=0, trailing_pointer=0;
static uint32 current_centiseconds = 0;
static int songoffset = 0, current_voices = 0;

/**********************************************/
/* export the interface functions */

#define ctl xaw_control_mode


ControlMode ctl= 
{
  "XAW interface", 'a',
  1,1,0,
  ctl_open, ctl_pass_playing_list, ctl_close, ctl_read, cmsg,
  ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time, ctl_current_time, 
  ctl_note, 
  ctl_master_volume, ctl_program, ctl_volume, 
  ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};


static int check_midi_file(char *path)
{
  FILE *fp;
  char tmp[4];
  int len;

  if (!(fp=open_file(path, 1, OF_VERBOSE, 0))) return 0;

  if ((fread(tmp,1,4,fp) != 4) || (fread(&len,4,1,fp) != 1)) {
	fclose(fp);
	return 0;
  }
  len=BE_LONG(len);
  if (memcmp(tmp, "MThd", 4) || len < 6) {
	fclose(fp);
	return 0;
  }
  fclose(fp);
  return 1;
}

static char local_buf[300];

/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/
#define CMSG_MESSAGE 16

static int cmsg(int type, int verbosity_level, const char *fmt, ...) {
  char buff[2048];
  int flagnl = 1;
  va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  if (*fmt == '~')
   {
     flagnl = 0;
     fmt++;
   }
  va_start(ap, fmt);

  if(!xaw_ready) {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    return 0;
  }

  vsprintf(buff, fmt, ap);
  if (flagnl) a_pipe_write_msg(buff, 0);
  else a_pipe_write_msg(buff, 1);

  va_end(ap);
  return 0;
}

/*ARGSUSED*/
static int tt_i;


static void ctl_current_time(uint32 ct)
{

    int i,v, flags=0;
    int centisecs, realct, sec;
  static int previous_sec=-1,last_voices=-1;
  static int last_v=-1, last_time=-1;
    realct = play_mode->output_count(ct);
    if (realct < 0) realct = 0;
    else realct += songoffset;
    centisecs = realct / (play_mode->rate/100);
  current_centiseconds = (uint32)centisecs;
  sec = centisecs / 100;
 
    v=0;
    i=voices;
    while (i--)
	if (voice[i].status!=VOICE_FREE) v++;


  if (sec!=previous_sec) {
    previous_sec=sec;
    snprintf(local_buf,sizeof(local_buf),"t %d",sec);
    a_pipe_write(local_buf);
  }
  if (last_time!=tt_i) {
    last_time=tt_i;
    sprintf(local_buf, "T %d", tt_i);
    a_pipe_write(local_buf);
  }
  if(!ctl.trace_playing) return;
  if(last_voices!=voices) {
    last_voices=voices;
    snprintf(local_buf,sizeof(local_buf),"vL%d",voices);
    a_pipe_write(local_buf);
  }
  if(last_v!=v) {
    last_v=v;
    snprintf(local_buf,sizeof(local_buf),"vl%d",v);
    a_pipe_write(local_buf);
  }
  
}

static void ctl_total_time(uint32 tt)
{
    tt_i = (int)tt / play_mode->rate;
    ctl_current_time(0);
    sprintf(local_buf,"m%d",9);
    songoffset = 0;
    current_centiseconds = 0;
    a_pipe_write(local_buf);
}

static void ctl_master_volume(int mv)
{
  sprintf(local_buf,"V %03d", mv);
  amplitude=atoi(local_buf+2);
  if (amplitude < 0) amplitude = 0;
  if (amplitude > MAXVOLUME) amplitude = MAXVOLUME;
  a_pipe_write(local_buf);
}

static void ctl_volume(int ch, int val)
{
  if(!ctl.trace_playing) return;
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  sprintf(local_buf, "PV%c%d", ch+'A', val);
  a_pipe_write(local_buf);
}

static void ctl_expression(int ch, int val)
{
  if(!ctl.trace_playing) return;
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  sprintf(local_buf, "PE%c%d", ch+'A', val);
  a_pipe_write(local_buf);
}

static void ctl_panning(int ch, int val)
{
  if(!ctl.trace_playing) return;
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  sprintf(local_buf, "PA%c%d", ch+'A', val);
  a_pipe_write(local_buf);
}

static void ctl_sustain(int ch, int val)
{
  if(!ctl.trace_playing) return;
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  sprintf(local_buf, "PS%c%d", ch+'A', val);
  a_pipe_write(local_buf);
}

static void ctl_pitch_bend(int ch, int val)
{
  if(!ctl.trace_playing) return;
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  sprintf(local_buf, "PB%c%d", ch+'A', val);
  a_pipe_write(local_buf);
}

static void ctl_lyric(int lyricid)
{
    char *lyric;
    static int lyric_col = 0;
    static char lyric_buf[300];

}

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout) {
  ctl.opened=1;

  ctl.trace_playing = 1;
  /* The child process won't come back from this call  */
  a_pipe_open();

  return 0;
}

static void ctl_close(void)
{
  if (ctl.opened) {
    a_pipe_write("Q");
    ctl.opened=0;
    xaw_ready=0;
  }
}

static void xaw_add_midi_file(char *additional_path) {
    char *files[1],**tmp;
    int i,nfiles,nfit;
    char *p;

    files[0] = additional_path;
    nfiles = 1;

    tmp = list_of_files;
    titles=(char **)realloc(titles,(number_of_files+nfiles)*sizeof(char *));
    list_of_files=(char **)safe_malloc((number_of_files+nfiles)*sizeof(char *));
    for (i=0;i<number_of_files;i++)
        list_of_files[i]=(char *)strdup(tmp[i]);
    for (i=0,nfit=0;i<nfiles;i++) {
        if(check_midi_file(additional_path)) {
            p=strrchr(additional_path,'/');
            if (p==NULL) p=additional_path; else p++;      
            titles[number_of_files+nfit]=(char *)safe_malloc(sizeof(char)*(strlen(p)+ 9));
            list_of_files[number_of_files+nfit]=strdup(additional_path);
            sprintf(titles[number_of_files+nfit],"%d. %s",number_of_files+nfit+1,p);
            nfit++;
        }
    }
    if(nfit>0) {
        file_table=(int *)realloc(file_table,
                                       (number_of_files+nfit)*sizeof(int));
        for(i = number_of_files; i < number_of_files + nfit; i++)
            file_table[i] = i;
        number_of_files+=nfit;
        sprintf(local_buf, "X %d", nfit);
        a_pipe_write(local_buf);
        for (i=0;i<nfit;i++)
            a_pipe_write(titles[number_of_files-nfit+i]);
    }
}

static void xaw_delete_midi_file(int delete_num) {
    int i;
    char *p;

    if(delete_num<0) {
        for(i=0;i<number_of_files;i++){
            free(list_of_files[i]);
            free(titles[i]);
        }
        list_of_files = NULL; titles = NULL;
        file_table=(int *)realloc(file_table,1*sizeof(int));
        file_table[0] = 0;
        number_of_files = 0;
    } else {
        free(titles[delete_num]);
        for(i=delete_num;i<number_of_files-1;i++){
            list_of_files[i]= list_of_files[i+1];
            p= strchr(titles[i+1],'.');
            titles[i]= (char *)safe_malloc(strlen(titles[i+1])*sizeof(char *));
            sprintf(titles[i],"%d%s",i+1,p);
        }
        if(number_of_files>0) number_of_files -= 1;
    }
}

static void ctl_file_name(char *name)
{
#if 0
    pipe_int_write(FILENAME_MESSAGE);
    pipe_string_write(name);
#endif
}

static void xaw_output_flist(void) {
    int i;

    sprintf(local_buf, "s%d",number_of_files);
    a_pipe_write(local_buf);
    for(i=0;i<number_of_files;i++){
        sprintf(local_buf, "%s",list_of_files[i]);
        a_pipe_write(local_buf);
    }
}

/*ARGSUSED*/
static int ctl_blocking_read(int32 *valp) {
  int n;

  a_pipe_read(local_buf,sizeof(local_buf));
  for (;;) {
    switch (local_buf[0]) {
      case 'P' :
	return RC_LOAD_FILE;
      case 'U' :
	return RC_TOGGLE_PAUSE;
      case 'f':
	*valp=(int32)(play_mode->rate * 10);
        return RC_FORWARD;
      case 'b':
	*valp=(int32)(play_mode->rate * 10);
        return RC_BACK;
      case 'S' :
	return RC_QUIT;
      case 'N' :
	songoffset = 0;
	ctl_reset();
	return RC_NEXT;
      case 'B' :
	return RC_REALLY_PREVIOUS;
      case 'R' :
	repeatflag=atoi(local_buf+2);
	return RC_NONE;
      case 'D' :
	randomflag=atoi(local_buf+2);
	return RC_QUIT;
      case 'd' :
	n=atoi(local_buf+2);
        xaw_delete_midi_file(atoi(local_buf+2));
        return RC_QUIT;
      case 'A' : xaw_delete_midi_file(-1);
        return RC_QUIT;
      case 'C' :
	n=atoi(local_buf+2);
#ifdef ORIG_XAW
        opt_chorus_control = n;
#endif
        return RC_QUIT;
      case 'E' :
	n=atoi(local_buf+2);
#ifdef ORIG_XAW
        opt_modulation_wheel = n & MODUL_BIT;
        opt_portamento = n & PORTA_BIT;
        opt_nrpn_vibrato = n & NRPNV_BIT;
        opt_reverb_control = n & REVERB_BIT;
        opt_channel_pressure = n & CHPRESSURE_BIT;
        opt_overlap_voice_allow = n & OVERLAPV_BIT;
        opt_trace_text_meta_event = n & TXTMETA_BIT;
#endif
        return RC_QUIT;
      case 'F' : 
      case 'L' :
	selectflag=atoi(local_buf+2);
	return RC_QUIT;
      case 'T' :
	a_pipe_read(local_buf,sizeof(local_buf));
        n=atoi(local_buf+2);
	songoffset = *valp= n * play_mode->rate;
        return RC_JUMP;
      case 'V' :
	a_pipe_read(local_buf,sizeof(local_buf));
        amplification=atoi(local_buf+2);
	*valp=(int32)0;
        return RC_CHANGE_VOLUME;
#ifdef ORIG_XAW
      case '+': a_pipe_read(local_buf,sizeof(local_buf));
        *valp = (int32)1; return RC_KEYUP;
      case '-': a_pipe_read(local_buf,sizeof(local_buf));
        *valp = (int32)-1; return RC_KEYDOWN;
      case '>': a_pipe_read(local_buf,sizeof(local_buf));
        *valp = (int32)1; return RC_SPEEDUP;
      case '<': a_pipe_read(local_buf,sizeof(local_buf));
        *valp = (int32)1; return RC_SPEEDDOWN;
      case 'o': a_pipe_read(local_buf,sizeof(local_buf));
        *valp = (int32)1; return RC_VOICEINCR;
      case 'O': a_pipe_read(local_buf,sizeof(local_buf));
        *valp = (int32)1; return RC_VOICEDECR;
#endif
      case 'X':
	a_pipe_read(local_buf,sizeof(local_buf));
        xaw_add_midi_file(local_buf + 2);
        return RC_NONE;
      case '#':
	n=atoi(local_buf+2) - 1;
	if (n >= 0 && n < 30 && cfg_names[n]) {
	    cfg_select = n;
	    return RC_PATCHCHANGE;
	}
        else return RC_NONE;
      case 's':
        xaw_output_flist();
	return RC_NONE;
#ifdef ORIG_XAW
      case 'g': return RC_TOGGLE_SNDSPEC;
#endif
      case 'q' :
	exitflag ^= EXITFLG_AUTOQUIT;
	return RC_NONE;
      case 'Q' :
      default  :
	exitflag |= EXITFLG_QUIT;
	return RC_QUIT;
    }
  }
}

static int ctl_read(int32 *valp)
{
	int num;

	if (last_rc_command)
	  {
		*valp = last_rc_arg;
		num = last_rc_command;
		last_rc_command = 0;
		return num;
	  }
	/* We don't wan't to lock on reading  */
	num=a_pipe_ready(); 

	if (num<=0)
		return RC_NONE;
  
	return(ctl_blocking_read(valp));
}

static void shuffle(int n,int *a) {
  int i,j,tmp;

  for (i=0;i<n;i++) {
    j=(int) (((double)n) *rand()/(RAND_MAX+1.0));
    tmp=a[i];
    a[i]=a[j];
    a[j]=tmp;
  }
}

static void ctl_pass_playing_list(int init_number_of_files,
                  const char *init_list_of_files[]) {
  int current_no,command=RC_NONE,i,j;
  int32 val;
  char *p;

  /* Wait prepare 'interface' */
  a_pipe_read(local_buf,sizeof(local_buf));
  if (strcmp("READY",local_buf)) return;
  xaw_ready=1;

  sprintf(local_buf,"%d",
  (1<<MODUL_N)
     | (1<<PORTA_N)
     | (1<<NRPNV_N)
     | (1<<REVERB_N)
     | (1<<CHPRESSURE_N)
     | (1<<OVERLAPV_N)
     | (1<<TXTMETA_N));
  a_pipe_write(local_buf);
  sprintf(local_buf,"%d",1);
  a_pipe_write(local_buf);

  /* Make title string */
  titles=(char **)safe_malloc(init_number_of_files*sizeof(char *));
  list_of_files=(char **)safe_malloc(init_number_of_files*sizeof(char *));
  for (i=0,j=0;i<init_number_of_files;i++) {
    {
      p=strrchr(init_list_of_files[i],'/');
      if (p==NULL) {
        p=strdup(init_list_of_files[i]);
      } else p++;
      list_of_files[j]= strdup(init_list_of_files[i]);
      titles[j]=(char *)safe_malloc(sizeof(char)*(strlen(p)+ 9));
      sprintf(titles[j],"%d. %s",j+1,p);
      j++; number_of_files = j;
    }
  }
  titles=(char **)realloc(titles,init_number_of_files*sizeof(char *));
  list_of_files=(char **)realloc(list_of_files,init_number_of_files*sizeof(char *));

  /* Send title string */
  sprintf(local_buf,"%d",number_of_files);
  a_pipe_write(local_buf);
  for (i=0;i<number_of_files;i++)
    a_pipe_write(titles[i]);

  /* Make the table of play sequence */
  file_table=(int *)safe_malloc(number_of_files*sizeof(int));
  for (i=0;i<number_of_files;i++) file_table[i]=i;

  /* Draw the title of the first file */
  current_no=0;
  if(number_of_files!=0){
    snprintf(local_buf,sizeof(local_buf),"E %s",titles[file_table[0]]);
    a_pipe_write(local_buf);
    command=ctl_blocking_read(&val);
  }

  /* Main loop */
  for (;;) {
    /* Play file */
    if (command==RC_LOAD_FILE&&number_of_files!=0) {
      char *title;
      snprintf(local_buf,sizeof(local_buf),"E %s",titles[file_table[current_no]]);
      a_pipe_write(local_buf);
      title = list_of_files[current_no];
      snprintf(local_buf,sizeof(local_buf),"e %s", title);
      a_pipe_write(local_buf);
      command=play_midi_file(list_of_files[file_table[current_no]]);
    } else {
      if (command==RC_CHANGE_VOLUME) amplitude+=val;
      if (command==RC_JUMP) ;
      /* Quit timidity*/
      if (exitflag & EXITFLG_QUIT) return;
      /* Stop playing */
      if (command==RC_QUIT) {
        sprintf(local_buf,"T 00:00");
        a_pipe_write(local_buf);
        /* Shuffle the table */
        if (randomflag) {
          if(number_of_files == 0) {
            randomflag=0;
            continue;
          }
          current_no=0;
          if (randomflag==1) {
            shuffle(number_of_files,file_table);
            randomflag=0;
            command=RC_LOAD_FILE;
            continue;
          }
          randomflag=0;
          for (i=0;i<number_of_files;i++) file_table[i]=i;
          snprintf(local_buf,sizeof(local_buf),"E %s",titles[file_table[current_no]]);
          a_pipe_write(local_buf);
        }
        /* Play the selected file */
        if (selectflag) {
          for (i=0;i<number_of_files;i++)
            if (file_table[i]==selectflag-1) break;
          if (i!=number_of_files) current_no=i;
          selectflag=0;
          command=RC_LOAD_FILE;
          continue;
        }
        /* After the all file played */
      } else if (command==RC_TUNE_END || command==RC_ERROR) {
        if (current_no+1<number_of_files) {
          current_no++;
          command=RC_LOAD_FILE;
          continue;
        } else if (exitflag & EXITFLG_AUTOQUIT) {
          return;
          /* Repeat */
        } else if (repeatflag) {
          current_no=0;
          command=RC_LOAD_FILE;
          continue;
          /* Off the play button */
        } else {
          a_pipe_write("O");
        }
        /* Play the next */
      } else if (command==RC_NEXT) {
/* next 2 lines just added --gl */
        sprintf(local_buf,"T 00:00");
        a_pipe_write(local_buf);
        if (current_no+1<number_of_files) current_no++;
        command=RC_LOAD_FILE;
        continue;
        /* Play the previous */
      } else if (command==RC_REALLY_PREVIOUS) {
        if (current_no>0) current_no--;
        command=RC_LOAD_FILE;
        continue;
      } else if (command==RC_PATCHCHANGE) {
	free_instruments();
	end_soundfont();
	clear_config();
	read_config_file(current_config_file, 0);
        command=RC_NONE;
        continue;
      }
      command=ctl_blocking_read(&val);
    }
  }
}

/* ------ Pipe handlers ----- */

static int pipe_in_fd,pipe_out_fd;

extern void a_start_interface(int);

static void a_pipe_open(void) {
  int cont_inter[2],inter_cont[2];

  if (pipe(cont_inter)<0 || pipe(inter_cont)<0) exit(1);

  if (fork()==0) {
    close(cont_inter[1]);
    close(inter_cont[0]);
    pipe_in_fd=cont_inter[0];
    pipe_out_fd=inter_cont[1];
    a_start_interface(pipe_in_fd);
  }
  close(cont_inter[0]);
  close(inter_cont[1]);
  pipe_in_fd=inter_cont[0];
  pipe_out_fd=cont_inter[1];
}

void a_pipe_write(const char *buf) {
  write(pipe_out_fd,buf,strlen(buf));
  write(pipe_out_fd,"\n",1);
}

static int a_pipe_ready(void) {
  fd_set fds;
  static struct timeval tv;
  int cnt;

  FD_ZERO(&fds);
  FD_SET(pipe_in_fd,&fds);
  tv.tv_sec=0;
  tv.tv_usec=0;
  if((cnt=select(pipe_in_fd+1,&fds,NULL,NULL,&tv))<0)
    return -1;
  return cnt > 0 && FD_ISSET(pipe_in_fd, &fds) != 0;
}

int a_pipe_read(char *buf,int bufsize) {
  int i;

  bufsize--;
  for (i=0;i<bufsize;i++) {
    read(pipe_in_fd,buf+i,1);
    if (buf[i]=='\n') break;
  }
  buf[i]=0;
  return 0;
}

int a_pipe_nread(char *buf, int n)
{
    int i, j;

    j = 0;
    while(n > 0 && (i = read(pipe_in_fd, buf + j, n - j)) > 0)
    j += i;
    return j;
}

static void a_pipe_write_msg(char *msg, int lyric)
{
    int msglen;
    char buf[2 + sizeof(int)], *p, *q;

    msglen = strlen(msg);
    if (~lyric) msglen++;
    buf[0] = 'L';
    buf[1] = '\n';

    memcpy(buf + 2, &msglen, sizeof(int));
    write(pipe_out_fd, buf, sizeof(buf));
    if (lyric) write(pipe_out_fd, msg, msglen);
    else {
        write(pipe_out_fd, msg, msglen - 1);
        write(pipe_out_fd, "\n", 1);
    }
}

static void
ctl_xaw_note(int status, int ch, int note, int velocity)
{
  char c;

  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  if(!ctl.trace_playing) return;
 
    c = '.';
    switch(status) {
    case VOICE_ON:
      c = '*';
      break;
    case VOICE_SUSTAINED:
      c = '&';
      break;
    case VOICE_FREE:
    case VOICE_DIE:
    case VOICE_OFF:
    default:
      break;
    }
    snprintf(local_buf,sizeof(local_buf),"Y%c%c%03d%d",ch+'A',c,(unsigned char)note,velocity);
    a_pipe_write(local_buf);
}

static void ctl_note(int v)
{
  int i, n;
  if (!ctl.trace_playing)
    return;
  if (voice[v].clone_type != 0) return;

  old_note[leading_pointer].status = voice[v].status;
  old_note[leading_pointer].channel = voice[v].channel;
  old_note[leading_pointer].note = voice[v].note;
  old_note[leading_pointer].velocity = voice[v].velocity;
  old_note[leading_pointer].time = current_event->time / (play_mode->rate/100);
  n=0;
  i=voices;
  while (i--)
    if (voice[i].status!=VOICE_FREE) n++;
  old_note[leading_pointer].voices = n;
  leading_pointer++;
  if (leading_pointer == MAX_OLD_NOTES) leading_pointer = 0;

}

static void ctl_note_display(void)
{
  int v = trailing_pointer;
  uint32 then;

  then = old_note[v].time;

  while (then <= current_centiseconds && v != leading_pointer)
    {
	ctl_xaw_note(old_note[v].status, old_note[v].channel, old_note[v].note, old_note[v].velocity);
        current_voices = old_note[v].voices;
        v++;
        if (v == MAX_OLD_NOTES) v = 0;
        then = old_note[v].time;
    }
  trailing_pointer = v;
}


static void ctl_program( int ch, int val, const char *name)
{
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  if(!ctl.trace_playing) return;

  sprintf(local_buf, "PP%c%c%d", ch+'A', channel[ch].kit? 'd' : 'i', val);
  a_pipe_write(local_buf);
  if (name) sprintf(local_buf, "I%c%s", ch+'A', name);
  else sprintf(local_buf, "I%c%s", ch+'A', "");
    a_pipe_write(local_buf);
}


static void ctl_refresh(void)
{
       ctl_current_time(0);
       ctl_note_display();
	update_indicator();
}

static void set_otherinfo(int ch, int val, char c) {
  if(!ctl.trace_playing) return;
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  sprintf(local_buf, "P%c%c%d", c, ch+'A', val);
  a_pipe_write(local_buf);
}

static void ctl_reset(void)
{
  int i;

  if(!ctl.trace_playing) return;

  indicator_last_update = 0;
  for (i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
    if (channel[i].kit) {
      ctl_program(i, channel[i].bank, channel[i].name);
        set_otherinfo(i, channel[i].reverberation, 'r');
    } else {
      ToneBank *bank;
      int b;

      ctl_program(i, channel[i].program, channel[i].name);
      b = channel[i].bank;
      set_otherinfo(i, channel[i].bank, 'b');
        set_otherinfo(i, channel[i].reverberation, 'r');
        set_otherinfo(i, channel[i].chorusdepth, 'c');
    }
    ctl_volume(i, channel[i].volume);
    ctl_expression(i, channel[i].expression);
    ctl_panning(i, channel[i].panning);
    ctl_sustain(i, channel[i].sustain);
    if(channel[i].pitchbend == 0x2000 && channel[i].modulation_wheel > 0)
      ctl_pitch_bend(i, -1);
    else
      ctl_pitch_bend(i, channel[i].pitchbend);
  }
  for (i=0; i<MAX_OLD_NOTES; i++)
    {
      old_note[i].time = 0;
    }
  leading_pointer = trailing_pointer = current_voices = 0;
  sprintf(local_buf, "R");
  a_pipe_write(local_buf);  
}

static void update_indicator(void)
{
    int diff;

    if(!ctl.trace_playing)
        return;

    diff = current_centiseconds - indicator_last_update;
    if(diff > XAW_UPDATE_TIME)
    {
       sprintf(local_buf, "U");
       a_pipe_write(local_buf);
       indicator_last_update = current_centiseconds;
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_a_loader(void)
{
    return &ctl;
}

#ifndef HAVE_SNPRINTF

/*
 * Revision 12: http://theos.com/~deraadt/snprintf.c
 *
 * Copyright (c) 1997 Theo de Raadt
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#if __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#endif
#include <setjmp.h>

#ifndef roundup
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
#endif

static int pgsize;
static char *curobj;
static sigjmp_buf bail;

#define EXTRABYTES	2	/* XXX: why 2? you don't want to know */

static char *
msetup(str, n)
	char *str;
	size_t n;
{
	char *e;

	if (n == 0)
		return NULL;
	if (pgsize == 0)
		pgsize = getpagesize();
	curobj = (char *)malloc(n + EXTRABYTES + pgsize * 2);
	if (curobj == NULL)
		return NULL;
	e = curobj + n + EXTRABYTES;
	e = (char *)roundup((unsigned long)e, pgsize);
	if (mprotect(e, pgsize, PROT_NONE) == -1) {
		free(curobj);
		curobj = NULL;
		return NULL;
	}
	e = e - n - EXTRABYTES;
	*e = '\0';
	return (e);
}

static void
mcatch(int sig)
{
	siglongjmp(bail, 1);
}

static void
mcleanup(str, n, p)
	char *str;
	size_t n;
	char *p;
{
	strncpy(str, p, n-1);
	str[n-1] = '\0';
	if (mprotect((caddr_t)(p + n + EXTRABYTES), pgsize,
	    PROT_READ|PROT_WRITE|PROT_EXEC) == -1)
		mprotect((caddr_t)(p + n + EXTRABYTES), pgsize,
		    PROT_READ|PROT_WRITE);
	free(curobj);
}

int vsnprintf(char *str, size_t n, char const *fmt, va_list ap)
{
	struct sigaction osa, nsa;
	char *p;
	int ret = n + 1;	/* if we bail, indicated we overflowed */

	memset(&nsa, 0, sizeof nsa);
	nsa.sa_handler = mcatch;
	sigemptyset(&nsa.sa_mask);

	p = msetup(str, n);
	if (p == NULL) {
		*str = '\0';
		return 0;
	}
	if (sigsetjmp(bail, 1) == 0) {
		if (sigaction(SIGSEGV, &nsa, &osa) == -1) {
			mcleanup(str, n, p);
			return (0);
		}
		ret = vsprintf(p, fmt, ap);
	}
	mcleanup(str, n, p);
	(void) sigaction(SIGSEGV, &osa, NULL);
	return (ret);
}

int snprintf(char *str, size_t n, char const *fmt, ...)
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif

	return (vsnprintf(str, n, fmt, ap));
	va_end(ap);
}


#endif
#endif /* IA_XAW */
