## Process this file with automake to produce Makefile.in

SUBDIRS = intl po macros src glade

EXTRA_DIST =			\
	sodipodi.desktop	\
	sodipodi.spec.in	\
	$(oaf_DATA)		\
	README HACKING		\
	sodipodi.png


appicondir = $(datadir)/pixmaps
appicon_DATA = sodipodi.png

sysdir = $(datadir)/gnome/apps/Graphics
sys_DATA = sodipodi.desktop

oafdir = $(datadir)/oaf
oaf_DATA = GNOME_Sodipodi.oafinfo

if BONOBO
install-exec-local:
	if test -r $(bindir)/sodipodi-bonobo; then \
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
	mkdir $(distdir)/samples
	cp $(srcdir)/samples/*svg $(distdir)/samples
	cp $(srcdir)/samples/*png $(distdir)/samples




