#define SP_OBJECT_FILL_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../mdi-desktop.h"
#include "../selection.h"
#include "../desktop-handles.h"
#include "object-fill.h"

static void apply_fill (void);
static void sp_fill_read_selection (void);

static void sp_fill_show_dialog (void);
static void sp_fill_hide_dialog (void);

/* glade gui handlers */

void sp_object_fill_apply (GnomePropertyBox * propertybox, gint pagenum);
void sp_object_fill_close (void);
void sp_object_fill_color_changed (void);
void sp_object_fill_none (void);
void sp_object_fill_color (void);
void sp_object_fill_gradient (void);
void sp_object_fill_special (void);
void sp_object_fill_fractal (void);

/* Signal handlers */

static void sp_fill_view_changed (GnomeMDI * mdi, GtkWidget * widget, gpointer data);
static void sp_fill_sel_changed (SPSelection * selection, gpointer data);
static void sp_fill_sel_destroy (GtkObject * object, gpointer data);

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;

GtkColorSelection * cs;
GtkToggleButton * tb_none, * tb_color;

static SPCSSAttr * css = NULL;

static guint view_changed_id = 0;
static guint sel_destroy_id = 0;
static guint sel_changed_id = 0;
static SPSelection * sel_current;

void sp_object_fill_dialog (void)
{
	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "fill");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "fill");
		cs = (GtkColorSelection *) glade_xml_get_widget (xml, "fill_color");
		gtk_color_selection_set_opacity (cs, TRUE);
		tb_none = (GtkToggleButton *) glade_xml_get_widget (xml, "type_none");
		tb_color = (GtkToggleButton *) glade_xml_get_widget (xml, "type_color");
	}

	sp_fill_read_selection ();
	sp_fill_show_dialog ();
}

static void
sp_fill_read_selection (void)
{
	SPSelection * selection;
	const GSList * l;
	SPRepr * repr;
	const gchar * str;
	SPFillType fill_type;
	guint32 fill_color;
	gdouble color[4];
	gdouble opacity;

	g_return_if_fail (dialog != NULL);

	if (css != NULL) {
		sp_repr_css_attr_unref (css);
		css = NULL;
	}

	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	g_return_if_fail (selection != NULL);

	l = sp_selection_repr_list (selection);

	if (l != NULL) {
		repr = (SPRepr *) l->data;
		css = sp_repr_css_attr_inherited (repr, "style");
	}

	if (css != NULL) {
		str = sp_repr_css_property (css, "fill", "none");
		fill_type = sp_svg_read_fill_type (str);

		switch (fill_type) {
		case SP_FILL_NONE:
			gtk_toggle_button_set_active (tb_none, TRUE);
			break;
		case SP_FILL_COLOR:
			gtk_toggle_button_set_active (tb_color, TRUE);
			fill_color = sp_svg_read_color (str);
			color[0] = (gdouble)((fill_color >> 24) & 0xff) / 255;
			color[1] = (gdouble)((fill_color >> 16) & 0xff) / 255;
			color[2] = (gdouble)((fill_color >>  8) & 0xff) / 255;
			str = sp_repr_css_property (css, "fill-opacity", "100%");
			opacity = sp_svg_read_percentage (str);
			color[3] = opacity;
			gtk_color_selection_set_color (cs, color);
			gtk_color_selection_set_color (cs, color);
			break;
		default:
#if 0
			g_assert_not_reached ();
#endif
			break;
		}
	}
}

void
sp_object_fill_apply (GnomePropertyBox * propertybox, gint pagenum)
{
	gdouble color[4];
	guint32 fill_color;
	SPFillType fill_type;
	gchar cstr[80];

	if (pagenum != 0) return;

	/* fixme: */
	if (css == NULL) return;

	gtk_color_selection_get_color (cs, color);

	fill_color =	(guint)(color[0] * 255) << 24 |
			(guint)(color[1] * 255) << 16 |
			(guint)(color[2] * 255) << 8 |
			(guint)(color[3] * 255);

	if (gtk_toggle_button_get_active (tb_color)) {
		fill_type = SP_FILL_COLOR;
	} else {
		fill_type = SP_FILL_NONE;
	}

	cstr[79] = '\0';
	sp_svg_write_fill_type (cstr, 79, fill_type, fill_color);
	sp_repr_css_set_property (css, "fill", cstr);

	sp_svg_write_percentage (cstr, 79, color[3]);
	sp_repr_css_set_property (css, "fill-opacity", cstr);

	apply_fill ();

	gnome_property_box_set_modified (GNOME_PROPERTY_BOX (dialog), FALSE);
}

