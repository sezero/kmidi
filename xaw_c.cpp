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

#ifdef ORIG_XAW
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#else
#define HAVE_UNISTD_H
#endif

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

#ifdef ORIG_XAW
#include "timidity.h"
#else
#include "config.h"
#endif

#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#ifdef ORIG_XAW
#include "miditrace.h"
#include "timer.h"
#endif
#include "xaw.h"
static void ctl_lyric(int lyricid);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
#ifdef ORIG_XAW
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);
#else
static int cmsg(int type, int verbosity_level, const char *fmt, ...);
#endif
static void ctl_pass_playing_list(int number_of_files, const char *list_of_files[]);

static void a_pipe_open(void);
static int a_pipe_ready(void);
static void ctl_master_volume(int);
int a_pipe_read(char *,int);
#ifdef ORIG_XAW
void a_pipe_write(char *);
static void a_pipe_write_msg(char *msg);
#else
void a_pipe_write(const char *);
static void a_pipe_write_msg(char *msg, int lyric);
#endif

static void ctl_refresh(void);
#ifdef ORIG_XAW
static void ctl_current_time(int secs, int v);
static void ctl_total_time(int tt);
static void ctl_note(int status, int ch, int note, int velocity);
static void ctl_program(int ch, int val, void *vp);
#else
static void ctl_current_time(uint32 ct);
static void ctl_total_time(uint32 tt);
static void ctl_file_name(char *name);
static void ctl_xaw_note(int status, int ch, int note, int velocity);
static void ctl_note(int v);
static void ctl_program( int ch, int val, const char *name);
#endif
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

static double indicator_last_update = 0;
#define EXITFLG_QUIT 1
#define EXITFLG_AUTOQUIT 2
static int exitflag=0,randomflag=0,repeatflag=0,selectflag=0;
static int xaw_ready=0;
static int number_of_files;
static char **list_of_files;
static char **titles;
static int *file_table;
extern int amplitude;

/**********************************************/
/* export the interface functions */

#define ctl xaw_control_mode

#ifdef ORIG_XAW
ControlMode ctl=
{
    "XAW interface", 'a',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};
#else

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

#endif

static char local_buf[300];

/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/
#define CMSG_MESSAGE 16

