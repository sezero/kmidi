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

CLEANFILES = buildhack

buildhack:
	$(srcdir)/makelinks $(srcdir)/..
	touch buildhack

BITMAPS: buildhack

alsa_a.c: buildhack


arts_a.c: buildhack


b_out.c: buildhack


bag.c: buildhack


bsd20_a.c: buildhack


celeste_e.c: buildhack


cfg.c: buildhack


cfg.l: buildhack


chorus_e.c: buildhack


common.c: buildhack


common.h: buildhack


config.h: buildhack


controls.c: buildhack


controls.h: buildhack


ctl.c: buildhack


ctl.h: buildhack


dec_a.c: buildhack


dumb_c.c: buildhack


effects.c: buildhack


effects.h: buildhack


esd_a.c: buildhack


fffload.c: buildhack


filter.c: buildhack


filter.h: buildhack


gentxt.c: buildhack


getopt.c: buildhack


hpux_a.c: buildhack


hpux_d_a.c: buildhack


instrum.c: buildhack


instrum.h: buildhack


linux_a.c: buildhack


linux_a2.c: buildhack


mix.c: buildhack


mix.h: buildhack


nas_a.c: buildhack


ncurs_c.c: buildhack


output.c: buildhack


output.h: buildhack


phaser_e.c: buildhack


playmidi.c: buildhack


playmidi.h: buildhack


raw_a.c: buildhack


readmidi.c: buildhack


readmidi.h: buildhack


readsbk.c: buildhack


resample.c: buildhack


resample.h: buildhack


resample_f.c: buildhack


resample_l.c: buildhack


reverb_e.c: buildhack


sbk.h: buildhack


sbktext.c: buildhack


sf2cfg.c: buildhack


sflayer.h: buildhack


slang_c.c: buildhack


sndfont.c: buildhack


sun_a.c: buildhack


tables.c: buildhack


tables.h: buildhack


timidity.c: buildhack


version.h: buildhack


wav2pat.c: buildhack


wave_a.c: buildhack



