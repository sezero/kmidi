#
#	$Id$
#
#                           KMidi Makefile
#
#  Copyright 1997 Bernd Johannes Wuebben, wuebben@math.cornell.edu
#

VERSION = 0.2i

INCLUDES = $(X_INCLUDES) `$(ARTSCCONFIG) --cflags`
AM_CFLAGS = -DDEFAULT_PATH=\"$(TIMID_DIR)\" $(EXTRATDEFS) $(EXTRATCFLAGS)
LDFLAGS = `$(ARTSCCONFIG) --libs`
LDADD = $(LIBALIB) $(LIBAUDIO) $(LIBMME) $(EXTRATLIBS) $(X_LIBS) -lm

# Where to install the patches, config files, and MIDI files.
# If you change this, it's a good idea to recompile the binary,
# or you'll need to invoke timidity with the -L option.
TIMID_DIR = /$(kde_datadir)/kmidi/config

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = timidity.desktop

bin_PROGRAMS = timidity

timidity_SOURCES = timidity.c common.c readmidi.c playmidi.c resample.c mix.c instrum.c \
        tables.c controls.c output.c filter.c \
        wave_a.c raw_a.c dumb_c.c fffload.c \
        sndfont.c readsbk.c \
	effects.c reverb_e.c chorus_e.c phaser_e.c celeste_e.c \
	resample_l.c resample_f.c cfg.c \
	b_out.c hpux_a.c linux_a.c linux_a2.c sun_a.c dec_a.c \
	hpux_d_a.c alsa_a.c arts_a.c \
	bsd20_a.c esd_a.c nas_a.c \
	ncurs_c.c slang_c.c tk_c.c gtk_i.c ctl.c \
	motif_c.c motif_i.c motif_p.c xaw_i.c xaw_c.c

noinst_HEADERS = config.h common.h readmidi.h playmidi.h resample.h mix.h instrum.h \
        tables.h controls.h output.h filter.h motif.h xaw.h ctl.h

#cfg.c:
#	rm -f cfg.c
#	flex -t -B $(srcdir)/cfg.l >cfg.c

# $(HEADERS): ;

# Ok, anything that gets linked in here, we make it depend on BITMAPS
# since that's the last file we link in.

BITMAPS: 
	$(srcdir)/makelinks $(srcdir)/..

alsa_a.c: BITMAPS


arts_a.c: BITMAPS


b_out.c: BITMAPS


bag.c: BITMAPS


bsd20_a.c: BITMAPS


celeste_e.c: BITMAPS


cfg.c: BITMAPS


cfg.l: BITMAPS


chorus_e.c: BITMAPS


common.c: BITMAPS


common.h: BITMAPS


config.h: BITMAPS


controls.c: BITMAPS


controls.h: BITMAPS


ctl.c: BITMAPS


ctl.h: BITMAPS


dec_a.c: BITMAPS


dumb_c.c: BITMAPS


effects.c: BITMAPS


effects.h: BITMAPS


esd_a.c: BITMAPS


fffload.c: BITMAPS


filter.c: BITMAPS


filter.h: BITMAPS


gentxt.c: BITMAPS


getopt.c: BITMAPS


hpux_a.c: BITMAPS


hpux_d_a.c: BITMAPS


instrum.c: BITMAPS


instrum.h: BITMAPS


linux_a.c: BITMAPS


linux_a2.c: BITMAPS


mix.c: BITMAPS


mix.h: BITMAPS


nas_a.c: BITMAPS


ncurs_c.c: BITMAPS


output.c: BITMAPS


output.h: BITMAPS


phaser_e.c: BITMAPS


playmidi.c: BITMAPS


playmidi.h: BITMAPS


raw_a.c: BITMAPS


readmidi.c: BITMAPS


readmidi.h: BITMAPS


readsbk.c: BITMAPS


resample.c: BITMAPS


resample.h: BITMAPS


resample_f.c: BITMAPS


resample_l.c: BITMAPS


reverb_e.c: BITMAPS


sbk.h: BITMAPS


sbktext.c: BITMAPS


sf2cfg.c: BITMAPS


sflayer.h: BITMAPS


slang_c.c: BITMAPS


sndfont.c: BITMAPS


sun_a.c: BITMAPS


tables.c: BITMAPS


tables.h: BITMAPS


timidity.c: BITMAPS


version.h: BITMAPS


wav2pat.c: BITMAPS


wave_a.c: BITMAPS