#ifdef ORIG_XAW
static int cmsg(int type, int verbosity_level, char *fmt, ...) {
  char *buff;
  MBlockList pool;
#else
static int cmsg(int type, int verbosity_level, const char *fmt, ...) {
  char buff[2048];
  int flagnl = 1;
#endif
  va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
#ifndef ORIG_XAW
  if (*fmt == '~')
   {
     flagnl = 0;
     fmt++;
   }
#endif
  va_start(ap, fmt);

  if(!xaw_ready) {
    vfprintf(stderr, fmt, ap);
#ifdef ORIG_XAW
    fprintf(stderr, NLS);
#else
    fprintf(stderr, "\n");
#endif
    va_end(ap);
    return 0;
  }

#ifdef ORIG_XAW
  init_mblock(&pool);
  buff = (char *)new_segment(&pool, MIN_MBLOCK_SIZE);
  vsnprintf(buff, MIN_MBLOCK_SIZE, fmt, ap);
  a_pipe_write_msg(buff);
  reuse_mblock(&pool);
#else
  vsprintf(buff, fmt, ap);
  if (flagnl) a_pipe_write_msg(buff, 0);
  else a_pipe_write_msg(buff, 1);
#endif

  va_end(ap);
  return 0;
}

/*ARGSUSED*/
static int tt_i;

#ifdef ORIG_XAW
static void ctl_current_time(int sec, int v) {
  static int previous_sec=-1,last_voices=-1;
  static int last_v=-1, last_time=-1;
#else

static void ctl_current_time(uint32 ct)
{

    int i,v, flags=0;
    int centisecs, realct, sec;
  static int previous_sec=-1,last_voices=-1;
  static int last_v=-1, last_time=-1;
    realct = play_mode->output_count(ct);
    if (realct < 0) realct = 0;
    /* else realct += songoffset; */
    centisecs = realct / (play_mode->rate/100);
  sec = centisecs / 100;
 
    v=0;
    i=voices;
    while (i--)
	if (voice[i].status!=VOICE_FREE) v++;
#endif


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
#ifdef ORIG_XAW
  if(!ctl.trace_playing || midi_trace.flush_flag) return;
#else
  if(!ctl.trace_playing) return;
#endif
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
#ifdef ORIG_XAW
    ctl_current_time(0, 0);
    sprintf(local_buf,"m%d",play_system_mode);
#else
    ctl_current_time(0);
    sprintf(local_buf,"m%d",9);
#endif
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

#ifdef ORIG_XAW
    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
        if(lyric[0] == ME_KARAOKE_LYRIC)
        {
            if(lyric[1] == '/' || lyric[1] == '\\')
            {
                strcpy(lyric_buf, lyric + 2);
                a_pipe_write_msg(lyric_buf);
                lyric_col = strlen(lyric_buf);
            }
            else if(lyric[1] == '@')
            {
                if(lyric[2] == 'L')
                    snprintf(lyric_buf, sizeof(lyric_buf), "Language: %s", lyric + 3);
                else if(lyric[2] == 'T')
                    snprintf(lyric_buf, sizeof(lyric_buf), "Title: %s", lyric + 3);
                else
                    strncpy(lyric_buf, lyric + 1, sizeof(lyric_buf) - 1);
                a_pipe_write_msg(lyric_buf);
                lyric_col = 0;
            }
            else
            {
                strncpy(lyric_buf + lyric_col, lyric + 1, sizeof(lyric_buf) - lyric_col - 1);
                a_pipe_write_msg(lyric_buf);
                lyric_col += strlen(lyric + 1);
            }
        }
        else
        {
            lyric_col = 0;
            a_pipe_write_msg(lyric + 1);
        }
    }
#endif
}

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout) {
  ctl.opened=1;

#ifdef ORIG_XAW
  set_trace_loop_hook(update_indicator);
#else
  ctl.trace_playing = 1;
#endif
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
    char *files[1],**ret,**tmp;
    int i,nfiles,nfit;
    char *p;

    files[0] = additional_path;
    nfiles = 1;
#ifdef ORIG_XAW
    ret = expand_file_archives(files, &nfiles);
    if(ret == NULL)
#endif
      return;
#ifdef ORIG_XAW
    tmp = list_of_files;
    titles=(char **)safe_realloc(titles,(number_of_files+nfiles)*sizeof(char *));
    list_of_files=(char **)safe_malloc((number_of_files+nfiles)*sizeof(char *));
    for (i=0;i<number_of_files;i++)
        list_of_files[i]=safe_strdup(tmp[i]);
    for (i=0,nfit=0;i<nfiles;i++) {
        if(check_midi_file(ret[i]) >= 0) {
            p=strrchr(ret[i],'/');
            if (p==NULL) p=ret[i]; else p++;      
            titles[number_of_files+nfit]=(char *)safe_malloc(sizeof(char)*(strlen(p)+ 9));
            list_of_files[number_of_files+nfit]=safe_strdup(ret[i]);
            sprintf(titles[number_of_files+nfit],"%d. %s",number_of_files+nfit+1,p);
            nfit++;
        }
    }
    if(nfit>0) {
        file_table=(int *)safe_realloc(file_table,
                                       (number_of_files+nfit)*sizeof(int));
        for(i = number_of_files; i < number_of_files + nfit; i++)
            file_table[i] = i;
        number_of_files+=nfit;
        sprintf(local_buf, "X %d", nfit);
        a_pipe_write(local_buf);
        for (i=0;i<nfit;i++)
            a_pipe_write(titles[number_of_files-nfit+i]);
    }
    free(ret[0]);
    free(ret);
#endif
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
#ifdef ORIG_XAW
        file_table=(int *)safe_realloc(file_table,1*sizeof(int));
#else
        file_table=(int *)realloc(file_table,1*sizeof(int));
#endif
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

#ifndef ORIG_XAW

static void ctl_file_name(char *name)
{
#if 0
    pipe_int_write(FILENAME_MESSAGE);
    pipe_string_write(name);
#endif
}
#endif

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
      case 'P' : return RC_LOAD_FILE;
      case 'U' : return RC_TOGGLE_PAUSE;
      case 'f': *valp=(int32)(play_mode->rate * 10);
        return RC_FORWARD;
      case 'b': *valp=(int32)(play_mode->rate * 10);
        return RC_BACK;
      case 'S' : return RC_QUIT;
      case 'N' : return RC_NEXT;
      case 'B' : return RC_REALLY_PREVIOUS;
      case 'R' : repeatflag=atoi(local_buf+2);return RC_NONE;
      case 'D' : randomflag=atoi(local_buf+2);return RC_QUIT;
      case 'd' : n=atoi(local_buf+2);
        xaw_delete_midi_file(atoi(local_buf+2));
        return RC_QUIT;
      case 'A' : xaw_delete_midi_file(-1);
        return RC_QUIT;
      case 'C' : n=atoi(local_buf+2);
#ifdef ORIG_XAW
        opt_chorus_control = n;
#endif
        return RC_QUIT;
      case 'E' : n=atoi(local_buf+2);
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
      case 'L' : selectflag=atoi(local_buf+2);return RC_QUIT;
      case 'T' : a_pipe_read(local_buf,sizeof(local_buf));
        n=atoi(local_buf+2); *valp= n * play_mode->rate;
        return RC_JUMP;
      case 'V' : a_pipe_read(local_buf,sizeof(local_buf));
        amplification=atoi(local_buf+2); *valp=(int32)0;
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
      case 'X': a_pipe_read(local_buf,sizeof(local_buf));
        xaw_add_midi_file(local_buf + 2);
        return RC_NONE;
      case 's':
        xaw_output_flist();return RC_NONE;
#ifdef ORIG_XAW
      case 'g': return RC_TOGGLE_SNDSPEC;
#endif
      case 'q' : exitflag ^= EXITFLG_AUTOQUIT;return RC_NONE;
      case 'Q' :
      default  : exitflag |= EXITFLG_QUIT;return RC_QUIT;
    }
  }
}