void
sp_object_fill_close (void)
{
	sp_fill_hide_dialog ();
}

void
sp_object_fill_color_changed (void)
{
	gnome_property_box_changed ((GnomePropertyBox *) dialog);
}

void
sp_object_fill_none (void)
{
	gnome_property_box_changed ((GnomePropertyBox *) dialog);
}

void
sp_object_fill_color (void)
{
	gnome_property_box_changed ((GnomePropertyBox *) dialog);
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

static void
apply_fill (void)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPRepr * repr;
	const GSList * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	selection = SP_DT_SELECTION (desktop);

	l = sp_selection_repr_list (selection);

	while (l != NULL) {
		repr = (SPRepr *) l->data;
		sp_repr_css_change_recursive (repr, css, "style");
		l = l->next;
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

/*
 * selection handlers
 *
 */

static void
sp_fill_sel_changed (SPSelection * selection, gpointer data)
{
	sp_fill_read_selection ();
}

static void
sp_fill_sel_destroy (GtkObject * object, gpointer data)
{
	SPSelection * selection;

	selection = SP_SELECTION (object);

	if (selection == sel_current) {
		if (sel_destroy_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_destroy_id);
			sel_destroy_id = 0;
		}
		if (sel_changed_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_changed_id);
			sel_changed_id = 0;
		}
	}
}

static void
sp_fill_view_changed (GnomeMDI * mdi, GtkWidget * widget, gpointer data)
{
	if (sel_current != NULL) {
		if (sel_destroy_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_destroy_id);
			sel_destroy_id = 0;
		}
		if (sel_changed_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_changed_id);
			sel_changed_id = 0;
		}
	}

	sel_current = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sel_current != NULL) {
		if (sel_destroy_id < 1) {
			sel_destroy_id = gtk_signal_connect (GTK_OBJECT (sel_current), "destroy",
				GTK_SIGNAL_FUNC (sp_fill_sel_destroy), NULL);
		}
		if (sel_changed_id < 1) {
			sel_changed_id = gtk_signal_connect (GTK_OBJECT (sel_current), "changed",
				GTK_SIGNAL_FUNC (sp_fill_sel_changed), NULL);
		}
	}

	sp_fill_read_selection ();
}

static void
sp_fill_show_dialog (void)
{
	g_return_if_fail (dialog != NULL);

	sel_current = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sel_current != NULL) {
		if (sel_destroy_id < 1) {
			sel_destroy_id = gtk_signal_connect (GTK_OBJECT (sel_current), "destroy",
				GTK_SIGNAL_FUNC (sp_fill_sel_destroy), NULL);
		}
		if (sel_changed_id < 1) {
			sel_changed_id = gtk_signal_connect (GTK_OBJECT (sel_current), "changed",
				GTK_SIGNAL_FUNC (sp_fill_sel_changed), NULL);
		}
	}

	if (view_changed_id < 1) {
		view_changed_id = gtk_signal_connect (GTK_OBJECT (SODIPODI), "view_changed",
			GTK_SIGNAL_FUNC (sp_fill_view_changed), NULL);
	}
	if (!GTK_WIDGET_VISIBLE (dialog)) {
		gtk_widget_show (dialog);
	}
}

void
sp_fill_hide_dialog (void)
{
	g_return_if_fail (dialog != NULL);

	if (GTK_WIDGET_VISIBLE (dialog)) {
		gtk_widget_hide (dialog);
	}

	if (view_changed_id > 0) {
		gtk_signal_disconnect (GTK_OBJECT (SODIPODI), view_changed_id);
		view_changed_id = 0;
	}

	if (sel_current != NULL) {
		if (sel_destroy_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_destroy_id);
			sel_destroy_id = 0;
		}
		if (sel_changed_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_changed_id);
			sel_changed_id = 0;
		}
	}
}

