#
# Widgets for sodipodi
# Author: Lauris Kaplinski <lauris@ximian.com>
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" 		\
	$(SODIPODI_CFLAGS) \
	-I$(includedir)

noinst_LIBRARIES = libspwidgets.a

libspwidgets_a_SOURCES = \
	sp-toolbox.c sp-toolbox.h \
	sp-color-preview.c sp-color-preview.h \
	sp-color-slider.c sp-color-slider.h \
	sp-color-selector.c sp-color-selector.h \
	gradient-image.c gradient-image.h \
	gradient-position.c gradient-position.h

#	sp-value-winder.c sp-value-winder.h
#	sp-spin-button.c sp-spin-button.h

#noinst_PROGRAMS = test-widgets

#test_widgets_SOURCES = \
#	test-widgets.c \
#	sp-value-winder.c sp-value-winder.h \
#	sp-color-slider.c sp-color-slider.h \
#	sp-color-selector.c sp-color-selector.h

#test_widgets_LDADD = $(SODIPODI_LIBS)
