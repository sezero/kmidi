#
#	$Id$
#
#                           KMidi Makefile
#
#  Copyright 1997 Bernd Johannes Wuebben, wuebben@math.cornell.edu
#

VERSION = 0.2i

INCLUDES = $(X_INCLUDES)
AM_CFLAGS = -DDEFAULT_PATH=\"$(TIMID_DIR)\" $(EXTRATDEFS) $(EXTRATCFLAGS)
if GXXOPTIMIZE
CFLAGS =
AM_CPPFLAGSS = -O3 -fomit-frame-pointer -ffast-math -malign-double
endif
LDADD = $(LIBALIB) $(LIBAUDIO) $(LIBMME) $(EXTRATLIBS) $(X_LIBS) -lm

# Where to install the patches, config files, and MIDI files.
# If you change this, it's a good idea to recompile the binary,
# or you'll need to invoke timidity with the -L option.
TIMID_DIR = $(kde_datadir)/kmidi/config

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
	hpux_d_a.c alsa_a.c \
	bsd20_a.c esd_a.c nas_a.c \
	ncurs_c.c slang_c.c tk_c.c gtk_i.c ctl.c \
	motif_c.c motif_i.c motif_p.c xaw_i.c xaw_c.c

noinst_HEADERS = config.h common.h readmidi.h playmidi.h resample.h mix.h instrum.h \
        tables.h controls.h output.h filter.h motif.h xaw.h ctl.h

cfg.c: cfg.l
	rm -f cfg.c
	flex -t -B $(srcdir)/cfg.l >cfg.c

