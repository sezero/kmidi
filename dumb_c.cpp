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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    dumb_c.c
    Minimal control mode -- no interaction, just prints out messages.
    */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "config.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"

static void ctl_refresh(void);
static void ctl_total_time(uint32 tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(uint32 ct);
static void ctl_note(int v);
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

/**********************************/
/* export the interface functions */

#define ctl dumb_control_mode

ControlMode ctl= 
{
  "dumb interface", 'd',
  1,0,0,
  ctl_open,dumb_pass_playing_list, ctl_close, ctl_read, cmsg,
  ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time, ctl_current_time, 
  ctl_note, 
  ctl_master_volume, ctl_program, ctl_volume, 
  ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};

static FILE *infp;  /* infp isn't actually used yet */
static FILE *outfp;

static int ctl_open(int using_stdin, int using_stdout)
{
  infp = stdin; outfp = stdout;
  if (using_stdin && using_stdout)
    infp=outfp=stderr;
  else if (using_stdout)
    outfp=stderr;
  else if (using_stdin)
    infp=stdout;

  ctl.opened=1;
  return 0;
}

static void ctl_close(void)
{ 
  fflush(outfp);
  ctl.opened=0;
}

static int ctl_read(int32 *valp)
{
  return RC_NONE;
}

static int cmsg(int type, int verbosity_level, const char *fmt, ...)
{
  va_list ap;
  int flag_newline = 1;
#ifdef ADAGIO
  if (interactive) return 0;
#endif
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  if (*fmt == '~')
    {
      flag_newline = 0;
      fmt++;
    }
  va_start(ap, fmt);
  if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      if (flag_newline) fprintf(stderr, "\n");
    }
  else
    {
      vfprintf(outfp, fmt, ap);
      if (flag_newline) fprintf(outfp, "\n");
    }
  va_end(ap);
  if (!flag_newline) fflush(outfp);
  return 0;
}

static void ctl_refresh(void) { }

static void ctl_total_time(uint32 tt)
{
  uint32 mins, secs;
  if (ctl.trace_playing)
    {
      secs=tt/play_mode->rate;
      mins=secs/60;
      secs-=mins*60;
      fprintf(outfp, "Total playing time: %3ld min %02ld s\n", mins, secs);
    }
}

static void ctl_master_volume(int mv) { mv = 0; }

static void ctl_file_name(char *name)
{
  if (ctl.verbosity>=0 || ctl.trace_playing)
    fprintf(outfp, "Playing %s\n", name);
}

static void ctl_current_time(uint32 ct)
{
  uint32 mins, secs;
  if (ctl.trace_playing)
    {
      secs=ct/play_mode->rate;
      mins=secs/60;
      secs-=mins*60;
      fprintf(outfp, "\r%3ld:%02ld", mins, secs);
      fflush(outfp);
    }
}

static void ctl_note(int v) { v = 0; }

static void ctl_program(int ch, int val, const char *name) { ch = val = 0; name = 0; }

static void ctl_volume(int ch, int val) { ch = val = 0; }

static void ctl_expression(int ch, int val) { ch = val = 0; }

static void ctl_panning(int ch, int val) { ch = val = 0; }

static void ctl_sustain(int ch, int val) { ch = val = 0; }

static void ctl_pitch_bend(int ch, int val) { ch = val = 0; }

static void ctl_reset(void) {}
