#define OBJECT_STROKE_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../mdi-desktop.h"
#include "../selection.h"
#include "../desktop-handles.h"
#include "object-stroke.h"

static void apply_stroke (void);
static void sp_stroke_read_selection (void);

static void sp_stroke_show_dialog (void);
static void sp_stroke_hide_dialog (void);

/* glade_gui_handlers */

void sp_object_stroke_apply (GnomePropertyBox * propertybox, gint pagenum);
void sp_object_stroke_close (void);

void sp_object_stroke_changed (void);

/* Signal handlers */

static void sp_stroke_view_changed (GnomeMDI * mdi, GtkWidget * widget, gpointer data);
static void sp_stroke_sel_changed (SPSelection * selection, gpointer data);
static void sp_stroke_sel_destroy (GtkObject * object, gpointer data);

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;

GtkToggleButton * tb_stroked;
GnomeColorPicker * cp_stroke_color;
GtkSpinButton * sp_stroke_width;
GtkToggleButton * tb_stroke_scaled;
GtkToggleButton * tb_join_miter;
GtkToggleButton * tb_join_round;
GtkToggleButton * tb_join_bevel;
GtkToggleButton * tb_cap_butt;
GtkToggleButton * tb_cap_round;
GtkToggleButton * tb_cap_square;

static SPCSSAttr * css = NULL;

static guint view_changed_id = 0;
static guint sel_destroy_id = 0;
static guint sel_changed_id = 0;
static SPSelection * sel_current;

void sp_object_stroke_dialog (void)
{
	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "stroke");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "stroke");
		tb_stroked = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_stroked");
		cp_stroke_color = (GnomeColorPicker *) glade_xml_get_widget (xml, "stroke_dialog_color");
		sp_stroke_width = (GtkSpinButton *) glade_xml_get_widget (xml, "stroke_dialog_width");
		tb_stroke_scaled = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_scale");
		tb_join_miter = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_miter");
		tb_join_round = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_round");
		tb_join_bevel = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_bevel");
		tb_cap_butt = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_cap_butt");
		tb_cap_round = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_cap_round");
		tb_cap_square = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_cap_square");
	}

	sp_stroke_read_selection ();
	sp_stroke_show_dialog ();
}

static void
sp_stroke_read_selection (void)
{
	SPSelection * selection;
	const GSList * l;
	SPRepr * repr;
	const gchar * str;
	SPStrokeType stroke_type;
	guint32 stroke_color;
	gdouble stroke_opacity;
	gdouble stroke_width;
	SPSVGUnit stroke_units;
	ArtPathStrokeJoinType stroke_join;
	ArtPathStrokeCapType stroke_cap;

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
	} else {
		repr = sp_document_repr_root (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP));
		css = sp_repr_css_attr (repr, "style");
	}

	if (css != NULL) {
		str = sp_repr_css_property (css, "stroke", "none");
		stroke_type = sp_svg_read_stroke_type (str);

		switch (stroke_type) {
		case SP_STROKE_NONE:
			gtk_toggle_button_set_active (tb_stroked, FALSE);
			break;
		case SP_STROKE_COLOR:
			gtk_toggle_button_set_active (tb_stroked, TRUE);
			stroke_color = sp_svg_read_color (str);
			str = sp_repr_css_property (css, "stroke-opacity", "100%");
			stroke_opacity = sp_svg_read_percentage (str);
			gnome_color_picker_set_i8 (cp_stroke_color,
				(stroke_color >> 24),
				(stroke_color >> 16) & 0xff,
				(stroke_color >>  8) & 0xff,
				((guint) (stroke_opacity * 255 + 0.5)) & 0xff);
			break;
		}

		str = sp_repr_css_property (css, "stroke-width", "1.0");
		stroke_width = sp_svg_read_length (&stroke_units, str);
		gtk_spin_button_set_value (sp_stroke_width, stroke_width);
		gtk_toggle_button_set_active (tb_stroke_scaled, stroke_units != SP_SVG_UNIT_PIXELS);

		str = sp_repr_css_property (css, "stroke-linejoin", "miter");
		stroke_join = sp_svg_read_stroke_join (str);
		switch (stroke_join) {
		case ART_PATH_STROKE_JOIN_MITER:
			gtk_toggle_button_set_active (tb_join_miter, TRUE);
			break;
		case ART_PATH_STROKE_JOIN_ROUND:
			gtk_toggle_button_set_active (tb_join_round, TRUE);
			break;
		case ART_PATH_STROKE_JOIN_BEVEL:
			gtk_toggle_button_set_active (tb_join_bevel, TRUE);
			break;
		default:
			g_assert_not_reached ();
		}

		str = sp_repr_css_property (css, "stroke-linecap", "butt");
		stroke_cap = sp_svg_read_stroke_cap (str);
		switch (stroke_cap) {
		case ART_PATH_STROKE_CAP_BUTT:
			gtk_toggle_button_set_active (tb_cap_butt, TRUE);
			break;
		case ART_PATH_STROKE_CAP_ROUND:
			gtk_toggle_button_set_active (tb_cap_round, TRUE);
			break;
		case ART_PATH_STROKE_CAP_SQUARE:
			gtk_toggle_button_set_active (tb_cap_square, TRUE);
			break;
		default:
			g_assert_not_reached ();
		}
	}
}

