#	$Id$

CONFIG_FILES = arrow.xbm back.xbm check.xbm fwd.xbm fwrd.xbm next.xbm \
	off.xbm on.xbm pause.xbm play.xbm prev.xbm quit.xbm restart.xbm \
	stop.xbm tback.xbm timidity.xbm tnext.xbm tpause.xbm tprev.xbm tquit.xbm

TIMID_DIR = $(kde_datadir)/kmidi/config

EXTRA_DIST = $(CONFIG_FILES)

install-data-local:
	$(mkinstalldirs) $(TIMID_DIR)
	$(mkinstalldirs) $(TIMID_DIR)/BITMAPS
	@for i in $(CONFIG_FILES); do \
	  if [ ! -f "$(TIMID_DIR)/BITMAPS/$$i" ] ; then  \
	    echo "$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(TIMID_DIR)/BITMAPS"; \
	    $(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(TIMID_DIR)/BITMAPS ; \
          fi ; \
        done

uninstall-local:
	for i in $(CONFIG_FILES); do \
	  rm -f $(TIMID_DIR)/BITMAPS/$$i ; \
	done