static int ctl_read(int32 *valp) {
  if (a_pipe_ready()<=0) return RC_NONE;
  return ctl_blocking_read(valp);
}

static void shuffle(int n,int *a) {
  int i,j,tmp;

  for (i=0;i<n;i++) {
#ifdef ORIG_XAW
    j=int_rand(n);
#else
    j=(int) (((double)n) *rand()/(RAND_MAX+1.0));
#endif
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

#ifdef ORIG_XAW
  sprintf(local_buf,"%d",
  (opt_modulation_wheel<<MODUL_N)
     | (opt_portamento<<PORTA_N)
     | (opt_nrpn_vibrato<<NRPNV_N)
     | (opt_reverb_control<<REVERB_N)
     | (opt_channel_pressure<<CHPRESSURE_N)
     | (opt_overlap_voice_allow<<OVERLAPV_N)
     | (opt_trace_text_meta_event<<TXTMETA_N));
  a_pipe_write(local_buf);
  sprintf(local_buf,"%d",opt_chorus_control);
  a_pipe_write(local_buf);
#else
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
#endif

  /* Make title string */
  titles=(char **)safe_malloc(init_number_of_files*sizeof(char *));
  list_of_files=(char **)safe_malloc(init_number_of_files*sizeof(char *));
  for (i=0,j=0;i<init_number_of_files;i++) {
#ifdef ORIG_XAW
    if(check_midi_file(init_list_of_files[i]) >= 0) {
#else
    {
#endif
      p=strrchr(init_list_of_files[i],'/');
      if (p==NULL) {
#ifdef ORIG_XAW
        p=safe_strdup(init_list_of_files[i]);
#else
        p=strdup(init_list_of_files[i]);
#endif
      } else p++;
#ifdef ORIG_XAW
      list_of_files[j]= safe_strdup(init_list_of_files[i]);
#else
      list_of_files[j]= strdup(init_list_of_files[i]);
#endif
      titles[j]=(char *)safe_malloc(sizeof(char)*(strlen(p)+ 9));
      sprintf(titles[j],"%d. %s",j+1,p);
      j++; number_of_files = j;
    }
  }
#ifdef ORIG_XAW
  titles=(char **)safe_realloc(titles,init_number_of_files*sizeof(char *));
  list_of_files=(char **)safe_realloc(list_of_files,init_number_of_files*sizeof(char *));
#else
  titles=(char **)realloc(titles,init_number_of_files*sizeof(char *));
  list_of_files=(char **)realloc(list_of_files,init_number_of_files*sizeof(char *));
#endif

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
#ifdef ORIG_XAW
      if((title = get_midi_title(list_of_files[current_no])) == NULL)
#endif
      title = list_of_files[current_no];
      snprintf(local_buf,sizeof(local_buf),"e %s", title);
      a_pipe_write(local_buf);
      command=play_midi_file(list_of_files[file_table[current_no]]);
    } else {
      if (command==RC_CHANGE_VOLUME) amplitude+=val;
      if (command==RC_JUMP) ;
#ifdef ORIG_XAW
      if (command==RC_TOGGLE_SNDSPEC) ;
#endif
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
        if (current_no+1<number_of_files) current_no++;
        command=RC_LOAD_FILE;
        continue;
        /* Play the previous */
      } else if (command==RC_REALLY_PREVIOUS) {
        if (current_no>0) current_no--;
        command=RC_LOAD_FILE;
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

#ifdef ORIG_XAW
static void a_pipe_write_msg(char *msg)
#else
static void a_pipe_write_msg(char *msg, int lyric)
#endif
{
    int msglen;
    char buf[2 + sizeof(int)], *p, *q;

#ifdef ORIG_XAW
    /* strip '\r' */
    p = q = msg;
    while(*q) {
    if(*q != '\r')
        *p++ = *q;
    q++;
    }
    *p = '\0';
#endif

#ifdef ORIG_XAW
    msglen = strlen(msg) + 1; /* +1 for '\n' */
#else
    msglen = strlen(msg);
    if (~lyric) msglen++;
#endif
    buf[0] = 'L';
    buf[1] = '\n';

    memcpy(buf + 2, &msglen, sizeof(int));
    write(pipe_out_fd, buf, sizeof(buf));
#ifdef ORIG_XAW
    write(pipe_out_fd, msg, msglen - 1);
    write(pipe_out_fd, "\n", 1);
#else
    if (lyric) write(pipe_out_fd, msg, msglen);
    else {
        write(pipe_out_fd, msg, msglen - 1);
        write(pipe_out_fd, "\n", 1);
    }
#endif
}

#ifdef ORIG_XAW
static void
ctl_note(int status, int ch, int note, int velocity)
#else
static void
ctl_xaw_note(int status, int ch, int note, int velocity)
#endif
{
  char c;

  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
#ifdef ORIG_XAW
  if(!ctl.trace_playing || midi_trace.flush_flag) return;
#else
  if(!ctl.trace_playing) return;
#endif
 
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

#ifndef ORIG_XAW
static void ctl_note(int v)
{
	int ch, note, vel;

	if (!ctl.trace_playing) 
		return;

	if (voice[v].clone_type != 0) return;

	ch = voice[v].channel;
	ch &= 0x1f;
	if (ch < 0 || ch >= MAXCHAN) return;

	note = voice[v].note;
	vel = voice[v].velocity;
	ctl_xaw_note(voice[v].status, ch, note, vel);
}
#endif

#ifdef ORIG_XAW
static void ctl_program(int ch, int val, void *comm)
#else
static void ctl_program( int ch, int val, const char *name)
#endif
{
  if(ch >= MAX_XAW_MIDI_CHANNELS) return;
  if(!ctl.trace_playing) return;

#ifdef ORIG_XAW
  if(channel[ch].special_sample)
    val = channel[ch].special_sample;
  else
    val += progbase;
#endif
  sprintf(local_buf, "PP%c%d", ch+'A', val);
  a_pipe_write(local_buf);
#ifdef ORIG_XAW
  if (comm != NULL) {
    sprintf(local_buf, "I%c%s", ch+'A', (char *)comm);
    if (ISDRUMCHANNEL(ch))
      sprintf(local_buf, "I%c%s", ch+'A',
              (!strlen((char *)comm))? "<drum>":(char *)comm);
    a_pipe_write(local_buf);
  }
#else
  if (name) sprintf(local_buf, "I%c%s", ch+'A', name);
  else sprintf(local_buf, "I%c%s", ch+'A', "");
    a_pipe_write(local_buf);
#endif
}

#ifdef ORIG_XAW
static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
    case CTLE_LOADING_DONE:
      break;
    case CTLE_CURRENT_TIME:
      ctl_current_time((int)e->v1, (int)e->v2);
      break;
    case CTLE_PLAY_START:
      ctl_total_time((int)e->v1);
      break;
    case CTLE_PLAY_END:
      break;
    case CTLE_TEMPO:
      break;
    case CTLE_METRONOME:
      break;
    case CTLE_NOTE:
      ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4);
      break;
    case CTLE_PROGRAM:
      ctl_program((int)e->v1, (int)e->v2, (char *)e->v3);
      break;
    case CTLE_VOLUME:
      ctl_volume((int)e->v1, (int)e->v2);
      break;
    case CTLE_EXPRESSION:
      ctl_expression((int)e->v1, (int)e->v2);
      break;
    case CTLE_PANNING:
      ctl_panning((int)e->v1, (int)e->v2);
      break;
    case CTLE_SUSTAIN:
      ctl_sustain((int)e->v1, (int)e->v2);
      break;
    case CTLE_PITCH_BEND:
      ctl_pitch_bend((int)e->v1, (int)e->v2);
      break;
    case CTLE_MOD_WHEEL:
      ctl_pitch_bend((int)e->v1, e->v2 ? -1 : 0x2000);
      break;
    case CTLE_CHORUS_EFFECT:
      set_otherinfo((int)e->v1, (int)e->v2, 'c');
      break;
    case CTLE_REVERB_EFFECT:
      set_otherinfo((int)e->v1, (int)e->v2, 'r');
      break;
    case CTLE_LYRIC:
      ctl_lyric((int)e->v1);
      break;
    case CTLE_MASTER_VOLUME:
      ctl_master_volume((int)e->v1);
      break;
    case CTLE_REFRESH:
      ctl_refresh();
      break;
    case CTLE_RESET:
      ctl_reset();
      break;

    }
}
#endif

static void ctl_refresh(void)
{
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

#ifdef ORIG_XAW
  indicator_last_update = get_current_calender_time();
#else
  indicator_last_update = 0;
#endif
  for (i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
#ifdef ORIG_XAW
    if(ISDRUMCHANNEL(i)) {
      ctl_program(i, channel[i].bank, channel_instrum_name(i));
#else
    if (channel[i].kit) {
      ctl_program(i, channel[i].bank, channel[i].name);
#endif
#ifdef ORIG_XAW
      if (opt_reverb_control)
        set_otherinfo(i, get_reverb_level(i), 'r');
#else
        set_otherinfo(i, channel[i].reverberation, 'r');
#endif
    } else {
      ToneBank *bank;
      int b;

#ifdef ORIG_XAW
      ctl_program(i, channel[i].program, channel_instrum_name(i));
#else
      ctl_program(i, channel[i].program, channel[i].name);
#endif
      b = channel[i].bank;
#ifdef ORIG_XAW
      if((bank = tonebank[b]) == NULL
         || bank->tone[channel[i].program].instrument == NULL)  {
          b = 0;
          bank = tonebank[0];
      }
#endif
      set_otherinfo(i, channel[i].bank, 'b');
#ifdef ORIG_XAW
      if (opt_reverb_control)
        set_otherinfo(i, get_reverb_level(i), 'r');

      if(opt_chorus_control)
        set_otherinfo(i, get_chorus_level(i), 'c');
#else
        set_otherinfo(i, channel[i].reverberation, 'r');
        set_otherinfo(i, channel[i].chorusdepth, 'c');
#endif
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
  sprintf(local_buf, "R");
  a_pipe_write(local_buf);  
}

static void update_indicator(void)
{
    double t, diff;

    if(!ctl.trace_playing)
        return;
#ifdef ORIG_XAW
    t = get_current_calender_time();
#else
    t = indicator_last_update + 1;
#endif

    diff = t - indicator_last_update;
    if(diff > XAW_UPDATE_TIME)
    {
       sprintf(local_buf, "U");
       a_pipe_write(local_buf);
       indicator_last_update = t;
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_a_loader(void)
{
    return &ctl;
}