void
sp_object_stroke_apply (GnomePropertyBox * propertybox, gint pagenum)
{
	gdouble stroke_width;
	gchar cstr[80];

	if (pagenum != 0) return;

	/* fixme: */
	if (css == NULL) return;

	cstr[79] = '\0';

	if (gtk_toggle_button_get_active (tb_stroked)) {
		sp_svg_write_color (cstr, 79,
			((guint32) (cp_stroke_color->r * 255 + 0.5) << 24) |
			((guint32) (cp_stroke_color->g * 255 + 0.5) << 16) |
			((guint32) (cp_stroke_color->b * 255 + 0.5) <<  8));
		sp_repr_css_set_property (css, "stroke", cstr);
	} else {
		sp_repr_css_set_property (css, "stroke", "none");
	}

	sp_svg_write_percentage (cstr, 79, cp_stroke_color->a);
	sp_repr_css_set_property (css, "stroke-opacity", cstr);

	stroke_width = gtk_spin_button_get_value_as_float (sp_stroke_width);

	if (gtk_toggle_button_get_active (tb_stroke_scaled)) {
		sp_svg_write_length (cstr, 79, stroke_width, SP_SVG_UNIT_USER);
		sp_repr_css_set_property (css, "stroke-width", cstr);
	} else {
		sp_svg_write_length (cstr, 79, stroke_width, SP_SVG_UNIT_PIXELS);
		sp_repr_css_set_property (css, "stroke-width", cstr);
	}

	if (gtk_toggle_button_get_active (tb_join_round)) {
		sp_repr_css_set_property (css, "stroke-linejoin", "round");
	} else if (gtk_toggle_button_get_active (tb_join_bevel)) {
		sp_repr_css_set_property (css, "stroke-linejoin", "bevel");
	} else {
		sp_repr_css_set_property (css, "stroke-linejoin", "miter");
	}

	if (gtk_toggle_button_get_active (tb_cap_round)) {
		sp_repr_css_set_property (css, "stroke-linecap", "round");
	} else if (gtk_toggle_button_get_active (tb_cap_square)) {
		sp_repr_css_set_property (css, "stroke-linecap", "square");
	} else {
		sp_repr_css_set_property (css, "stroke-linecap", "butt");
	}

	apply_stroke ();
}

void
sp_object_stroke_close (void)
{
	sp_stroke_hide_dialog ();
}

void
sp_object_stroke_changed (void)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (dialog));
}

static void
apply_stroke (void)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPRepr * repr;
	const GSList * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	selection = SP_DT_SELECTION (desktop);

	l = sp_selection_repr_list (selection);

	if (l != NULL) {
		while (l != NULL) {
			repr = (SPRepr *) l->data;
			sp_repr_css_change_recursive (repr, css, "style");
			l = l->next;
		}
	} else {
		repr = sp_document_repr_root (SP_DT_DOCUMENT (desktop));
		sp_repr_css_change (repr, css, "style");
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

/*
 * selection handlers
 *
 */

static void
sp_stroke_sel_changed (SPSelection * selection, gpointer data)
{
	sp_stroke_read_selection ();
}

static void
sp_stroke_sel_destroy (GtkObject * object, gpointer data)
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
sp_stroke_view_changed (GnomeMDI * mdi, GtkWidget * widget, gpointer data)
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
				GTK_SIGNAL_FUNC (sp_stroke_sel_destroy), NULL);
		}
		if (sel_changed_id < 1) {
			sel_changed_id = gtk_signal_connect (GTK_OBJECT (sel_current), "changed",
				GTK_SIGNAL_FUNC (sp_stroke_sel_changed), NULL);
		}
	}

	sp_stroke_read_selection ();
}

static void
sp_stroke_show_dialog (void)
{
	g_return_if_fail (dialog != NULL);

	sel_current = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sel_current != NULL) {
		if (sel_destroy_id < 1) {
			sel_destroy_id = gtk_signal_connect (GTK_OBJECT (sel_current), "destroy",
				GTK_SIGNAL_FUNC (sp_stroke_sel_destroy), NULL);
		}
		if (sel_changed_id < 1) {
			sel_changed_id = gtk_signal_connect (GTK_OBJECT (sel_current), "changed",
				GTK_SIGNAL_FUNC (sp_stroke_sel_changed), NULL);
		}
	}

	if (view_changed_id < 1) {
		view_changed_id = gtk_signal_connect (GTK_OBJECT (SODIPODI), "view_changed",
			GTK_SIGNAL_FUNC (sp_stroke_view_changed), NULL);
	}
	if (!GTK_WIDGET_VISIBLE (dialog)) {
		gtk_widget_show (dialog);
	}
}

void
sp_stroke_hide_dialog (void)
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

