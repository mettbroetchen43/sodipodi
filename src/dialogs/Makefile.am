#
# Several object property dialogs
# Author: Lauris Kaplinski <lauris@ariman.ee>
#
# Here should be things, that use only xml interface, not objects themselves
# Currently we still have selection_changed functions, but these will be
# replaced by selection 'changed' signal handlers
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" \
	-I$(includedir) \
	$(GNOME_INCLUDEDIR)

noinst_LIBRARIES = libspdialogs.a

libspdialogs_a_SOURCES = \
	object-fill.c object-fill.h \
	object-stroke.c object-stroke.h \
	text-edit.c text-edit.h \
	export.c export.h \
	xml-tree.c xml-tree.h \
	align.c align.h	\
	transformation.c transformation.h \
	desktop-properties.c desktop-properties.h

