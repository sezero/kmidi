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

    ncurs_c.c
   
    */

#ifdef IA_NCURSES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include "config.h"

#ifdef HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#else
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "version.h"

static void ctl_refresh(void);
static void ctl_help_mode(void);
static void ctl_total_time(uint32 tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(uint32 ct);
static void ctl_note(int v);
static void ctl_note_display(void);
static void ctl_program(int ch, int val, const char *name);
static void ctl_volume(int ch, int val);
static void ctl_expression(int ch, int val);
static void ctl_panning(int ch, int val);
static void ctl_sustain(int ch, int val);
static void ctl_pitch_bend(int ch, int val);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, const char *fmt, ...);

/**********************************************/
/* export the interface functions */

#undef ctl
#define ctl ncurses_control_mode

ControlMode ctl= 
{
  "ncurses interface", 'n',
  1,0,0,
  ctl_open,dumb_pass_playing_list, ctl_close, ctl_read, cmsg,
  ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time, ctl_current_time, 
  ctl_note, 
  ctl_master_volume, ctl_program, ctl_volume, 
  ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};


/***********************************************************************/
/* foreground/background checks disabled since switching to curses */
/* static int in_foreground=1; */
static int ctl_helpmode=0;
static int maxy, maxx;
static int save_master_volume;
static int save_total_time;
static char save_file_name[50];
static int screen_bugfix = 0;
static uint32 current_centiseconds = 0;
static int songoffset = 0, current_voices = 0;

extern MidiEvent *current_event;

typedef struct {
  uint8 status, channel, note, velocity, voices;
  uint32 time;
} OldVoice;
#define MAX_OLD_NOTES 500
static OldVoice old_note[MAX_OLD_NOTES];
static int leading_pointer=0, trailing_pointer=0;

static int tr_lm, tr_nw;
#define MAX_PNW 12
static char patch_name[16][MAX_PNW];

static char short_cfg_names[4][6];

static int my_bg = COLOR_BLACK;

static void set_color(WINDOW *win, chtype color)
{
	if (has_colors()) {
		static bool *pairs;
		int n = (color + 1);
		if (pairs == 0)
			pairs = (bool *)calloc(COLORS+1, sizeof(bool));
		if (!pairs[n]) {
			init_pair(n, color, my_bg);
			pairs[n] = TRUE;
		}
		wattroff(win, A_COLOR);
		wattron(win, COLOR_PAIR(n));
	}
}

static void unset_color(WINDOW *win)
{
	if (has_colors())
		wattrset(win, COLOR_PAIR(0));
}


static WINDOW *dftwin=0, *msgwin=0;

static void _ctl_refresh(void)
{
  wmove(dftwin, 0,0);
  wrefresh(dftwin);
}

static void ctl_refresh(void)
{
  if (ctl.trace_playing)
    {
       ctl_current_time(0);
       ctl_note_display();
       wmove(dftwin, 4,48);
       wprintw(dftwin, (char *)"%2d", current_voices);
    }
    _ctl_refresh();
}

/* This is taken from TiMidity++. */
static void re_init_screen(void)
{
    if(screen_bugfix)
	return;
    screen_bugfix = 1;
  wmove(dftwin, 1,3);
  wprintw(dftwin, (char *)"patchset: %-20s interpolation: %-8s",
	cfg_names[cfg_select]? cfg_names[cfg_select] : "?",
	current_interpolation? ((current_interpolation==1)? "C-spline":"LaGrange") : "linear" );
    touchwin(dftwin);
    _ctl_refresh();
    if(msgwin)
    {
	touchwin(msgwin);
	wrefresh(msgwin);
    }
}

