#define OBJECT_PROPERTIES_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection-chemistry.h"
#include "object-properties.h"

void sp_object_properties_page_changed (GtkNotebook * notebook,
					GtkNotebookPage * page,
					gint page_num,
					gpointer user_data);
  

void apply_stroke (SPCSSAttr * stroke_css);
void sp_object_properties_reread_stroke (void);
void sp_object_properties_apply_stroke (void);
void sp_object_stroke_changed (void);

void apply_fill (SPCSSAttr * fill_css);
void sp_object_properties_reread_fill (void);
void sp_object_properties_apply_fill (void);
void sp_object_fill_changed (void);

void sp_object_properties_reread_layout (void);
void sp_object_properties_apply_layout (void);
void sp_object_layout_changed (void);

// dialog
static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;

GtkNotebook * prop_notebook;
GtkButton * prop_apply;
GtkButton * prop_reread;

// stroke
GtkToggleButton * stroked;
GnomeColorPicker * cp_stroke_color;
GtkSpinButton * sp_stroke_width;

GtkToggleButton * stroke_scaled;
GtkToggleButton * join_miter;
GtkToggleButton * join_round;
GtkToggleButton * join_bevel;
GtkToggleButton * cap_butt;
GtkToggleButton * cap_round;
GtkToggleButton * cap_square;
GtkToggleButton * line_full;
GtkToggleButton * line_dashed;
GtkToggleButton * line_double;

// fill
GnomeColorPicker * cs;
GtkToggleButton * fill_none;
GtkToggleButton * fill_color;
GtkAdjustment * fill_red;
GtkAdjustment * fill_green;
GtkAdjustment * fill_blue;
GtkAdjustment * fill_alpha;

// layout
GtkSpinButton * position_hor;
GtkSpinButton * position_ver;
GtkSpinButton * dimension_width;
GtkSpinButton * dimension_height;


guint sel_changed_id = 0;

gboolean stroke_changed = FALSE;
gboolean stroke_reread = FALSE;
gboolean fill_changed = FALSE;
gboolean fill_reread = FALSE; 
gboolean layout_changed = FALSE;
gboolean layout_reread = FALSE;

static void sp_object_fill_adjustment_changed (GtkAdjustment * adjustment, gpointer data);
static void sp_object_fill_picker_changed (void);

/*
 * dialog invoking functions
 */

void sp_object_properties_stroke (void)
{
	if (!GTK_IS_WIDGET (dialog)) sp_object_properties_dialog ();
	gtk_notebook_set_page (prop_notebook, 0);
	sp_object_properties_reread_page ();
	if (sel_changed_id < 1) {
		sel_changed_id= gtk_signal_connect (GTK_OBJECT (SODIPODI),
						    "change_selection",
						    GTK_SIGNAL_FUNC (sp_object_properties_selection_changed),
						    NULL);
	}
	gtk_widget_show (dialog);
}

void sp_object_properties_fill (void)
{
	if (!GTK_IS_WIDGET (dialog)) sp_object_properties_dialog ();
	gtk_notebook_set_page (prop_notebook, 1);
	sp_object_properties_reread_page ();
	if (sel_changed_id < 1) {
		sel_changed_id= gtk_signal_connect (GTK_OBJECT (SODIPODI),
						    "change_selection",
						    GTK_SIGNAL_FUNC (sp_object_properties_selection_changed),
						    NULL);
	}
	gtk_widget_show (dialog);
}

void sp_object_properties_layout (void)
{
	if (!GTK_IS_WIDGET (dialog)) sp_object_properties_dialog ();
	gtk_notebook_set_page (prop_notebook, 2);
	sp_object_properties_reread_page ();
	if (sel_changed_id < 1) {
		sel_changed_id= gtk_signal_connect (GTK_OBJECT (SODIPODI),
						    "change_selection",
						    GTK_SIGNAL_FUNC (sp_object_properties_selection_changed),
						    NULL);
	}
	gtk_widget_show (dialog);
}


/*
 * dialog generation
 */

