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
	ln -sf sodipodi $(DESTDIR)$(bindir)/sodipodi-bonobo
endif

dist-hook:
	mkdir $(distdir)/samples
	cp $(srcdir)/samples/*svg $(distdir)/samples
	cp $(srcdir)/samples/*png $(distdir)/samples