static void ctl_help_mode(void)
{
  static WINDOW *helpwin;
  if (ctl_helpmode)
    {
      ctl_helpmode=0;
      delwin(helpwin);
      touchwin(dftwin);
      _ctl_refresh();
    }
  else
    {
      ctl_helpmode=1;
      /* And here I thought the point of curses was that you could put
	 stuff on windows without having to worry about whether this
	 one is overlapping that or the other way round... */
      helpwin=newwin(2,maxx,0,0);
      wattron(helpwin, A_REVERSE);
      werase(helpwin); 
      waddstr(helpwin, 
	      "V/Up=Louder    b/Left=Skip back      "
	      "n/Next=Next file      r/Home=Restart file");
      wmove(helpwin, 1,0);
      waddstr(helpwin, 
	      "v/Down=Softer  f/Right=Skip forward  "
	      "p/Prev=Previous file  q/End=Quit program");
      wattroff(helpwin, A_REVERSE);
      wrefresh(helpwin);
    }
}

static void ctl_total_time(uint32 tt)
{
  int mins, secs=(int)tt/play_mode->rate;
  save_total_time = tt;
  mins=secs/60;
  secs-=mins*60;

  wmove(dftwin, 4,6+6+3);
  wattron(dftwin, A_BOLD);
  wprintw(dftwin, (char *)"%3d:%02d", mins, secs);
  wattroff(dftwin, A_BOLD);
  _ctl_refresh();

  songoffset = 0;
  current_centiseconds = 0;
  for (secs=0; secs<16; secs++) patch_name[secs][0] = '\0';
}

static void ctl_master_volume(int mv)
{
  save_master_volume = mv;
  wmove(dftwin, 4,maxx-5);
  wattron(dftwin, A_BOLD);
  wprintw(dftwin, (char *)"%3d%%", mv);
  wattroff(dftwin, A_BOLD);
  _ctl_refresh();
}

static void ctl_file_name(char *name)
{
  strncpy(save_file_name, name, 50);
  save_file_name[49] = '\0';
  wmove(dftwin, 3,6);
  wclrtoeol(dftwin );
  wattron(dftwin, A_BOLD);
  waddstr(dftwin, name);
  wattroff(dftwin, A_BOLD);
  _ctl_refresh();
}

