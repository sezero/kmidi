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

   readmidi.h 
   
   */

typedef struct {
  MidiEvent event;
  void *next;
} MidiEventList;

extern int32 quietchannels;

extern MidiEvent *read_midi_file(FILE *mfp, uint32 *count, uint32 *sp);

struct meta_text_type {
    unsigned long time;
    unsigned char type;
    char *text;
    struct meta_text_type *next;
};

extern struct meta_text_type *meta_text_list;

