#
#	$Id$
#
#                           KMidi Makefile
#
#  Copyright 1997 Bernd Johannes Wuebben, wuebben@math.cornell.edu
#

VERSION = 0.2i

INCLUDES = $(all_includes)
AM_CFLAGS = -DDEFAULT_PATH=\"$(TIMID_DIR)\" -DKMIDI
CFLAGS = -O -Wall -W -Wpointer-arith -Wmissing-prototypes -Wwrite-strings -Wconversion
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KDEUI) $(LIBALIB) $(LIBAUDIO) $(LIBMME) $(LIB_KFILE)

# Where to install the patches, config files, and MIDI files.
# If you change this, it's a good idea to recompile the binary,
# or you'll need to invoke timidity with the -L option.
TIMID_DIR = $(kde_datadir)/kmidi/config


bin_PROGRAMS = kmidi sf2cfg

kmidi_SOURCES = kmidi.cpp  playlist.cpp bwlednum.cpp configdlg.cpp log.cpp \
	docking.cpp \
	timidity.c common.c readmidi.c playmidi.c resample.c mix.c instrum.c \
	tables.c controls.c output.c filter.c wave_a.c raw_a.c dumb_c.c \
	sndfont.c readsbk.c linux_a.c ctl.c dec_a.c sun_a.c hpux_a.c \
	b_out.c fffload.c \
	effects.c reverb_e.c chorus_e.c phaser_e.c celeste_e.c \
	resample_l.c resample_f.c

sf2cfg_SOURCES = sf2cfg.c readsbk.c

noinst_HEADERS = kmidi.h  playlist.h bwlednum.h configdlg.h \
	docking.h \
	config.h common.h readmidi.h playmidi.h resample.h mix.h instrum.h \
	tables.h controls.h output.h filter.h log.h version.h filepic.h \
	cduppic.h constants.h folderpic.h sbk.h sflayer.h ctl.h effects.h

METASOURCES = kmidi.moc  playlist.moc bwlednum.moc configdlg.moc log.moc docking.moc

SUBDIRS = config bitmaps patch midis

appsdir = $(kde_appsdir)/Multimedia
apps_DATA = kmidi.desktop

KDE_ICON = kmidi

picsdir = $(kde_datadir)/kmidi/pics
pics_DATA = kmidilogo.png mini-kmidi.png

EXTRA_DIST = $(apps_DATA) $(pics_DATA) 


install-data-local:
	$(mkinstalldirs) $(TIMID_DIR)/patch
	if [ ! -f $(TIMID_DIR)/patch/sologrnd.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/sologrnd.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/nylongt2.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/nylongt2.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/kick2.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/kick2.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/kick1.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/kick1.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/hihatcl.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/hihatcl.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/cymride1.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/cymride1.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi
	if [ ! -f $(TIMID_DIR)/patch/acbass.pat ]; then $(INSTALL_DATA) $(srcdir)/patch/acbass.pat $(DESTDIR)$(TIMID_DIR)/patch ; fi

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

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kmidi_SOURCES) && mv messages.po $(podir)/kmidi.pot