void sp_object_properties_dialog (void)
{
	if (xml == NULL) {
		GtkWidget * w;

		xml = glade_xml_new (SODIPODI_GLADEDIR "/object_props.glade", "properties");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "properties");

		prop_notebook = (GtkNotebook *) glade_xml_get_widget (xml, "properties_notebook");
		prop_apply = (GtkButton *) glade_xml_get_widget (xml, "properties_apply");
		prop_reread = (GtkButton *) glade_xml_get_widget (xml, "properties_reread");

		// stroke dialog
		stroked = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_stroked");
		cp_stroke_color = (GnomeColorPicker *) glade_xml_get_widget (xml, "stroke_dialog_color");
		sp_stroke_width = (GtkSpinButton *) glade_xml_get_widget (xml, "stroke_dialog_width");
		stroke_scaled = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_scale");

		join_miter = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_miter");
		join_round = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_round");
		join_bevel = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_join_bevel");

		cap_butt = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_cap_butt");
		cap_round = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_cap_round");
		cap_square = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_cap_square");

		line_full = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_line_full");
		line_dashed = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_line_dashed");
		line_double = (GtkToggleButton *) glade_xml_get_widget (xml, "stroke_dialog_line_double");

		// fill dialog
		cs = (GnomeColorPicker *) glade_xml_get_widget (xml, "fill_dialog_color");
		fill_none = (GtkToggleButton *) glade_xml_get_widget (xml, "fill_type_none");
		fill_color = (GtkToggleButton *) glade_xml_get_widget (xml, "fill_type_color");
		fill_red = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
		gtk_signal_connect (GTK_OBJECT (fill_red), "value_changed",
				    GTK_SIGNAL_FUNC (sp_object_fill_adjustment_changed), dialog);
		w = glade_xml_get_widget (xml, "r_scale");
		gtk_range_set_adjustment (GTK_RANGE (w), fill_red);
		w = glade_xml_get_widget (xml, "r_spin");
		gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (w), fill_red);
		fill_green = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
		gtk_signal_connect (GTK_OBJECT (fill_green), "value_changed",
				    GTK_SIGNAL_FUNC (sp_object_fill_adjustment_changed), dialog);
		w = glade_xml_get_widget (xml, "g_scale");
		gtk_range_set_adjustment (GTK_RANGE (w), fill_green);
		w = glade_xml_get_widget (xml, "g_spin");
		gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (w), fill_green);
		fill_blue = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
		gtk_signal_connect (GTK_OBJECT (fill_blue), "value_changed",
				    GTK_SIGNAL_FUNC (sp_object_fill_adjustment_changed), dialog);
		w = glade_xml_get_widget (xml, "b_scale");
		gtk_range_set_adjustment (GTK_RANGE (w), fill_blue);
		w = glade_xml_get_widget (xml, "b_spin");
		gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (w), fill_blue);
		fill_alpha = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
		gtk_signal_connect (GTK_OBJECT (fill_alpha), "value_changed",
				    GTK_SIGNAL_FUNC (sp_object_fill_adjustment_changed), dialog);
		w = glade_xml_get_widget (xml, "a_scale");
		gtk_range_set_adjustment (GTK_RANGE (w), fill_alpha);
		w = glade_xml_get_widget (xml, "a_spin");
		gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (w), fill_alpha);

		// layout dialog
		position_hor = (GtkSpinButton *) glade_xml_get_widget (xml, "position_hor"); 
		position_ver = (GtkSpinButton *) glade_xml_get_widget (xml, "position_ver"); 
		dimension_width = (GtkSpinButton *) glade_xml_get_widget (xml, "dimension_width"); 
		dimension_height = (GtkSpinButton *) glade_xml_get_widget (xml, "dimension_height"); 
	}

	sp_object_properties_reread_stroke ();
	sp_object_properties_reread_fill ();
	sp_object_properties_reread_layout ();
}

/*
 * dialog handlers
 */

void
sp_object_properties_selection_changed (void)
{
	SPDesktop * desktop;
	SPSelection * selection;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop != NULL) {
		selection = SP_DT_SELECTION (desktop);
		if (!sp_selection_is_empty(selection)) {

			// we really have a selection !!
      
			stroke_reread = TRUE;
			fill_reread = TRUE;
			layout_reread = TRUE;

			sp_object_properties_reread_page ();
			return;
		}
	}

	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
}


void
sp_object_properties_reread_page (void)
{
	gint page;

	page = gtk_notebook_get_current_page(prop_notebook);
	switch (page) {
	case 0:
		sp_object_properties_reread_stroke ();
		break;
	case 1:
		sp_object_properties_reread_fill ();
		break;
	case 2:
		sp_object_properties_reread_layout ();
		break;
	}
}

void
sp_object_properties_page_changed (GtkNotebook * notebook,
				   GtkNotebookPage * page,
				   gint page_num,
				   gpointer user_data) {
  switch (page_num) {
  case 0 :
    if (stroke_reread) sp_object_properties_reread_stroke ();
      else if (stroke_changed) {
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);
      } else {
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
      };
    break;
  case 1 :
    if (fill_reread) sp_object_properties_reread_fill ();
      else if (fill_changed) {
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);
      } else {
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
      };
    break;
  case 2 :
    if (layout_reread) sp_object_properties_reread_layout ();
      else if (layout_changed) {
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);
      } else {
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
      };
    break;
  }
}

