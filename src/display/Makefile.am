#
# Sodipodi GnomeCanvas objects
# Author: Lauris Kaplinski <lauris@ariman.ee>
#
# Here are major objects, used for displaying things
#

if WITH_NEW_RENDER
nr_sources = \
	nr-canvas.c nr-canvas.h \
	nr-svp.c nr-svp.h \
	nr-svp-uncross.c nr-svp-uncross.h \
	nr-svp-render.c nr-svp-render.h
else
nr_sources =
endif

INCLUDES = \
	-I$(includedir) \
	$(GNOME_INCLUDEDIR)

noinst_LIBRARIES = libspdisplay.a

libspdisplay_a_SOURCES = \
	canvas-bgroup.c canvas-bgroup.h \
	canvas-image.c canvas-image.h \
	cpath-component.c cpath-component.h \
	path-archetype.c path-archetype.h \
	canvas-shape.c canvas-shape.h \
	fill.c fill.h \
	stroke.c stroke.h \
	$(nr_sources)

