#
# Widgets for sodipodi
# Author: Lauris Kaplinski <lauris@ximian.com>
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-I.. \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" 		\
	$(SODIPODI_CFLAGS) \
	-I$(includedir)

noinst_LIBRARIES = libspwidgets.a

libspwidgets_a_SOURCES = \
	sp-toolbox.c sp-toolbox.h \
	sp-menu-button.c sp-menu-button.h \
	sp-color-preview.c sp-color-preview.h \
	sp-color-slider.c sp-color-slider.h \
	sp-color-selector.c sp-color-selector.h \
	dash-selector.c dash-selector.h \
	gradient-image.c gradient-image.h \
	gradient-position.c gradient-position.h \
	paint-selector.c paint-selector.h \
	gradient-selector.c gradient-selector.h \
	gradient-vector.c gradient-vector.h \
	font-selector.c font-selector.h \
	sp-xmlview-tree.c sp-xmlview-tree.h \
	sp-xmlview-attr-list.c sp-xmlview-attr-list.h \
	sp-xmlview-content.c sp-xmlview-content.h \
	sp-widget.c sp-widget.h

#	sp-value-winder.c sp-value-winder.h
#	sp-spin-button.c sp-spin-button.h

#noinst_PROGRAMS = test-widgets

#test_widgets_SOURCES = \
#	test-widgets.c \
#	sp-value-winder.c sp-value-winder.h \
#	sp-color-slider.c sp-color-slider.h \
#	sp-color-selector.c sp-color-selector.h

#test_widgets_LDADD = $(SODIPODI_LIBS)
