noinst_LIBRARIES = libnrtype.a

# if USE_GNOME_PRINT
# gnome_print_sources = nr-type-gnome.c nr-type-gnome.h
# endif

if USE_XFT
xft_sources = nr-type-xft.c nr-type-xft.h
endif

if PLATFORM_WIN32
win32_source = nr-type-w32.c nr-type-w32.h \
	       cp1250.c  cp1252.c  cp1254.c  cp1256.c \
	       cp1258.c  cp932.c   cp949.c   cp1251.c \
               cp1253.c  cp1255.c  cp1257.c  cp874.c  \
               cp936.c  cp950.c
endif

INCLUDES = \
	-I.. \
	-I$(top_srcdir)/src \
	$(FREETYPE_CFLAGS) \
	$(SODIPODI_CFLAGS) \
	$(XFT_CFLAGS) \
	$(LIBNR_CFLAGS)

#	$(GNOME_PRINT_CFLAGS)

libnrtype_a_SOURCES = \
	nr-type-primitives.c nr-type-primitives.h \
	nr-typeface.c nr-typeface.h \
	nr-font.c nr-font.h \
	nr-rasterfont.c nr-rasterfont.h \
	nr-type-directory.c nr-type-directory.h \
	nr-glyphs.c nr-glyphs.h \
	nr-type-ft2.c nr-type-ft2.h \
	${win32_source} \
	$(xft_sources)

#	$(gnome_print_sources)

EXTRA_DIST = \
	nr-type-xft.c nr-type-xft.h \
	nr-type-w32.c nr-type-w32.h \
	codepages.h

#	nr-type-gnome.c nr-type-gnome.h
