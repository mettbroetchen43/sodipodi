## Process this file with automake to produce Makefile.in

if BONOBO
bonobo_subdirs = bonobo
bonobo_libs = bonobo/libspbonobo.a
endif

SUBDIRS = xml svg helper dialogs display $(bonobo_subdirs)

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" 		\
	-DSODIPODI_PIXMAPDIR=\""$(pixmapdir)"\" 	\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-I$(srcdir/xml) 				\
	-I$(srcdir/svg) 				\
	-I$(top_srcdir)/intl 				\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = sodipodi

sodipodi_SOURCES = \
	sodipodi.c sodipodi.h sodipodi-private.h \
	\
	sp-object.c sp-object.h \
	sp-object-repr.c sp-object-repr.h \
	sp-object-group.c sp-object-group.h \
	sp-defs.c sp-defs.h \
	sp-use.c sp-use.h \
	sp-item.c sp-item.h \
	sp-item-group.c \
	sp-root.c sp-root.h \
	sp-namedview.c sp-namedview.h \
	sp-guide.c sp-guide.h \
	sp-image.c sp-image.h \
	sp-path-component.c sp-path-component.h \
	sp-path.c sp-path.h \
	sp-shape-style.c sp-shape-style.h \
	sp-shape.c sp-shape.h \
	sp-rect.c sp-rect.h \
	sp-ellipse.c sp-ellipse.h \
	sp-line.c sp-line.h \
	sp-polyline.c sp-polyline.h \
	sp-polygon.c sp-polygon.h \
	sp-chars.c sp-chars.h \
	sp-text.c sp-text.h \
	sp-gradient.c sp-gradient.h \
	\
	document.c document.h document-private.h \
	document-undo.c \
	selection.c selection.h \
	desktop.c desktop.h \
	desktop-units.h \
	desktop-events.c desktop-events.h \
	desktop-handles.c desktop-handles.h \
	desktop-affine.c desktop-affine.h \
	desktop-snap.c desktop-snap.h \
	sp-metrics.c sp-metrics.h \
	toolbox.c toolbox.h \
	\
	event-context.c event-context.h \
	seltrans-handles.c seltrans-handles.h \
	seltrans.c seltrans.h \
	select-context.c select-context.h \
	node-context.c node-context.h \
	nodepath.c nodepath.h \
	rect-context.c rect-context.h \
	ellipse-context.c ellipse-context.h \
	draw-context.c draw-context.h \
	text-context.c text-context.h \
	\
	knot.c knot.h \
	\
	main.c \
	help.c help.h \
	sp-cursor.c sp-cursor.h \
	dir-util.c dir-util.h \
	rubberband.c rubberband.h \
	event-broker.c event-broker.h \
	path-chemistry.c path-chemistry.h \
	selection-chemistry.c selection-chemistry.h \
	sp-item-transform.c sp-item-transform.h \
	\
	sp-node-path-edit.c sp-node-path-edit.h \
	zoom-context.c zoom-context.h \
	file.c file.h \
	interface.c interface.h

#	font-wrapper.c font-wrapper.h \

sodipodi_LDFLAGS = -export-dynamic

sodipodi_libs = \
	xml/libspxml.a \
	dialogs/libspdialogs.a \
	svg/libspsvg.a \
	display/libspdisplay.a \
	helper/libspchelp.a


sodipodi_LDADD = \
	$(sodipodi_libs) \
	$(bonobo_libs) \
	$(GNOME_LIBDIR) \
	-lpng \
	-lglade-gnome \
	-lglade \
	-lgdk_pixbuf \
	-lunicode \
	$(GNOMEUI_LIBS) \
	$(GNOME_PRINT_LIBS) \
	$(EXTRA_BONOBO_GNOME_LIBS) \
	$(INTLLIBS)

EXTRA_DIST = \
	brokenimage.xpm

dist-hook:
	mkdir $(distdir)/pixmaps
	cp $(srcdir)/pixmaps/*xpm $(distdir)/pixmaps
