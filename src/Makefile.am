## Process this file with automake to produce Makefile.in

SUBDIRS = xml svg helper dialogs display

gladedir = $(PACKAGE_GLADE_DIR)

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" \
	-I$(srcdir/xml) \
	-I$(srcdir/svg) \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = sodipodi

sodipodi_SOURCES = \
	sodipodi.h \
	\
	sp-item.c sp-item.h \
	sp-item-repr.c \
	sp-item-group.c \
	sp-root.c sp-root.h \
	sp-image.c sp-image.h \
	sp-path-component.c sp-path-component.h \
	sp-path.c sp-path.h \
	sp-shape-style.c sp-shape-style.h \
	sp-shape.c sp-shape.h \
	sp-rect.c sp-rect.h \
	sp-ellipse.c sp-ellipse.h \
	sp-chars.c sp-chars.h \
	sp-text.c sp-text.h \
	\
	repr-item.c repr-item.h \
	\
	document.c document.h \
	mdi.c mdi.h \
	mdi-child.c mdi-child.h \
	mdi-desktop.c mdi-desktop.h \
	mdi-document.c mdi-document.h \
	mdi-ui.c mdi-ui.h \
	selection.c selection.h \
	desktop.c desktop.h \
	desktop-units.h \
	desktop-handles.c desktop-handles.h \
	desktop-affine.c desktop-affine.h \
	\
	main.c \
	help.c help.h \
	sp-cursor.c sp-cursor.h \
	dir-util.c dir-util.h \
	rubberband.c rubberband.h \
	event-broker.c event-broker.h \
	path-chemistry.c path-chemistry.h \
	selection-chemistry.c selection-chemistry.h \
	\
	seltrans-handles.c seltrans-handles.h \
	seltrans.c seltrans.h \
	sp-select-context.c sp-select-context.h \
	nodepath.c nodepath.h \
	sp-node-path-edit.c sp-node-path-edit.h \
	sp-node-context.c sp-node-context.h \
	object-context.c object-context.h \
	sp-draw-context.c sp-draw-context.h \
	text-context.c text-context.h \
	zoom-context.c zoom-context.h \
	font-wrapper.c font-wrapper.h \
	file.c file.h

sodipodi_LDFLAGS = -export-dynamic

sodipodi_libs = \
	xml/libspxml.a \
	helper/libspgt1.a \
	dialogs/libspdialogs.a \
	svg/libspsvg.a \
	display/libspdisplay.a \
	helper/libspchelp.a

sodipodi_LDADD = \
	$(sodipodi_libs) \
	$(GNOME_LIBDIR) \
	-lglade-gnome \
	-lglade \
	-lgdk_pixbuf \
	$(GNOMEUI_LIBS) \
	$(GNOME_PRINT_LIBS) \
	$(INTLLIBS)
