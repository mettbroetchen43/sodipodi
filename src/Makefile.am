## Process this file with automake to produce Makefile.in

if BONOBO
bonobo_subdirs = bonobo
bonobo_libs = bonobo/libspbonobo.a
endif

SUBDIRS = xml svg widgets helper dialogs display $(bonobo_subdirs)

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" 		\
	-DSODIPODI_PIXMAPDIR=\""$(pixmapdir)"\" 	\
	-DSODIPODI_VERSION=\""$(VERSION)"\" \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-I$(srcdir/xml) 				\
	-I$(srcdir/svg) 				\
	$(SODIPODI_CFLAGS)				\
	$(SODIPODI_BONOBO_CFLAGS)

bin_PROGRAMS = sodipodi

sodipodi_SOURCES = \
	sodipodi.c sodipodi.h sodipodi-private.h \
	\
	forward.h \
	sp-object.c sp-object.h \
	sp-object-repr.c sp-object-repr.h \
	sp-object-group.c sp-object-group.h \
	sp-defs.c sp-defs.h \
	sp-use.c sp-use.h \
	sp-item.c sp-item.h \
	sp-item-group.c sp-item-group.h \
	sp-anchor.c sp-anchor.h \
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
	sp-star.c sp-star.h \
	sp-spiral.c sp-spiral.h \
	sp-line.c sp-line.h \
	sp-polyline.c sp-polyline.h \
	sp-polygon.c sp-polygon.h \
	sp-chars.c sp-chars.h \
	sp-text.c sp-text.h \
	sp-paint-server.c sp-paint-server.h \
	sp-gradient.c sp-gradient.h \
	sp-clippath.c sp-clippath.h \
	\
	color.c color.h \
	style.c style.h \
	\
	document.c document.h document-private.h \
	document-undo.c \
	uri-references.c uri-references.h \
	view.c view.h \
	svg-view.c svg-view.h \
	selection.c selection.h \
	desktop.c desktop.h \
	desktop-units.h \
	desktop-events.c desktop-events.h \
	desktop-handles.c desktop-handles.h \
	desktop-affine.c desktop-affine.h \
	desktop-snap.c desktop-snap.h \
	sp-metrics.c sp-metrics.h \
	sp-ruler.c sp-ruler.h \
	toolbox.c toolbox.h \
	\
	event-context.c event-context.h \
	seltrans-handles.c seltrans-handles.h \
	seltrans.c seltrans.h \
	select-context.c select-context.h \
	node-context.c node-context.h \
	nodepath.c nodepath.h \
	knotholder.c knotholder.h \
	rect-context.c rect-context.h \
	ellipse-context.c ellipse-context.h \
	star-context.c star-context.h \
	spiral-context.c spiral-context.h \
	draw-context.c draw-context.h \
	dyna-draw-context.c dyna-draw-context.h \
	text-context.c text-context.h \
	slide-context.c slide-context.h \
	\
	knot.c knot.h \
	\
	main.c \
	help.c help.h \
	sp-cursor.c sp-cursor.h \
	marshal.c marshal.h \
	dir-util.c dir-util.h \
	rubberband.c rubberband.h \
	event-broker.c event-broker.h \
	path-chemistry.c path-chemistry.h \
	selection-chemistry.c selection-chemistry.h \
	gradient-chemistry.c gradient-chemistry.h \
	sp-item-transform.c sp-item-transform.h \
	\
	sp-node-path-edit.c sp-node-path-edit.h \
	zoom-context.c zoom-context.h \
	file.c file.h \
	interface.c interface.h \
	\
	preferences-skeleton.h

#	font-wrapper.c font-wrapper.h \
#

sodipodi_private_libs = \
	dialogs/libspdialogs.a \
	xml/libspxml.a \
	svg/libspsvg.a \
	widgets/libspwidgets.a \
	display/libspdisplay.a \
	helper/libspchelp.a


sodipodi_LDADD = \
	$(sodipodi_private_libs) \
	$(bonobo_libs) \
	$(SODIPODI_LIBS) \
	$(SODIPODI_BONOBO_LIBS)

#	$(GNOME_LIBDIR) \
#	-lpng \
#	-lglade-gnome \
#	-lglade \
#	-lgdk_pixbuf \
#	$(GNOMEUI_LIBS) \
#	$(GNOME_PRINT_LIBS) \
#	$(EXTRA_BONOBO_GNOME_LIBS) \
#	$(INTLLIBS)

EXTRA_DIST = \
	brokenimage.xpm

dist-hook:
	mkdir $(distdir)/pixmaps
	cp $(srcdir)/pixmaps/*xpm $(distdir)/pixmaps
