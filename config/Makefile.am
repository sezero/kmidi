#	$Id$

CONFIG_FILES = gravis.cfg gsdrum.cfg midia.cfg mt32.cfg timidity.cfg wowpats.cfg chaos12-voices chaos8-voices megadrum megainst pila-voices sound-canvas-drums guitar piano piano+guitar drums $(TCLFILES)

TIMID_DIR = $(kde_datadir)/kmidi/config

EXTRA_DIST = $(CONFIG_FILES)

SUBDIRS = patch

install-data-local:
	$(mkinstalldirs) $(TIMID_DIR)
	@for i in $(CONFIG_FILES); do \
	  if [ ! -f "$(TIMID_DIR)/$$i" ] ; then  \
	    echo "$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(TIMID_DIR)"; \
	    $(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(TIMID_DIR) ; \
          fi ; \
        done
	if [ ! -f "$(TIMID_DIR)/tclIndex" -a -f "$(TIMID_DIR)/misc.tcl" ] ; then  \
	  (cd $(TIMID_DIR); sed -e s@%TCL_DIR%@$(TIMID_DIR)@g $(srcdir)/tkmidity.ptcl > tkmidity.tcl ) ; \
	  (cd $(TIMID_DIR); echo 'auto_mkindex . *.tcl; exit' | tclsh ) ; \
	fi

uninstall-local:
	for i in $(CONFIG_FILES); do \
	  rm -f $(TIMID_DIR)/$$i ; \
	done

