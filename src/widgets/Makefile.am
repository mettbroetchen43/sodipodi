#
# Widgets for sodipodi
# Author: Lauris Kaplinski <lauris@ximian.com>
#

gladedir = $(datadir)/sodipodi

INCLUDES = \
	-I.. \
	-I$(top_srcdir)/src \
	$(SODIPODI_CFLAGS) \
	-DSODIPODI_PIXMAPDIR=\""$(gladedir)"\"

noinst_LIBRARIES = libspwidgets.a

BUILT_SOURCES = sp-dock-typebuiltins.c sp-dock-typebuiltins.h

CLEANFILES = sp-dock-typebuiltins.c sp-dock-typebuiltins.h

libspwidgets_a_SOURCES = \
	icon.c icon.h \
	button.c button.h \
	menu.c menu.h \
	ruler.c ruler.h \
	sp-toolbox.c sp-toolbox.h \
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
	spw-utilities.c spw-utilities.h \
	sp-widget.c sp-widget.h \
	$(spdock_headers) $(spdock_sources)

spdock_headers = \
	sp-dock-object.h \
	sp-dock-master.h \
	sp-dock.h \
	sp-dock-item.h \
	sp-dock-item-grip.h \
	sp-dock-notebook.h \
	sp-dock-paned.h \
	sp-dock-tablabel.h \
	sp-dock-layout.h \
	sp-dock-placeholder.h

spdock_sources = \
	sp-dock-object.c \
	sp-dock-master.c \
	sp-dock.c \
	sp-dock-item.c \
	sp-dock-item-grip.c \
	sp-dock-notebook.c \
	sp-dock-paned.c \
	sp-dock-tablabel.c \
	sp-dock-layout.c \
	sp-dock-placeholder.c \
	sp-dock-typebuiltins.c sp-dock-typebuiltins.h

#	sp-value-winder.c sp-value-winder.h
#	sp-spin-button.c sp-spin-button.h

#noinst_PROGRAMS = test-widgets test-wrapbox
#noinst_PROGRAMS = test-wrapbox

#test_widgets_SOURCES = \
#	test-widgets.c \
#	sp-value-winder.c sp-value-winder.h \
#	sp-color-slider.c sp-color-slider.h \
#	sp-color-selector.c sp-color-selector.h

#test_widgets_LDADD = $(SODIPODI_LIBS)

#test_wrapbox_SOURCES = \
#	test-wrapbox.c \
#	sp-wrap-box.c sp-wrap-box.h \
#	sp-hwrap-box.c sp-hwrap-box.h \
#	sp-vwrap-box.c sp-vwrap-box.h

#test_wrapbox_LDADD = $(SODIPODI_LIBS)

sp-dock-typebuiltins.c: $(spdock_headers)
	glib-mkenums \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"sp-dock-typebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(spdock_headers) | \
		sed -e 's/s_p/sp_/g' -e 's/S_P/SP_/g' -e 's/_PDOCK/_DOCK/g' > xgen-gtbec \
	&& cp xgen-gtbec $(@F) \
	&& rm -f xgen-gtbec

sp-dock-typebuiltins.h: $(spdock_headers)
	glib-mkenums \
			--fhead "#ifndef __SPDOCKTYPEBUILTINS_H__\n" \
			--fhead "#define __SPDOCKTYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <widgets/sp-dock.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __SPDOCKTYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define SP_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(spdock_headers) | \
		sed -e 's/s_p/sp_/g' -e 's/S_P/SP_/g' -e 's/_PDOCK/_DOCK/g' > xgen-gth \
	&& cp xgen-gth $(@F) \
	&& rm -f xgen-gth
