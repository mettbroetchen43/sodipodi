#define OBJECT_STROKE_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../sp-shape.h"
#include "../selection.h"
#include "object-stroke.h"

void stroke_dialog_ok (GtkButton * button, gpointer data);
void stroke_dialog_apply (GtkButton * button, gpointer data);
void stroke_dialog_cancel (GtkButton * button, gpointer data);
void stroke_dialog_stroked_toggled (GtkToggleButton * button, gpointer data);
void stroke_dialog_color_set (GnomeColorPicker * picker, gpointer data);
void stroke_dialog_width_changed (GtkSpinButton * spinbutton, gpointer data);
void stroke_dialog_scale_toggled (GtkToggleButton * button, gpointer data);
void stroke_dialog_join_miter (GtkToggleButton * button, gpointer data);
void stroke_dialog_join_round (GtkToggleButton * button, gpointer data);
void stroke_dialog_join_bevel (GtkToggleButton * button, gpointer data);

static void stroke_dialog_show (SPCSSAttr * css);
static void stroke_dialog_hide (void);

static void apply_stroke_list (GList * list);

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;
static SPCSSAttr * settings = NULL;
static guint32 stroke_color = 0x000000ff;

void
sp_object_stroke_selection_changed (void)
{
}

void sp_object_stroke_dialog (void)
{
#if 0
	SPRepr * repr;

	if (settings != NULL) sp_repr_css_attr_unref (settings);
	settings = NULL;

	repr = sp_selection_repr ();

	if (repr != NULL) {
		settings = sp_repr_css_attr_inherited (repr, "style");
	} else {
		settings = sp_repr_css_attr (sp_drawing_root (), "style");
	}

	stroke_dialog_show (settings);

	return;
#endif
}

void
stroke_dialog_ok (GtkButton * button, gpointer data)
{
#if 0
	GList * list;

	list = sp_selection_list ();

	if (list != NULL) {
		apply_stroke_list (list);
	}
	stroke_dialog_hide ();
#endif
}

void
stroke_dialog_apply (GtkButton * button, gpointer data)
{
#if 0
	GList * list;

	list = sp_selection_list ();

	if (list != NULL)
		apply_stroke_list (list);
#endif
}

void
stroke_dialog_cancel (GtkButton * button, gpointer data)
{
#if 0
	stroke_dialog_hide ();
#endif
}

void
stroke_dialog_stroked_toggled (GtkToggleButton * button, gpointer data)
{
#if 0
	gboolean active;
	gchar scs[80];

	active = gtk_toggle_button_get_active (button);

	if (active) {
		scs[79] = '\0';
		sp_svg_write_color (scs, 79, stroke_color);
		sp_repr_css_set_property (settings, "stroke", scs);
	} else {
		sp_repr_css_set_property (settings, "stroke", "none");
	}
#endif
}

void
stroke_dialog_color_set (GnomeColorPicker * picker, gpointer data)
{
#if 0
	guint32 r, g, b, a;

	r = picker->r * 255;
	g = picker->g * 255;
	b = picker->b * 255;
	a = picker->a * 255;

	stroke_color = (r << 24) | (g << 16) | (b << 8) | a;
#endif
}

void
stroke_dialog_width_changed (GtkSpinButton * spinbutton, gpointer data)
{
#if 0
	gdouble width;
	gchar sws[16];

	width = gtk_spin_button_get_value_as_float (spinbutton);

	snprintf (sws, 16, "%g", width);
	sp_repr_css_set_property (settings, "stroke-width", sws);
#endif
}


void
stroke_dialog_scale_toggled (GtkToggleButton * button, gpointer data)
{
#if 0
	gboolean active;

	active = gtk_toggle_button_get_active (button);

	stroke_scaled = active;
#endif
}

void
stroke_dialog_join_miter (GtkToggleButton * button, gpointer data)
{
#if 0
	gboolean active;

	active = gtk_toggle_button_get_active (button);

	if (active)
		sp_repr_css_set_property (settings, "stroke-linejoin", "miter");
#endif
}

void
stroke_dialog_join_round (GtkToggleButton * button, gpointer data)
{
#if 0
	gboolean active;

	active = gtk_toggle_button_get_active (button);

	if (active)
		sp_repr_css_set_property (settings, "stroke-linejoin", "round");
#endif
}

void
stroke_dialog_join_bevel (GtkToggleButton * button, gpointer data)
{
#if 0
	gboolean active;

	active = gtk_toggle_button_get_active (button);

	if (active)
		sp_repr_css_set_property (settings, "stroke-linejoin", "bevel");
#endif
}


static void
apply_stroke_list (GList * list)
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

static void
stroke_dialog_show (SPCSSAttr * css)
{
#if 0
	GtkToggleButton * tb;
	GnomeColorPicker * cp;
	GtkSpinButton * sp;
	SPStroke * stroke;

	stroke = sp_stroke_new ();
	if (css != NULL)
		sp_stroke_read (stroke, css);

	if (dialog == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/stroke-dialog.glade", NULL);
		g_assert (xml != NULL);

		glade_xml_signal_autoconnect (xml);

		dialog = glade_xml_get_widget (xml, "stroke_dialog");
		g_assert (dialog != NULL);
	} else {
		gtk_widget_show (dialog);
	}

	tb = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_stroked");
	gtk_toggle_button_set_active (tb, stroke->type == SP_STROKE_COLOR);
	cp = (GnomeColorPicker *) glade_xml_get_widget (xml, "stroke_dialog_color");
	gnome_color_picker_set_i8 (cp,
		stroke->color >> 24,
		(stroke->color >> 16) & 0xff,
		(stroke->color >> 8) & 0xff,
		stroke->color & 0xff);
	sp = (GtkSpinButton *) glade_xml_get_widget (xml, "stroke_dialog_width");
	gtk_spin_button_set_value (sp, stroke->width);
	tb = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_scale");
	gtk_toggle_button_set_active (tb, stroke->scaled);

	switch (stroke->join) {
		case ART_PATH_STROKE_JOIN_MITER:
			tb = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_miter");
			gtk_toggle_button_set_active (tb, TRUE);
			break;
		case ART_PATH_STROKE_JOIN_ROUND:
			tb = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_round");
			gtk_toggle_button_set_active (tb, TRUE);
			break;
		case ART_PATH_STROKE_JOIN_BEVEL:
			tb = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_bevel");
			gtk_toggle_button_set_active (tb, TRUE);
			break;
		default:
			g_assert_not_reached ();
	}
	sp_stroke_unref (stroke);
#endif
}

static void
stroke_dialog_hide (void)
{
#if 0
	g_assert (dialog != NULL);
	g_assert (GTK_WIDGET_VISIBLE (dialog));

	gtk_widget_hide (dialog);
#endif
}
