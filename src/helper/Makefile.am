#
# Miscellaneous unsystematized and temporary helper utilities
#
# libspchelp - canvas utilities, specific canvas items, libart add-ons
#

if USE_KDE
kdeincludes = $(all_includes)
kdesources = kde-private.cpp kde-private.h kde.cpp kde.h
kde-private.cpp: kde-private.h
	$(MOC) kde-private.h -o kde-private.cpp
endif

noinst_LIBRARIES = libspchelp.a

INCLUDES = \
	-I.. \
	$(cflags) \
	$(SODIPODI_CFLAGS) \
	$(kdeincludes)

libspchelp_a_SOURCES = \
	sp-intl.h \
	sp-marshal.c sp-marshal.h \
	helper-forward.h \
	sp-canvas.c sp-canvas.h \
	sp-canvas-util.c sp-canvas-util.h \
	gnome-utils.c gnome-utils.h \
	curve.c curve.h \
	gnome-canvas-acetate.c gnome-canvas-acetate.h \
	sodipodi-ctrl.c sodipodi-ctrl.h \
	sodipodi-ctrlrect.c sodipodi-ctrlrect.h \
	sp-ctrlline.c sp-ctrlline.h \
	guideline.c guideline.h \
	canvas-grid.c canvas-grid.h \
	canvas-bpath.c canvas-bpath.h \
	png-write.c png-write.h \
	nr-plain-stuff.c nr-plain-stuff.h \
	nr-plain-stuff-gdk.c nr-plain-stuff-gdk.h \
	bezier-utils.c bezier-utils.h \
	units.c units.h \
	unit-menu.c unit-menu.h \
	$(kdesources)

#	pathops.c pathops.h

sp-marshal.h: sp-marshal.list
	rm -f $@
	glib-genmarshal --prefix=sp_marshal --header sp-marshal.list > $@

sp-marshal.c: sp-marshal.h sp-marshal.list
	rm -f $@
	glib-genmarshal --prefix=sp_marshal --body sp-marshal.list > $@

EXTRA_DIST = sp-marshal.list $(kdesources)

