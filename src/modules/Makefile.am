
extdir = $(libdir)/sodipodi/extensions
extdatadir = $(datadir)/sodipodi/extensions

INCLUDES = \
	-I.. \
	-DSODIPODI_EXTENSIONDIR=\""$(extdir)"\" \
	-DSODIPODI_EXTENSIONDATADIR=\""$(extdatadir)"\" \
	$(SODIPODI_CFLAGS)

noinst_LIBRARIES = libspmodules.a

libspmodules_a_SOURCES = \
		sp-module.c \
		sp-module-doc.c \
		sp-module-doc.h \
		sp-module-exec.c \
		sp-module-exec-ext.c \
		sp-module-exec-ext.h \
		sp-module-exec.h \
		sp-module.h \
		sp-module-sys.c \
		sp-module-sys.h

