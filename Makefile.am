## Process this file with automake to produce Makefile.in

SUBDIRS = intl po macros src

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/glade; then \
	  $(mkinstalldirs) $(datadir)/sodipodi; \
	  for uifile in $(srcdir)/glade/*; do \
	    if test -f $$uifile; then \
	      $(INSTALL_DATA) $$uifile $(datadir)/sodipodi; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d glade; then \
	  mkdir $(distdir)/glade; \
	  for uifile in glade/*; do \
	    if test -f $$uifile; then \
	      cp -p $$uifile $(distdir)/glade; \
	    fi \
	  done \
	fi

