noinst_LIBRARIES = libmlview.a

INCLUDES = \
	-I$(top_srcdir) \
	-DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-DDATADIR=\"$(datadir)\" \
	$(MLVIEW_CFLAGS)

libmlview_a_SOURCES = \
	mlview-utils.c \
	mlview-utils.h \
	mlview-app-context.c  \
	mlview-app-context.h  \
	mlview-action.h \
	mlview-iview.c \
	mlview-iview.h \
	mlview-view-adapter.c \
	mlview-view-adapter.h \
	mlview-tree-view.c \
	mlview-tree-view.h \
	mlview-editor.c \
	mlview-editor.h \
	mlview-app.c  \
	mlview-app.h  \
	mlview-node-editor.c \
	mlview-node-editor.h \
	mlview-node-type-picker.c \
	mlview-node-type-picker.h \
	mlview-xml-document.c \
	mlview-xml-document.h \
	mlview-file-descriptor.c \
	mlview-file-descriptor.h \
	mlview-parsing-utils.c \
	mlview-parsing-utils.h \
	mlview-file-selection.c \
	mlview-file-selection.h \
	mlview-attribute-picker.c \
	mlview-attribute-picker.h \
	mlview-marshal.c \
	mlview-marshal.h \
	mlview-tree-editor2.c \
	mlview-tree-editor2.h \
	mlview-attrs-editor.c \
	mlview-attrs-editor.h \
	mlview-preferences.c \
	mlview-preferences.h \
	mlview-ns-editor.c \
	mlview-ns-editor.h \
	mlview-icon-tree.c \
	mlview-icon-tree.h

#bin_PROGRAMS = mlv
#
#mlv_SOURCES  =  main.c 
#
#LDADD=
#mlv_LDADD = \
#	libmlview.a \
#	$(LDADD)
#
#mlv_LDFLAGS = \
#    $(MLVIEW_LDFLAGS)
#
#marshal:
#	glib-genmarshal --header --stdinc --prefix=mlview_marshal mlview-marshal.list > mlview-marshal.h
#	glib-genmarshal --body   --stdinc --prefix=mlview_marshal mlview-marshal.list > mlview-marshal.c
