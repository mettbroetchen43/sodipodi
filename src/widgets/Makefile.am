#
# Widgets for sodipodi
# Author: Lauris Kaplinski <lauris@helixcode.com>
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-DSODIPODI_GLADEDIR=\""$(gladedir)"\" 		\
	$(SODIPODI_CFLAGS) \
	$(GNOME_INCLUDEDIR) \
	-I$(includedir)

noinst_LIBRARIES = libspwidgets.a

libspwidgets_a_SOURCES = \
	sp-toolbox.c sp-toolbox.h