void
sp_object_properties_close (void)
{
	g_assert (dialog != NULL);

	gtk_widget_hide (dialog);
	if (sel_changed_id > 0) {
		gtk_signal_disconnect (GTK_OBJECT (sodipodi), sel_changed_id);
		sel_changed_id = 0;
	}
}

void
sp_object_properties_apply (void)
{
	gint page;

	page = gtk_notebook_get_current_page(prop_notebook);
	switch (page) {
	case 0:
		sp_object_properties_apply_stroke ();

		stroke_changed = FALSE;
		gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
		break;
	case 1:
		sp_object_properties_apply_fill ();

		fill_changed = FALSE;
		gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
		break;
	case 2:
		sp_object_properties_apply_layout ();

		layout_changed = FALSE;
		gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
		break;
	}
}

/*
 * stroke
 */

void
sp_object_properties_reread_stroke (void)
{
	SPSelection * selection;
	SPCSSAttr * stroke_css;
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

	if (SP_ACTIVE_DESKTOP == NULL) return;
	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	g_return_if_fail (selection != NULL);

	l = sp_selection_repr_list (selection);

	if (l != NULL) {
		repr = (SPRepr *) l->data;
		stroke_css = sp_repr_css_attr_inherited (repr, "style");
	} else {
		repr = sp_document_repr_root (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP));
		stroke_css = sp_repr_css_attr (repr, "style");
	}

	if (stroke_css != NULL) {
		str = sp_repr_css_property (stroke_css, "stroke", "none");
		stroke_type = sp_svg_read_stroke_type (str);

		switch (stroke_type) {
		case SP_STROKE_NONE:
			gtk_toggle_button_set_active (stroked, FALSE);
			break;
		case SP_STROKE_COLOR:
			gtk_toggle_button_set_active (stroked, TRUE);
			stroke_color = sp_svg_read_color (str, 0x00000000);
			str = sp_repr_css_property (stroke_css, "stroke-opacity", "100%");
			stroke_opacity = sp_svg_read_percentage (str, 1.0);
			gnome_color_picker_set_i8 (cp_stroke_color,
				(stroke_color >> 24),
				(stroke_color >> 16) & 0xff,
				(stroke_color >>  8) & 0xff,
				((guint) (stroke_opacity * 255 + 0.5)) & 0xff);
			break;
		}

		str = sp_repr_css_property (stroke_css, "stroke-width", "1.0");
		stroke_width = sp_svg_read_length (&stroke_units, str, 1.0);
		gtk_spin_button_set_value (sp_stroke_width, stroke_width);
		gtk_toggle_button_set_active (stroke_scaled, stroke_units != SP_SVG_UNIT_PIXELS);

		str = sp_repr_css_property (stroke_css, "stroke-linejoin", "miter");
		stroke_join = sp_svg_read_stroke_join (str);
		switch (stroke_join) {
		case ART_PATH_STROKE_JOIN_MITER:
			gtk_toggle_button_set_active (join_miter, TRUE);
			break;
		case ART_PATH_STROKE_JOIN_ROUND:
			gtk_toggle_button_set_active (join_round, TRUE);
			break;
		case ART_PATH_STROKE_JOIN_BEVEL:
			gtk_toggle_button_set_active (join_bevel, TRUE);
			break;
		default:
			g_assert_not_reached ();
		}

		str = sp_repr_css_property (stroke_css, "stroke-linecap", "butt");
		stroke_cap = sp_svg_read_stroke_cap (str);
		switch (stroke_cap) {
		case ART_PATH_STROKE_CAP_BUTT:
			gtk_toggle_button_set_active (cap_butt, TRUE);
			break;
		case ART_PATH_STROKE_CAP_ROUND:
			gtk_toggle_button_set_active (cap_round, TRUE);
			break;
		case ART_PATH_STROKE_CAP_SQUARE:
			gtk_toggle_button_set_active (cap_square, TRUE);
			break;
		default:
			g_assert_not_reached ();
		}

		sp_repr_css_attr_unref (stroke_css);
	}

	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
	stroke_changed = FALSE;
	stroke_reread = FALSE;
}

