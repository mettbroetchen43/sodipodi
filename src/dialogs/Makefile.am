#
# Several object property dialogs
# Author: Lauris Kaplinski <lauris@kaplinski.com>
#
# Here should be things, that use only xml interface, not objects themselves
# Currently we still have selection_changed functions, but these will be
# replaced by selection 'changed' signal handlers
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-I.. \
	-I$(top_srcdir)/src \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" \
	-DSODIPODI_PIXMAPDIR=\""$(gladedir)"\" \
	$(SODIPODI_CFLAGS)

noinst_LIBRARIES = libspdialogs.a

libspdialogs_a_SOURCES = \
	sp-attribute-widget.c sp-attribute-widget.h \
	text-edit.c text-edit.h \
	export.c export.h \
	align.c align.h	\
	display-settings.c display-settings.h \
	desktop-properties.c desktop-properties.h \
	document-properties.c document-properties.h \
	object-properties.c object-properties.h \
	item-properties.c item-properties.h \
	object-attributes.c object-attributes.h \
	tool-options.c tool-options.h \
	tool-attributes.c tool-attributes.h \
	fill-style.c fill-style.h \
	stroke-style.c stroke-style.h \
	transformation.c transformation.h \
	node-edit.c node-edit.h \
	xml-tree.c xml-tree.h \
	document-tree.c document-tree.h

moduledir = $(libdir)/sodipodi/modules

module_LTLIBRARIES = libxmleditor.la

libxmleditor_la_CPPFLAGS = \
	-DSTANDALONE_MODULE

libxmleditor_la_SOURCES = \
	xml-editor.c

libxmleditor_la_LDFLAGS = \
	-module \
	-avoid-version


