## Process this file with automake to produce Makefile.in

SUBDIRS = intl po macros src

EXTRA_DIST = \
	sodipodi.desktop \
	sodipodi.spec.in \
	sodipodi.gnorba \
	README HACKING \
	sodipodi.png \
	samples/*

appicondir = $(datadir)/pixmaps
appicon_DATA = sodipodi.png

sysdir = $(datadir)/gnome/apps/Graphics
sys_DATA = sodipodi.desktop

gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = sodipodi.gnorba

if BONOBO
install-exec-local:
	if test -e $(bindir)/sodipodi-bonobo; then \
		rm -f $(bindir)/sodipodi-bonobo; \
	fi ; \
	ln -s $(bindir)/sodipodi $(bindir)/sodipodi-bonobo
endif

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