void
sp_object_properties_apply_stroke (void)
{
	SPCSSAttr * stroke_css;
	gdouble stroke_width;
	gchar cstr[80];  

	stroke_css = sp_repr_css_attr_new ();

	cstr[79] = '\0';

	if (gtk_toggle_button_get_active (stroked)) {
		sp_svg_write_color (cstr, 79,
			((guint32) (cp_stroke_color->r * 255 + 0.5) << 24) |
			((guint32) (cp_stroke_color->g * 255 + 0.5) << 16) |
			((guint32) (cp_stroke_color->b * 255 + 0.5) <<  8));
		sp_repr_css_set_property (stroke_css, "stroke", cstr);
	} else {
		sp_repr_css_set_property (stroke_css, "stroke", "none");
	}

	sp_svg_write_percentage (cstr, 79, cp_stroke_color->a);
	sp_repr_css_set_property (stroke_css, "stroke-opacity", cstr);

	stroke_width = gtk_spin_button_get_value_as_float (sp_stroke_width);

	if (gtk_toggle_button_get_active (stroke_scaled)) {
		sp_svg_write_length (cstr, 79, stroke_width, SP_SVG_UNIT_USER);
		sp_repr_css_set_property (stroke_css, "stroke-width", cstr);
	} else {
		sp_svg_write_length (cstr, 79, stroke_width, SP_SVG_UNIT_PIXELS);
		sp_repr_css_set_property (stroke_css, "stroke-width", cstr);
	}

	if (gtk_toggle_button_get_active (join_round)) {
		sp_repr_css_set_property (stroke_css, "stroke-linejoin", "round");
	} else if (gtk_toggle_button_get_active (join_bevel)) {
		sp_repr_css_set_property (stroke_css, "stroke-linejoin", "bevel");
	} else {
		sp_repr_css_set_property (stroke_css, "stroke-linejoin", "miter");
	}

	if (gtk_toggle_button_get_active (cap_round)) {
		sp_repr_css_set_property (stroke_css, "stroke-linecap", "round");
	} else if (gtk_toggle_button_get_active (cap_square)) {
		sp_repr_css_set_property (stroke_css, "stroke-linecap", "square");
	} else {
		sp_repr_css_set_property (stroke_css, "stroke-linecap", "butt");
	}

	apply_stroke (stroke_css);
	sp_repr_css_attr_unref (stroke_css);
}


void
apply_stroke (SPCSSAttr * stroke_css)
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
			sp_repr_css_change_recursive (repr, stroke_css, "style");
			l = l->next;
		}
	} else {
		repr = sp_document_repr_root (SP_DT_DOCUMENT (desktop));
		sp_repr_css_change (repr, stroke_css, "style");
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_object_stroke_changed (void) {
 stroke_changed = TRUE;
 gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
 gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);
}

/*
 * fill
 */

void
sp_object_properties_reread_fill (void)
{
	SPSelection * selection;
	SPCSSAttr * fill_css;
	const GSList * l;
	SPRepr * repr;
	const gchar * str;
	SPFillType fill_type;
	guint32 f_color;
	gdouble opacity;

	g_return_if_fail (dialog != NULL);

	if (SP_ACTIVE_DESKTOP == NULL) return;
      	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	g_return_if_fail (selection != NULL);

	l = sp_selection_repr_list (selection);

	if (l != NULL) {
		repr = (SPRepr *) l->data;
		fill_css = sp_repr_css_attr_inherited (repr, "style");
	} else {
		fill_css = sp_repr_css_attr (sp_document_repr_root (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP)), "style");
	}

	if (fill_css != NULL) {
		str = sp_repr_css_property (fill_css, "fill", "none");
		fill_type = sp_svg_read_fill_type (str);

		switch (fill_type) {
		case SP_FILL_NONE:
			gtk_toggle_button_set_active (fill_none, TRUE);
			break;
		case SP_FILL_COLOR:
			gtk_toggle_button_set_active (fill_color, TRUE);
			f_color = sp_svg_read_color (str, 0x00000000);
			str = sp_repr_css_property (fill_css, "fill-opacity", "100%");
			opacity = sp_svg_read_percentage (str, 1.0);
			gnome_color_picker_set_i8 (cs,
				(f_color >> 24),
				(f_color >> 16) & 0xff,
				(f_color >>  8) & 0xff,
				((guint) (opacity * 255 + 0.5)) & 0xff);
			/* fixme: */
			sp_object_fill_picker_changed ();
			break;
		default:
#if 0
			g_assert_not_reached ();
#endif
			break;
		}

		sp_repr_css_attr_unref (fill_css);
	}
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
	fill_changed = FALSE;
	fill_reread = FALSE;
}

