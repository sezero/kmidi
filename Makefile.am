#
#	$Id$
#
#                           KMidi Makefile
#
#  Copyright 1997 Bernd Johannes Wuebben, wuebben@math.cornell.edu
#

VERSION = 0.2i
KDE_OPTIONS = nofinal

INCLUDES = `$(ARTSCCONFIG) --cflags` $(all_includes)
AM_CPPFLAGS = -DDEFAULT_PATH=\"$(TIMID_DIR)\" -DKMIDI $(EXTRADEFS) $(CPPFLAGS)
if GXXOPTIMIZE
KDE_CXXFLAGS = -O3 -fomit-frame-pointer -ffast-math
endif
LDFLAGS = $(all_libraries) `$(ARTSCCONFIG) --libs` $(KDE_RPATH)
#LDADD = $(LIB_KFILE) $(LIBALIB) $(LIBAUDIO) $(LIBMME) $(EXTRALIBS) -lsoundserver_idl -lartsflow
LDADD = $(LIB_KFILE) $(LIBALIB) $(LIBAUDIO) $(LIBMME) $(EXTRALIBS)

# Where to install the patches, config files, and MIDI files.
# If you change this, it's a good idea to recompile the binary,
# or you'll need to invoke timidity with the -L option.
TIMID_DIR = $(kde_datadir)/kmidi/config


bin_PROGRAMS = kmidi sf2cfg


check_PROGRAMS = dcopplay

dcopplay_SOURCES = dcopplay.cpp
dcopplay_LDADD = $(LIB_KDECORE)

kmidi_SOURCES = kmidi.cpp  playlist.cpp bwlednum.cpp configdlg.cpp log.cpp \
	docking.cpp table.cpp \
	timidity.cpp common.cpp readmidi.cpp playmidi.cpp resample.cpp mix.cpp instrum.cpp \
	tables.cpp controls.cpp output.cpp filter.cpp wave_a.cpp raw_a.cpp dumb_c.cpp \
	sndfont.cpp readsbk.cpp ncurs_c.cpp slang_c.cpp ctl.cpp cfg.cpp \
	linux_a.cpp linux_a2.cpp dec_a.cpp sun_a.cpp hpux_a.cpp hpux_d_a.cpp alsa_a.cpp \
	bsd20_a.cpp esd_a.cpp nas_a.cpp arts_a.cpp \
	b_out.cpp fffload.cpp \
	effects.cpp reverb_e.cpp chorus_e.cpp phaser_e.cpp celeste_e.cpp \
	resample_l.cpp resample_f.cpp midiapplication.cpp kmidiframe.cpp \
	kmidifiledlg.cpp

sf2cfg_SOURCES = sf2cfg.cpp readsbk.cpp
sf2cfg_LDADD =

noinst_HEADERS = kmidi.h  playlist.h bwlednum.h configdlg.h \
	docking.h table.h \
	config.h common.h readmidi.h playmidi.h resample.h mix.h instrum.h \
	tables.h controls.h output.h filter.h log.h version.h filepic.h \
	cduppic.h folderpic.h sbk.h sflayer.h ctl.h effects.h \
	midiapplication.h kmidiframe.h kmidifiledlg.h

METASOURCES = AUTO

SUBDIRS = config bitmaps midis TIMIDITY BITMAPS

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmidi.desktop

KDE_ICON = kmidi

picsdir = $(kde_datadir)/kmidi/pics
pics_DATA = kmidilogo.png 

EXTRA_DIST = $(apps_DATA) $(pics_DATA)


cfg.cpp: cfg.l
	rm -f cfg.cpp
	flex -t -B $(srcdir)/cfg.l >cfg.cpp

install-data-local:
	$(mkinstalldirs) $(TIMID_DIR)/patch
	if [ ! -f $(TIMID_DIR)/patch/sologrnd.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/sologrnd.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/nylongt2.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/nylongt2.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/kick2.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/kick2.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/kick1.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/kick1.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/hihatcl.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/hihatcl.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/cymride1.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/cymride1.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/acbass.pat ]; then $(INSTALL_DATA) $(srcdir)/config/patch/acbass.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi

	@echo ""
	@echo "Midis:"
	@echo "I have included a couple of nice midis in the kmidi/midis directory."
	@echo "These are installed in KDEDIR/share/apps/kmidi."
	@echo ""
	@echo "Sound Patches:"
	@echo "Only a simple  piano and guitar sound patch and 3 drum patches come with "
	@echo "this distribution.  For normal playing of midis, you'll need a full patch"
	@echo "set -- either a sf2 SoundFont (preferably) or a set of GUS .pat files.  For"
	@echo "more info look at kmidi's configure window, or read the KMidi Manual or the"
	@echo "README file."
	@echo ""
	@echo "Have fun with kmidi -- Bernd Wuebben <wuebben@kde.org>, Greg Lee <lee@hawaii.edu>"



uninstall-local:
	rm -f $(TIMID_DIR)/patch/acbass.pat
	rm -f $(TIMID_DIR)/patch/cymride1.pat
	rm -f $(TIMID_DIR)/patch/hihatcl.pat
	rm -f $(TIMID_DIR)/patch/kick1.pat
	rm -f $(TIMID_DIR)/patch/kick2.pat
	rm -f $(TIMID_DIR)/patch/nylongt2.pat
	rm -f $(TIMID_DIR)/patch/sologrnd.pat

