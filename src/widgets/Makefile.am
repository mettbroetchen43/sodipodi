#
# Widgets for sodipodi
# Author: Lauris Kaplinski <lauris@helixcode.com>
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" 		\
	$(SODIPODI_CFLAGS) \
	-I$(includedir)

noinst_LIBRARIES = libspwidgets.a

libspwidgets_a_SOURCES = \
	sp-toolbox.c sp-toolbox.h \
	sp-color-slider.c sp-color-slider.h \
	sp-color-selector.c sp-color-selector.h \
	gradient-image.c gradient-image.h \
	gradient-position.c gradient-position.h

noinst_PROGRAMS = test-widgets

test_widgets_SOURCES = \
	test-widgets.c \
	sp-color-slider.c sp-color-slider.h \
	sp-color-selector.c sp-color-selector.h

test_widgets_LDADD = $(SODIPODI_LIBS)