void
sp_object_properties_apply_fill (void)
{
	SPCSSAttr * fill_css;
	gchar cstr[80];

	fill_css = sp_repr_css_attr_new ();

	cstr[79] = '\0';

	sp_svg_write_color (cstr, 79,
			    ((guint32) (cs->r * 255 + 0.5) << 24) |
			    ((guint32) (cs->g * 255 + 0.5) << 16) |
			    ((guint32) (cs->b * 255 + 0.5) <<  8));
      
	if (gtk_toggle_button_get_active (fill_color)) {
	  sp_repr_css_set_property (fill_css, "fill", cstr);
	} else {
	  sp_repr_css_set_property (fill_css, "fill", "none");
	}

	sp_svg_write_percentage (cstr, 79, cs->a);
	sp_repr_css_set_property (fill_css, "fill-opacity", cstr);

	apply_fill (fill_css);
	sp_repr_css_attr_unref (fill_css);
}

void
apply_fill (SPCSSAttr * fill_css)
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
			sp_repr_css_change_recursive (repr, fill_css, "style");
			l = l->next;
		}
	} else {
		sp_repr_css_change (sp_document_repr_root (SP_DT_DOCUMENT (desktop)), fill_css, "style");
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_object_fill_changed (void)
{
	sp_object_fill_picker_changed ();
	fill_changed = TRUE;
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);
}

static void
sp_object_fill_adjustment_changed (GtkAdjustment * adjustment, gpointer data)
{
	gnome_color_picker_set_d (cs, fill_red->value, fill_green->value, fill_blue->value, fill_alpha->value);
	fill_changed = TRUE;
	gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);
}

static void
sp_object_fill_picker_changed (void)
{
	gdouble r, g, b, a;

	gnome_color_picker_get_d (cs, &r, &g, &b, &a);

	gtk_adjustment_set_value (fill_red, r);
	gtk_adjustment_set_value (fill_green, g);
	gtk_adjustment_set_value (fill_blue, b);
	gtk_adjustment_set_value (fill_alpha, a);
}

/*
 * layout
 */

void
sp_object_properties_reread_layout (void) {
  SPSelection * selection;
  SPDesktop * desktop;
  ArtDRect  bbox;
  gfloat w,h;

  desktop = SP_ACTIVE_DESKTOP;
  if (desktop == NULL) return;
  selection = SP_DT_SELECTION (desktop);
  if (selection == 0) return;

  sp_selection_bbox (selection, &bbox);
  w = bbox.x1 - bbox.x0;
  h = bbox.y1 - bbox.y0;

  gtk_spin_button_set_value (position_hor, bbox.x0); 
  gtk_spin_button_set_value (position_ver, bbox.y0); 
  gtk_spin_button_set_value (dimension_width, w); 
  gtk_spin_button_set_value (dimension_height, h); 

  gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
  layout_changed = FALSE;
  layout_reread = FALSE;


}


void
sp_object_properties_apply_layout (void) {
  SPDesktop * desktop;
  SPSelection * selection;
  ArtDRect  bbox;
  double p2o[6], o2n[6], scale[6], s[6], t[6];
  double dx, dy, nx, ny;

  desktop = SP_ACTIVE_DESKTOP;
  if (desktop == NULL) return;
  selection = SP_DT_SELECTION (desktop);
  if (selection == 0) return;

  sp_selection_bbox (selection, &bbox);

  art_affine_translate (p2o, -bbox.x0, -bbox.y0);

  dx = gtk_spin_button_get_value_as_float (dimension_width) / (bbox.x1 - bbox.x0);
  dy = gtk_spin_button_get_value_as_float (dimension_height) / (bbox.y1 - bbox.y0);
  art_affine_scale (scale, dx, dy);

  nx = gtk_spin_button_get_value_as_float (position_hor);
  ny = gtk_spin_button_get_value_as_float (position_ver);
  art_affine_translate (o2n, nx, ny);

  art_affine_multiply (s , p2o, scale);
  art_affine_multiply (t , s, o2n);

  sp_selection_apply_affine (selection, t);

  // update handels and undo
  sp_selection_changed (selection);
  sp_document_done (SP_DT_DOCUMENT (desktop));
   
  // update dialog
  gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), FALSE);
  layout_changed = FALSE;
  layout_reread = FALSE;
}


void
sp_object_layout_changed (void) {
 layout_changed = TRUE;
 gtk_widget_set_sensitive (GTK_WIDGET (prop_apply), TRUE);
 gtk_widget_set_sensitive (GTK_WIDGET (prop_reread), TRUE);

}

