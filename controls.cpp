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

   controls.c
   
   */

#include "config.h"
#include "controls.h"

int last_rc_command = 0;
int last_rc_arg = 0;

int check_for_rc(void) {
  int rc = last_rc_command;
  int32 val;

  if (!rc)
    {
	rc = ctl->read(&val);
	last_rc_command = rc;
	last_rc_arg = val;
    }

  switch (rc)
    {
      case RC_QUIT: /* [] */
      case RC_LOAD_FILE:	  
      case RC_NEXT: /* >>| */
      case RC_REALLY_PREVIOUS: /* |<< */
      case RC_PATCHCHANGE:
      case RC_CHANGE_VOICES:
      case RC_STOP:
	return rc;
      default:
	return 0;
    }
}

#ifdef KMIDI
  extern ControlMode kmidi_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &kmidi_control_mode
# endif
#endif

#ifdef IA_GTK
  extern ControlMode gtk_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &gtk_control_mode
# endif
#endif

#ifdef IA_MOTIF
  extern ControlMode motif_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &motif_control_mode
# endif
#endif

#ifdef IA_XAW
  extern ControlMode xaw_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &xaw_control_mode
# endif
#endif

#ifdef TCLTK
  extern ControlMode tk_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &tk_control_mode
# endif
#endif

#ifdef IA_NCURSES
  extern ControlMode ncurses_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &ncurses_control_mode
# endif
#endif

#ifdef IA_SLANG
  extern ControlMode slang_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &slang_control_mode
# endif
#endif

/* Minimal control mode */
extern ControlMode dumb_control_mode;
#ifndef DEFAULT_CONTROL_MODE
# define DEFAULT_CONTROL_MODE &dumb_control_mode
#endif

ControlMode *ctl_list[]={
#ifdef IA_NCURSES
  &ncurses_control_mode,
#endif
#ifdef IA_SLANG
  &slang_control_mode,
#endif
#ifdef IA_MOTIF
  &motif_control_mode,
#endif
#ifdef IA_XAW
  &xaw_control_mode,
#endif
#ifdef KMIDI
  &kmidi_control_mode,
#endif
#ifdef IA_GTK
  &gtk_control_mode,
#endif
#ifdef TCLTK
  &tk_control_mode,
#endif
  &dumb_control_mode,
  0
};

ControlMode *ctl=DEFAULT_CONTROL_MODE;
