/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    In case you haven't heard, this program is free software; 
    you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    mix.h

*/

extern void mix_voice(int32 *buf, int v, uint32 c);
extern int recompute_envelope(int v);
#ifdef tplus
extern int apply_envelope_to_amp(int v);
#else
extern void apply_envelope_to_amp(int v);
#endif
