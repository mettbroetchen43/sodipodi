#define SP_DESKTOP_PROPERTIES_C

#include <glade/glade.h>
#include <libgnomeui/gnome-color-picker.h>
#include "../helper/unit-menu.h"
#include "desktop-properties.h"
#include "../sodipodi.h"
#include "../desktop-handles.h"
#include "../svg/svg.h"

/*
 * Very-very basic desktop properties dialog
 *
 */ 

#define MM2PT(v) ((v) * 72.0 / 25.4)
#define PT2MM(v) ((v) * 25.4 / 72.0)

static GladeXML  * xml = NULL;
static GtkWidget * dialog = NULL;

static void sp_desktop_dialog_setup (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);

static void grid_unit_set (SPUnitMenu * menu, SPSVGUnit system, SPMetric metric, gpointer data);

void
sp_desktop_dialog (void)
{
	GtkWidget * t, * o, * u;

	if (dialog == NULL) {
		g_assert (xml == NULL);
		xml = glade_xml_new (SODIPODI_GLADEDIR "/desktop.glade", "desktop_dialog");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "desktop_dialog");
		
		/* fixme: experimental */
		t = glade_xml_get_widget (xml, "grid_table");
		o = glade_xml_get_widget (xml, "grid_units");
		u = sp_unitmenu_new (SP_SVG_UNIT_ABSOLUTE, SP_SVG_UNIT_ABSOLUTE, SP_MM, TRUE);
		gtk_widget_show (u);
		gtk_widget_unparent (o);
		gtk_table_attach (GTK_TABLE (t), u, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		gtk_signal_connect (GTK_OBJECT (u), "set_unit",
				    GTK_SIGNAL_FUNC (grid_unit_set), dialog);

		gtk_signal_connect_while_alive (GTK_OBJECT (sodipodi), "activate_desktop",
						GTK_SIGNAL_FUNC (sp_desktop_dialog_setup), NULL,
						GTK_OBJECT (dialog));
	} else {
		if (!GTK_WIDGET_VISIBLE (dialog))
			gtk_widget_show (dialog);
	}

	sp_desktop_dialog_setup (SODIPODI, SP_ACTIVE_DESKTOP, NULL);
}

static void
grid_unit_set (SPUnitMenu * menu, SPSVGUnit system, SPMetric metric, gpointer data)
{
	g_print ("System %d metric %d\n", system, metric);
}

/*
 * Fill entries etc. with default values
 */

static void
sp_desktop_dialog_setup (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data)
{
	SPNamedView * nv;
	GtkWidget * w;

	g_assert (sodipodi != NULL);
	g_assert (SP_IS_SODIPODI (sodipodi));
	g_assert (dialog != NULL);

	if (!desktop) return;

	nv = desktop->namedview;

	/* Show grid */
	w = glade_xml_get_widget (xml, "show_grid");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), nv->showgrid);

	/* Snap to grid */
	w = glade_xml_get_widget (xml, "snap_to_grid");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), nv->snaptogrid);

	/* Origin */
	w = glade_xml_get_widget (xml, "grid_origin_x");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (nv->gridorigin.x));
	w = glade_xml_get_widget (xml, "grid_origin_y");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (nv->gridorigin.y));

	/* Spacing */
	w = glade_xml_get_widget (xml, "grid_spacing_x");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (nv->gridspacing.x));
	w = glade_xml_get_widget (xml, "grid_spacing_y");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (nv->gridspacing.y));

	/* Tolerance */
	w = glade_xml_get_widget (xml, "grid_snap_distance");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), nv->gridtolerance);

	/* Color */
	w = glade_xml_get_widget (xml, "grid_color");
	gnome_color_picker_set_i8 (GNOME_COLOR_PICKER (w),
				   (nv->gridcolor >> 24) & 0xff,
				   (nv->gridcolor >> 16) & 0xff,
				   (nv->gridcolor >> 8) & 0xff,
				   nv->gridcolor & 0xff);

	/* Show guides */
	w = glade_xml_get_widget (xml, "show_guides");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), nv->showguides);

	/* Snap to grid */
	w = glade_xml_get_widget (xml, "snap_to_guides");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), nv->snaptoguides);

	/* Tolerance */
	w = glade_xml_get_widget (xml, "guide_snap_distance");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), nv->guidetolerance);

	/* Color */
	w = glade_xml_get_widget (xml, "guide_color");
	gnome_color_picker_set_i8 (GNOME_COLOR_PICKER (w),
				   (nv->guidecolor >> 24) & 0xff,
				   (nv->guidecolor >> 16) & 0xff,
				   (nv->guidecolor >> 8) & 0xff,
				   nv->guidecolor & 0xff);

	/* Color */
	w = glade_xml_get_widget (xml, "guide_hicolor");
	gnome_color_picker_set_i8 (GNOME_COLOR_PICKER (w),
				   (nv->guidehicolor >> 24) & 0xff,
				   (nv->guidehicolor >> 16) & 0xff,
				   (nv->guidehicolor >> 8) & 0xff,
				   nv->guidehicolor & 0xff);
}

