## Process this file with automake to produce Makefile.in

SUBDIRS = src glade po extensions build

appicondir = $(datadir)/pixmaps
appicon_DATA = sodipodi.png

Graphicsdir = $(datadir)/applications
Graphics_in_files = sodipodi.desktop.in
Graphics_DATA = $(Graphics_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

dist-hook:
	mkdir $(distdir)/samples
	cp $(srcdir)/samples/*svg $(distdir)/samples
	cp $(srcdir)/samples/*png $(distdir)/samples


man_MANS = sodipodi.1

EXTRA_DIST = \
	$(Graphics_in_files)	\
	sodipodi.spec.in	\
	sodipodi.spec		\
	README HACKING		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	sodipodi.png		\
	sodipodi.ico

DISTCLEANFILES = \
	sodipodi.desktop	\
	intltool-extract	\
	intltool-merge		\
	intltool-update

#ACLOCAL_AMFLAGS = -I m4
