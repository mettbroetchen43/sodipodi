
gladedir = $(datadir)/sodipodi

glade_DATA = 					\
	align.glade align.c			\
	desktop.glade desktop.c			\
	document.glade document.c		\
	object_props.glade object_props.c	\
	sodipodi.glade sodipodi-glade.c		\
	text-dialog.glade text-dialog.c		\
	toolbox.glade toolbox.c			\
	transformation.glade transformation.c	\
	xml-tree.glade xml-tree.c

EXTRA_DIST = $(glade_DATA)

dist-hook:
	cp $(srcdir)/*xpm $(distdir)

