#define OBJECT_FILL_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../selection.h"
#include "../display/fill.h"
#include "object-fill.h"

static void apply_fill_list (GList * list);

static void read_selection (void);
static void show_fill (void);

GladeXML * xml = NULL;
GtkWidget * dialog = NULL;
SPCSSAttr * settings = NULL;

void
sp_object_fill_selection_changed (void)
{
#if 0
	if (dialog != NULL) {
		if (GTK_WIDGET_VISIBLE (dialog)) {
			read_selection ();
			show_fill ();
		}
	}
#endif
}

void sp_object_fill (void)
{
#if 0
	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "fill");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "fill");
	}

	read_selection ();
	show_fill ();
#endif
};

static void
read_selection (void)
{
#if 0
	GList * list;
	SPRepr * repr;

	if (settings != NULL)
		sp_repr_css_attr_unref (settings);
	settings = NULL;

	list = sp_selection_list ();

	if (list != NULL) {
		repr = (SPRepr *) list->data;
		settings = sp_repr_css_attr_inherited (repr, "style");
	}
#endif
};

static void
show_fill (void)
{
#if 0
	GtkToggleButton * tb;
	GtkColorSelection * cs;
	gdouble color[4];
	SPFill * fill;

	g_assert (dialog != NULL);

	fill = sp_fill_new ();
	if (settings != NULL) {
		sp_fill_read (fill, settings);
	}

	cs = (GtkColorSelection *) glade_xml_get_widget (xml, "fill_color");

	switch (fill->type) {
	case SP_FILL_NONE:
		tb = (GtkToggleButton *) glade_xml_get_widget (xml, "type_none");
		gtk_toggle_button_set_active (tb, TRUE);
		gtk_widget_set_sensitive ((GtkWidget *) cs, FALSE);
		break;
	case SP_FILL_COLOR:
		tb = (GtkToggleButton *) glade_xml_get_widget (xml, "type_color");
		gtk_toggle_button_set_active (tb, TRUE);
		gtk_widget_set_sensitive ((GtkWidget *) cs, TRUE);
		gtk_color_selection_set_opacity (cs, TRUE);
		color[0] = (double)((fill->color >> 24) & 0xff) / 255;
		color[1] = (double)((fill->color >> 16) & 0xff) / 255;
		color[2] = (double)((fill->color >>  8) & 0xff) / 255;
		color[3] = (double)((fill->color      ) & 0xff) / 255;
		gtk_color_selection_set_color (cs, color);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	sp_fill_unref (fill);
	gtk_widget_show (dialog);
#endif
}

void
sp_object_fill_close (void)
{
#if 0
	g_assert (dialog != NULL);
	gtk_widget_hide (dialog);
g_print ("meso\n");
#endif
}

void
sp_object_fill_apply (void)
{
#if 0
	GtkColorSelection * cs;
	gdouble color[4];
	GList * list;
	guint32 cval;
	gchar cstr[80];

	cs = (GtkColorSelection *) glade_xml_get_widget (xml, "fill_color");
	gtk_color_selection_get_color (cs, color);

	cval =
		(guint)(color[0] * 255) << 24 |
		(guint)(color[1] * 255) << 16 |
		(guint)(color[2] * 255) << 8 |
		(guint)(color[3] * 255);

	cstr[79] = '\0';
	sp_svg_write_color (cstr, 79, cval);

	sp_repr_css_set_property (settings, "fill", cstr);

	list = sp_selection_list ();
	apply_fill_list (list);
#endif
}

void
sp_object_fill_color_changed (void)
{
#if 0
	gnome_property_box_changed ((GnomePropertyBox *) dialog);
#endif
}

static void ok_clicked (GtkButton * button, gpointer data)
{
#if 0
	GList * list;

	list = sp_selection_list ();

	if (list != NULL) {
		apply_fill_list (list);
	}
	gtk_widget_destroy (GTK_WIDGET (data));
#endif
}

static void apply_clicked (GtkButton * button, gpointer data)
{
#if 0
	GList * list;

	list = sp_selection_list ();

	if (list != NULL) {
		apply_fill_list (list);
	}
#endif
}

static void cancel_clicked (GtkButton * button, gpointer data)
{
#if 0
	gtk_widget_destroy (GTK_WIDGET (data));
#endif
}

static void
apply_fill_list (GList * list)
{
#if 0
	SPRepr * repr;

	while (list != NULL) {
		repr = (SPRepr *) list->data;
		sp_repr_css_change_recursive (repr, settings, "style");
		list = list->next;
	}
#endif
}

void
sp_object_fill_none (void)
{
#if 0
	sp_repr_css_set_property (settings, "fill", "none");
	gnome_property_box_changed ((GnomePropertyBox *) dialog);
#endif
}

void
sp_object_fill_color (void)
{
#if 0
	GtkColorSelection * cs;
	gdouble color[4];
	guint32 cval;
	gchar cstr[80];

	cs = (GtkColorSelection *) glade_xml_get_widget (xml, "fill_color");
	gtk_color_selection_get_color (cs, color);

	cval =
		(guint)(color[0] * 255) << 24 |
		(guint)(color[1] * 255) << 16 |
		(guint)(color[2] * 255) << 8 |
		(guint)(color[3] * 255);

	cstr[79] = '\0';
	sp_svg_write_color (cstr, 79, cval);

	sp_repr_css_set_property (settings, "fill", cstr);

	gnome_property_box_changed ((GnomePropertyBox *) dialog);
#endif
}

void
sp_object_fill_gradient (void)
{
}

void
sp_object_fill_special (void)
{
}

void
sp_object_fill_fractal (void)
{
}