void
sp_desktop_dialog_close (GtkWidget * widget)
{
	g_assert (dialog != NULL);

	if (GTK_WIDGET_VISIBLE (dialog))
		gtk_widget_hide (dialog);
}


void
sp_desktop_dialog_apply (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPRepr * repr;
	GtkWidget * w;
	gdouble t;
	guint8 r, g, b, a;
	gchar color[32];

	g_assert (dialog != NULL);

	desktop = SP_ACTIVE_DESKTOP;

	/* Fixme: Implement setting defaults */
	g_return_if_fail (desktop != NULL);

	repr = SP_OBJECT (desktop->namedview)->repr;

	/* Show grid */
	w = glade_xml_get_widget (xml, "show_grid");
	sp_repr_set_attr (repr, "showgrid", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? "true" : NULL);

	/* Snap to grid */
	w = glade_xml_get_widget (xml, "snap_to_grid");
	sp_repr_set_attr (repr, "snaptogrid", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? "true" : NULL);

	/* Origin */
	w = glade_xml_get_widget (xml, "grid_origin_x");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "gridoriginx", MM2PT (t));
	w = glade_xml_get_widget (xml, "grid_origin_y");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "gridoriginy", MM2PT (t));

	/* Spacing */
	w = glade_xml_get_widget (xml, "grid_spacing_x");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "gridspacingx", MM2PT (t));
	w = glade_xml_get_widget (xml, "grid_spacing_y");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "gridspacingy", MM2PT (t));

	/* Tolerance */
	w = glade_xml_get_widget (xml, "grid_snap_distance");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "gridtolerance", t);

	/* Color */
	w = glade_xml_get_widget (xml, "grid_color");
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER (w), &r, &g, &b, &a);
	sp_svg_write_color (color, 32, (r << 24) | (g << 16) | (b << 8));
	sp_repr_set_attr (repr, "gridcolor", color);
	sp_repr_set_double_attribute (repr, "gridopacity", (gdouble) a / 255.0);

	/* Show guides */
	w = glade_xml_get_widget (xml, "show_guides");
	sp_repr_set_attr (repr, "showguides", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? "true" : NULL);

	/* Snap to grid */
	w = glade_xml_get_widget (xml, "snap_to_guides");
	sp_repr_set_attr (repr, "snaptoguides", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? "true" : NULL);

	/* Tolerance */
	w = glade_xml_get_widget (xml, "guide_snap_distance");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "guidetolerance", t);

	/* Color */
	w = glade_xml_get_widget (xml, "guide_color");
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER (w), &r, &g, &b, &a);
	sp_svg_write_color (color, 32, (r << 24) | (g << 16) | (b << 8));
	sp_repr_set_attr (repr, "guidecolor", color);
	sp_repr_set_double_attribute (repr, "guideopacity", (gdouble) a / 255.0);

	/* Color */
	w = glade_xml_get_widget (xml, "guide_hicolor");
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER (w), &r, &g, &b, &a);
	sp_svg_write_color (color, 32, (r << 24) | (g << 16) | (b << 8));
	sp_repr_set_attr (repr, "guidehicolor", color);
	sp_repr_set_double_attribute (repr, "guidehiopacity", (gdouble) a / 255.0);

	sp_document_done (SP_DT_DOCUMENT (desktop));
}