static void ctl_current_time(uint32 ct)
{
  int centisecs, realct;
  int mins, secs;

  if (!ctl.trace_playing) 
    return;

  realct = play_mode->output_count(ct);
  if (realct < 0) realct = 0;
  else realct += songoffset;
  centisecs = realct / (play_mode->rate/100);
  current_centiseconds = (uint32)centisecs;

  secs=centisecs/100;
  mins=secs/60;
  secs-=mins*60;
  wmove(dftwin, 4,6);
  if (has_colors()) set_color(dftwin, COLOR_GREEN);
  else wattron(dftwin, A_BOLD);
  wprintw(dftwin, (char *)"%3d:%02d", mins, secs);
  if (has_colors()) unset_color(dftwin);
  else wattroff(dftwin, A_BOLD);
  _ctl_refresh();
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

#define LOW_CLIP 32
#define HIGH_CLIP 59

static void ctl_note_display(void)
{
  int v = trailing_pointer;
  uint32 then;

  if (tr_nw < 10) return;

  then = old_note[v].time;

  while (then <= current_centiseconds && v != leading_pointer)
    {
  	int xl, new_note;
	new_note = old_note[v].note - LOW_CLIP;
	if (new_note < 0) new_note = 0;
	if (new_note > HIGH_CLIP) new_note = HIGH_CLIP;
	xl = (new_note * tr_nw) / (HIGH_CLIP+1);
	wmove(dftwin, 8+(old_note[v].channel & 0x0f),xl + tr_lm);
	switch(old_note[v].status)
	  {
	    case VOICE_DIE:
	      waddch(dftwin, ACS_CKBOARD);
	      break;
	    case VOICE_FREE: 
	      waddch(dftwin, ACS_BULLET);
	      break;
	    case VOICE_ON:
	      if (has_colors())
		{
		  if (channel[old_note[v].channel].kit) set_color(dftwin, COLOR_YELLOW);
		  else set_color(dftwin, COLOR_RED);
		}
	      else wattron(dftwin, A_BOLD);
	      waddch(dftwin, '0'+(10*old_note[v].velocity)/128); 
	      if (has_colors())	unset_color(dftwin);
	      else wattroff(dftwin, A_BOLD);
	      break;
	    case VOICE_OFF:
	    case VOICE_SUSTAINED:
	      if (has_colors()) set_color(dftwin, COLOR_GREEN);
	      waddch(dftwin, '0'+(10*old_note[v].velocity)/128);
	      if (has_colors()) unset_color(dftwin);
	      break;
	  }
	current_voices = old_note[v].voices;
	v++;
	if (v == MAX_OLD_NOTES) v = 0;
	then = old_note[v].time;
    }
  trailing_pointer = v;
}

static void ctl_program(int ch, int val, const char *name)
{
  int realch = ch;
  if (!ctl.trace_playing) 
    return;
  ch &= 0x0f;

  if (name && realch<16)
    {
      strncpy(patch_name[ch], name, MAX_PNW);
      patch_name[ch][MAX_PNW-1] = '\0';
    }
  if (tr_lm > 3)
    {
      wmove(dftwin, 8+ch, 3);
      if (patch_name[ch][0])
        wprintw(dftwin, (char *)"%-12s", patch_name[ch]);
      else waddstr(dftwin, "            ");
    }

  wmove(dftwin, 8+ch, maxx-20);
  if (channel[ch].kit)
    {
      if (has_colors()) set_color(dftwin, COLOR_YELLOW);
      else wattron(dftwin, A_BOLD);
      wprintw(dftwin, (char *)"%03d", val);
      if (has_colors()) unset_color(dftwin);
      else wattroff(dftwin, A_BOLD);
    }
  else
    wprintw(dftwin, (char *)"%03d", val);
}

static void ctl_volume(int ch, int val)
{
  if (!ctl.trace_playing) 
    return;
  ch &= 0x0f;
  wmove(dftwin, 8+ch, maxx-16);
  wprintw(dftwin, (char *)"%3d", (val*100)/127);
}

static void ctl_expression(int ch, int val)
{
  if (!ctl.trace_playing) 
    return;
  ch &= 0x0f;
  wmove(dftwin, 8+ch, maxx-12);
  wprintw(dftwin, (char *)"%3d", (val*100)/127);
}

static void ctl_panning(int ch, int val)
{
  if (!ctl.trace_playing) 
    return;
  ch &= 0x0f;
  wmove(dftwin, 8+ch, maxx-8);
  if (val != NO_PANNING && has_colors())
    {
      if (val <= 60) set_color(dftwin, COLOR_CYAN);
      if (val >= 68) set_color(dftwin, COLOR_GREEN);
    }
  if (val==NO_PANNING)
    waddstr(dftwin, "   ");
  else if (val<5)
    waddstr(dftwin, " L ");
  else if (val>123)
    waddstr(dftwin, " R ");
  else if (val>60 && val<68)
    waddstr(dftwin, " C ");
  else
    {
      /* wprintw(dftwin, "%+02d", (100*(val-64))/64); */
      val = (100*(val-64))/64; /* piss on curses */
      if (val<0)
	{
	  waddch(dftwin, '-');
	  val=-val;
	}
      else waddch(dftwin, '+');
      wprintw(dftwin, (char *)"%02d", val);
    }
  if (val != NO_PANNING && has_colors())
    {
      unset_color(dftwin);
    }
}

static void ctl_sustain(int ch, int val)
{
  if (!ctl.trace_playing) 
    return;
  ch &= 0x0f;
  wmove(dftwin, 8+ch, maxx-4);
  if (val) waddch(dftwin, 'S');
  else waddch(dftwin, ' ');
}

static void ctl_pitch_bend(int ch, int val)
{
  if (!ctl.trace_playing) 
    return;
  ch &= 0x0f;
  wmove(dftwin, 8+ch, maxx-2);
  if (val>0x2000) waddch(dftwin, '+');
  else if (val<0x2000) waddch(dftwin, '-');
  else waddch(dftwin, ' ');
}

static void ctl_reset(void)
{
  int i,j;
  if (!ctl.trace_playing) 
    return;

  for (i=0; i<16; i++)
    {
      wmove(dftwin, 8+i, tr_lm);
      if (tr_nw >= 10)
        for (j=0; j<tr_nw; j++) waddch(dftwin, ACS_BULLET);
      ctl_program(i, channel[i].program, 0);
      ctl_volume(i, channel[i].volume);
      ctl_expression(i, channel[i].expression);
      ctl_panning(i, channel[i].panning);
      ctl_sustain(i, channel[i].sustain);
      ctl_pitch_bend(i, channel[i].pitchbend);
    }
  for (i=0; i<MAX_OLD_NOTES; i++)
    {
      old_note[i].time = 0;
    }
  leading_pointer = trailing_pointer = current_voices = 0;
  _ctl_refresh();
}

/***********************************************************************/

/*#define CURSED_REDIR_HACK*/

#ifdef CURSED_REDIR_HACK
static SCREEN *oldscr;
#endif

static void draw_windows(void)
{
  int i;
  tr_lm = 3;
  tr_nw = maxx - 25;
  if (tr_nw > MAX_PNW + 1 + 20)
    {
	tr_nw -= MAX_PNW + 1;
	tr_lm += MAX_PNW + 1;
    }
  werase(dftwin);
  wmove(dftwin, 0,0);
  waddstr(dftwin, "TiMidity v" TIMID_VERSION);
  wmove(dftwin, 0,maxx-52);
  waddstr(dftwin, "(C) 1995 Tuukka Toivonen <toivonen@clinet.fi>");
/*
  wmove(dftwin, 1,0);
  waddstr(dftwin, "Press 'h' for help with keys, or 'q' to quit.");
*/
  wmove(dftwin, 1,3);
  wprintw(dftwin, (char *)"patchset: %-20s interpolation: %-8s",
	cfg_names[cfg_select]? cfg_names[cfg_select] : "?",
	current_interpolation? ((current_interpolation==1)? "C-spline":"LaGrange") : "linear" );
  wmove(dftwin, 3,0);
  waddstr(dftwin, "File:");
  wmove(dftwin, 4,0);
  if (ctl.trace_playing)
    {
      waddstr(dftwin, "Time:");
      wmove(dftwin, 4,6+6+1);
      waddch(dftwin, '/');
      wmove(dftwin, 4,40);
      wprintw(dftwin, (char *)"Voices:    / %d", voices);
    }
  else
    {
      waddstr(dftwin, "Playing time:");
    }
  wmove(dftwin, 4,maxx-20);
  waddstr(dftwin, "Master volume:");
  if (save_master_volume) ctl_master_volume(save_master_volume);
  if (save_total_time) ctl_total_time(save_total_time);
  if (save_file_name[0]) ctl_file_name(save_file_name);
  wmove(dftwin, 5,0);
  for (i=0; i<maxx; i++)
    waddch(dftwin, ACS_HLINE);
  if (ctl.trace_playing)
    {
      wmove(dftwin, 6,0);
      waddstr(dftwin, "Ch");
      wmove(dftwin, 6,maxx-20);
      waddstr(dftwin, "Prg Vol Exp Pan S B");
      wmove(dftwin, 7,0);
      for (i=0; i<maxx; i++)
	waddch(dftwin, ACS_HLINE);
      for (i=0; i<16; i++)
	{
	  wmove(dftwin, 8+i, 0);
	  wprintw(dftwin, (char *)"%02d", i+1);
	}
    }
  else
    {
      if (msgwin) wresize(msgwin, maxy-6, maxx);
      else
	{
          msgwin=newwin(maxy-6,maxx,6,0);
          scrollok(msgwin, TRUE);
          werase(msgwin);
	}
      touchwin(msgwin);
      wrefresh(msgwin);
    }
  touchwin(dftwin);
  wrefresh(dftwin);
  screen_bugfix = 0;
}

static int ctl_open(int using_stdin, int using_stdout)
{
  int i;
#ifdef CURSED_REDIR_HACK
  FILE *infp=stdin, *outfp=stdout;
  SCREEN *dftscr;

  /* This doesn't work right */
  if (using_stdin && using_stdout)
    {
      infp=outfp=stderr;
      fflush(stderr);
      setvbuf(stderr, 0, _IOFBF, BUFSIZ);
    }
  else if (using_stdout)
    {
      outfp=stderr;
      fflush(stderr);
      setvbuf(stderr, 0, _IOFBF, BUFSIZ);
    }
  else if (using_stdin)
    {
      infp=stdout;
      fflush(stdout);
      setvbuf(stdout, 0, _IOFBF, BUFSIZ);
    }

  slk_init(3);
  dftscr=newterm(0, outfp, infp);
  if (!dftscr)
    return -1;
  oldscr=set_term(dftscr);
  /* dftwin=stdscr; */
#else
  slk_init(3);
  initscr();
#endif

  cbreak();
  noecho();
  nonl();
  nodelay(stdscr, 1);
  scrollok(stdscr, 0);
  /*leaveok(stdscr, 1);*/
  idlok(stdscr, 1);
  keypad(stdscr, TRUE);


  if (has_colors())
    {
	start_color();
#ifdef HAVE_USE_DEFAULT_COLORS
	if (use_default_colors() == OK)
	    my_bg = -1;
#endif
    }

  curs_set(0);
  getmaxyx(stdscr,maxy,maxx);
  ctl.opened=1;

  if (ctl.trace_playing)
    dftwin=stdscr;
  else
    dftwin=newwin(6,maxx,0,0);

  slk_set(1, "Help", 1);
  slk_set(2, "Lnear", 1);
  slk_set(3, "Cspln", 1);
  slk_set(4, "Lgrng", 1);

  for (i=0; i<4; i++)
    {
      if (cfg_names[i]) strncpy(short_cfg_names[i], cfg_names[i], 5);
      else strcpy(short_cfg_names[i], "(nne)");
      short_cfg_names[i][5] = '\0';
      slk_set(i+5, short_cfg_names[i], 1);
    }

  slk_set(10, "Quit", 1);
  if (opt_dry) slk_set(11, "Dry", 1);
  else slk_set(11, "Wet", 1);
  slk_refresh();
  draw_windows();
  
  return 0;
}

static void ctl_close(void)
{
  if (ctl.opened)
    {
	refresh();
	endwin();
	curs_set(1);
	ctl.opened=0;
    }
}

static int ctl_read(int32 *valp)
{
  int c;
  if (last_rc_command)
    {
	*valp = last_rc_arg;
	c = last_rc_command;
	last_rc_command = 0;
	return c;
    }
  while ((c=getch())!=ERR)
    {
      re_init_screen();

      switch(c)
	{
	case 'h':
	case '?':
	case KEY_F(1):
	  ctl_help_mode();
	  return RC_NONE;
	  
	case 'V':
	case KEY_UP:
	  *valp=10;
	  return RC_CHANGE_VOLUME;
	case 'v':
	case KEY_DOWN:
	  *valp=-10;
	  return RC_CHANGE_VOLUME;
	case 'q':
	case KEY_END:
	case KEY_F(10):
	  return RC_QUIT;
	case 'n':
	case KEY_NPAGE:
	  return RC_NEXT;
	case 'p':
	case KEY_PPAGE:
	  return RC_REALLY_PREVIOUS;
	case 'r':
	case KEY_HOME:
	  return RC_RESTART;

	case 'f':
	case KEY_RIGHT:
	 songoffset +=
	  *valp=play_mode->rate;
	  return RC_FORWARD;
	case 'b':
	case KEY_LEFT:
	 songoffset -=
	  *valp=play_mode->rate;
	 if (songoffset<0) songoffset=0;
	  return RC_BACK;
#ifdef KEY_RESIZE
	case KEY_RESIZE:
  	  getmaxyx(stdscr,maxy,maxx);
	  draw_windows();
	  ctl_reset();
	  return RC_NONE;
#endif
	case KEY_F(2):
	  current_interpolation = 0;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_NONE;
	case KEY_F(3):
	  current_interpolation = 1;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_NONE;
	case KEY_F(4):
	  current_interpolation = 2;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_NONE;
	case KEY_F(5):
	  cfg_select = 0;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_PATCHCHANGE;
	case KEY_F(6):
	  cfg_select = 1;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_PATCHCHANGE;
	case KEY_F(7):
	  cfg_select = 2;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_PATCHCHANGE;
	case KEY_F(8):
	  cfg_select = 3;
	  screen_bugfix = 0;
          re_init_screen();
	  return RC_PATCHCHANGE;
	case KEY_F(11):
	  if (!opt_dry)
	    {
		opt_dry = 1;
		slk_set(11, "Dry", 1);
	    }
	  else
	    {
		opt_dry = 0;
		slk_set(11, "Wet", 1);
	    }
	  slk_refresh();
	  return RC_NONE;
	case KEY_F(12):
	  return RC_NONE;
	  /* case ' ':
	     return RC_TOGGLE_PAUSE; */
	}
    }
    re_init_screen();

  return RC_NONE;
}

static int cmsg(int type, int verbosity_level, const char *fmt, ...)
{
  va_list ap;
  int flagnl = 1;
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  if (*fmt == '~')
   {
     flagnl = 0;
     fmt++;
   }
  va_start(ap, fmt);
  if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
    }
  else if (ctl.trace_playing)
    {
      switch(type)
	{
	  /* Pretty pointless to only have one line for messages, but... */
	case CMSG_WARNING:
	case CMSG_ERROR:
	case CMSG_FATAL:
	  wmove(dftwin, 2,0);
	  wclrtoeol(dftwin);
	  if (has_colors()) set_color(dftwin, COLOR_YELLOW);
	  else wattron(dftwin, A_REVERSE);
	  vwprintw(dftwin, (char *)fmt, ap);
	  if (has_colors()) unset_color(dftwin);
	  else wattroff(dftwin, A_REVERSE);
	  _ctl_refresh();
	  if (type==CMSG_WARNING)
	    sleep(1); /* Don't you just _HATE_ it when programs do this... */
	  else
	    sleep(2);
	  wmove(dftwin, 2,0);
	  wclrtoeol(dftwin);
	  _ctl_refresh();
	  break;
	}
    }
  else
    {
      switch(type)
	{
	default:
	  vwprintw(msgwin, (char *)fmt, ap);
	  if (flagnl) waddch(msgwin, '\n');
	  break;

	case CMSG_WARNING:
	  if (has_colors()) set_color(msgwin, COLOR_YELLOW);
	  else wattron(msgwin, A_BOLD);
	  vwprintw(msgwin, (char *)fmt, ap);
	  if (has_colors()) unset_color(msgwin);
	  else wattroff(msgwin, A_BOLD);
	  waddch(msgwin, '\n');
	  break;
	  
	case CMSG_ERROR:
	case CMSG_FATAL:
	  if (has_colors()) set_color(msgwin, COLOR_RED);
	  else wattron(msgwin, A_REVERSE);
	  vwprintw(msgwin, (char *)fmt, ap);
	  if (has_colors()) unset_color(msgwin);
	  else wattroff(msgwin, A_REVERSE);
	  waddch(msgwin, '\n');
	  if (type==CMSG_FATAL)
	    {
      	      wrefresh(msgwin);
	      sleep(2);
	    }
	  break;
	}
      wrefresh(msgwin);
    }

  va_end(ap);
  return 0;
}

#endif
