## Process this file with automake to produce Makefile.in

SUBDIRS = intl m4  intl m4 src glade po

appicondir = $(datadir)/pixmaps
appicon_DATA = sodipodi.png

Graphicsdir = $(datadir)/gnome/apps/Graphics
Graphics_in_files = sodipodi.desktop.in
Graphics_DATA = $(Graphics_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

oafdir = $(datadir)/oaf
oaf_in_files = GNOME_Sodipodi.oaf.in
oaf_DATA = $(oaf_in_files:.oaf.in=.oaf)
@INTLTOOL_OAF_RULE@

#if BONOBO
#install-exec-local:
#	ln -sf sodipodi $(DESTDIR)$(bindir)/sodipodi-bonobo
#endif

dist-hook:
	mkdir $(distdir)/samples
	cp $(srcdir)/samples/*svg $(distdir)/samples
	cp $(srcdir)/samples/*png $(distdir)/samples


man_MANS = sodipodi.1

EXTRA_DIST = config.rpath  config.rpath 			\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	$(Graphics_in_files)	\
	$(oaf_in_files)		\
	sodipodi.spec.in	\
	README HACKING		\
	sodipodi.png

ACLOCAL_AMFLAGS = -I m4
