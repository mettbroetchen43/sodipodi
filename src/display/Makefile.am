#
# Sodipodi GnomeCanvas objects
# Author: Lauris Kaplinski <lauris@ariman.ee>
#
# Here are major objects, used for displaying things
#

if WITH_NEW_RENDER
nr_sources = \
	nr-primitives.c nr-primitives.h \
	nr-uta.c nr-uta.h \
	nr-canvas.c nr-canvas.h \
	nr-canvas-item.c nr-canvas-item.h \
	nr-canvas-group.c nr-canvas-group.h \
	nr-aa-canvas.c nr-aa-canvas.h \
	sp-arena.c sp-arena.h \
	sp-canvas-item.c sp-canvas-item.h \
	sp-canvas-group.c sp-canvas-group.h \
	nr-svp.c nr-svp.h \
	nr-svp-uncross.c nr-svp-uncross.h \
	nr-svp-render.c nr-svp-render.h

noinst_PROGRAMS = testnr
testnr_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)
testnr_SOURCES = \
	testnr.c \
	$(nr_sources)
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

