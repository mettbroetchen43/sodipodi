#define __SP_TRANSFORMATION_C__

/*
 * Object transformation dialog
 *
 * Authors:
 *   Frank Felfe <innerspace@iname.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "config.h"

#include <gtk/gtkwindow.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkstock.h>

#include "helper/sp-intl.h"
#include "helper/unit-menu.h"
#include "widgets/icon.h"
#include "macros.h"
#include "sodipodi.h"
#include "selection.h"

/* Notebook pages */
/* These are hardcoded so do not play with them */

enum {
	SP_TRANSFORMATION_MOVE,
	SP_TRANSFORMATION_SCALE,
	SP_TRANSFORMATION_ROTATE,
	SP_TRANSFORMATION_SKEW
};

static void sp_transformation_dialog_present (unsigned int page);
static GtkWidget *sp_transformation_dialog_new (void);

static GtkWidget *sp_transformation_page_move_new (GObject *obj);
static void sp_transformation_move_update (GObject *obj, SPSelection *selection);

static GtkWidget *dlg = NULL;

void
sp_transformation_dialog_move (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_MOVE);
}

void
sp_transformation_dialog_scale (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_SCALE);
}

void
sp_transformation_dialog_rotate (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_ROTATE);
}

void
sp_transformation_dialog_skew (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_SKEW);
}

static void
sp_transformation_dialog_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (SODIPODI, object);

	dlg = NULL;
}

static void
sp_transformation_dialog_present (unsigned int page)
{
	GtkWidget *nbook;

	if (!dlg) {
		dlg = sp_transformation_dialog_new ();
	}

	nbook = g_object_get_data (G_OBJECT (dlg), "notebook");
	gtk_notebook_set_page (GTK_NOTEBOOK (nbook), page);

#if 0
	sp_transformation_apply_button_reset ();
#endif

	gtk_widget_show (dlg);
	gtk_window_present (GTK_WINDOW (dlg));
}

static void
sp_transformation_dialog_update_selection (GObject *obj, SPSelection *selection)
{
	GObject *notebook;
	GtkWidget *apply;
	int page;

	notebook = g_object_get_data (obj, "notebook");
	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	switch (page) {
	case SP_TRANSFORMATION_MOVE:
		sp_transformation_move_update (obj, selection);
		break;
	case SP_TRANSFORMATION_SCALE:
	case SP_TRANSFORMATION_ROTATE:
	case SP_TRANSFORMATION_SKEW:
	default:
		break;
	}

	apply = g_object_get_data (obj, "apply");
	if (selection && !sp_selection_is_empty (selection)) {
		gtk_widget_set_sensitive (apply, TRUE);
	} else {
		gtk_widget_set_sensitive (apply, FALSE); 
	}
}

static void
sp_transformation_dialog_selection_changed (Sodipodi *sodipodi, SPSelection *selection, GObject *obj)
{
	sp_transformation_dialog_update_selection (obj, selection);
}

static void
sp_transformation_dialog_selection_modified (Sodipodi *sodipodi, SPSelection *selection, unsigned int flags, GObject *obj)
{
	sp_transformation_dialog_update_selection (obj, selection);
}

static GtkWidget *
sp_transformation_dialog_new (void)
{
	GtkWidget *w, *hb, *vb, *nbook, *page, *img, *hs, *bb, *b;

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (w), _("Transform selection"));

	/* Toplevel hbox */
	hb = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hb);
	gtk_container_add (GTK_CONTAINER (w), hb);

	/* Toplevel vbox */
	vb = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vb);
	gtk_box_pack_start (GTK_BOX (hb), vb, TRUE, TRUE, 0);
	
	/* Notebook for individual transformations */
	nbook = gtk_notebook_new ();
	gtk_widget_show (nbook);
	gtk_box_pack_start (GTK_BOX (vb), nbook, TRUE, TRUE, 0);
	g_object_set_data (G_OBJECT (w), "notebook", nbook);
	/* Separator */
	hs = gtk_hseparator_new ();
	gtk_widget_show (hs);
	gtk_box_pack_start (GTK_BOX (vb), hs, FALSE, FALSE, 0);
	/* Buttons */
	bb = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (bb);
	gtk_box_pack_start (GTK_BOX (vb), bb, FALSE, FALSE, 0);
	b = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	g_object_set_data (G_OBJECT (w), "apply", b);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (bb), b, TRUE, TRUE, 0);
	b = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (bb), b, TRUE, TRUE, 0);

	/* Move page */
	img = sp_icon_new (SP_ICON_SIZE_NOTEBOOK, "move");
	gtk_widget_show (img);
	page = sp_transformation_page_move_new (G_OBJECT (w));
	gtk_widget_show (page);
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook), page, img);

	/* Connect signals */
	g_signal_connect (G_OBJECT (w), "destroy", G_CALLBACK (sp_transformation_dialog_destroy), NULL);
	g_signal_connect (G_OBJECT (SODIPODI), "change_selection", G_CALLBACK (sp_transformation_dialog_selection_changed), w);
	g_signal_connect (G_OBJECT (SODIPODI), "modify_selection", G_CALLBACK (sp_transformation_dialog_selection_modified), w);

	return w;
}

static void
sp_transformation_move_value_changed (GtkAdjustment *adj, GObject *obj)
{
	GtkWidget *apply;
	apply = g_object_get_data (obj, "apply");
	gtk_widget_set_sensitive (apply, TRUE);
}

static GtkWidget *
sp_transformation_page_move_new (GObject *obj)
{
	GtkWidget *frame, *vb, *tbl, *lbl, *img, *sb, *us;
	GtkAdjustment *adj;

	frame = gtk_frame_new (_("Move"));

	vb = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vb);

	/* Current position */
	tbl = gtk_table_new (5, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (tbl), 4);
	gtk_box_pack_start (GTK_BOX (vb), tbl, FALSE, FALSE, 0);
	lbl = gtk_label_new (_("X:"));
	gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	lbl = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_object_set_data (obj, "move_origin_x", lbl);
	lbl = gtk_label_new (_("Y:"));
	gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	lbl = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	g_object_set_data (obj, "move_origin_y", lbl);

	/* Unit selector */
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	g_object_set_data (obj, "move_units", us);

	/* New position */
	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "arrows_hor");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 2, 3, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "move_position_x", adj);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), adj);
	g_signal_connect (G_OBJECT (adj), "value_changed", G_CALLBACK (sp_transformation_move_value_changed), obj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "arrows_ver");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 3, 4, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "move_position_y", adj);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), adj);
	g_signal_connect (G_OBJECT (adj), "value_changed", G_CALLBACK (sp_transformation_move_value_changed), obj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	lbl = gtk_label_new (_("Units:"));
	gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 0, 1, 4, 5, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (tbl), us, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_widget_show_all (vb);

	return frame;
}

static void
sp_transformation_move_update (GObject *obj, SPSelection *selection)
{
	GtkLabel *lx, *ly;

	lx = g_object_get_data (obj, "move_origin_x");
	ly = g_object_get_data (obj, "move_origin_y");

	if (selection && !sp_selection_is_empty (selection)) {
		SPUnitSelector *us;
		const SPUnit *unit;
		NRRectF bbox;
		unsigned char c[64];
		sp_selection_bbox (selection, &bbox);
		us = g_object_get_data (obj, "move_units");
		unit = sp_unit_selector_get_unit (us);
		g_snprintf (c, 64, "%.2f%s", bbox.x0, unit->abbr_plural);
		gtk_label_set (lx, c);
		g_snprintf (c, 64, "%.2f%s", bbox.y0, unit->abbr_plural);
		gtk_label_set (ly, c);
	} else {
		gtk_label_set (lx, "");
		gtk_label_set (ly, "");
	}
}

#if 0

void
sp_transformation_display_position (ArtDRect * bbox, SPMetric metric) {
  GString * str;
  
  str = SP_PT_TO_METRIC_STRING (bbox->x0, metric);
  gtk_label_set (old_x, str->str);
  g_string_free (str, TRUE);
  str = SP_PT_TO_METRIC_STRING (bbox->y0, metric);
  gtk_label_set (old_y, str->str);
  g_string_free (str, TRUE);
}

void
sp_transformation_apply_move (SPSelection * selection) {
  double dx, dy;
  ArtPoint p;
  SPMetric metric;
  ArtDRect bbox;

  g_assert (transformation_dialog != NULL);
  g_assert (!sp_selection_is_empty (selection));

  metric = sp_transformation_get_move_metric ();

  dx = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (move_hor), metric);
  dy = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (move_ver), metric);

  switch (tr_move_type) {
  case RELATIVE:
    sp_selection_move_relative (selection, dx,dy);
    break;
  case ABSOLUTE:
    sp_selection_bbox (selection, &bbox);
    p.x = bbox.x0;
    p.y = bbox.y0;
    if (GTK_WIDGET_VISIBLE (expansion)) {
      if (gtk_toggle_button_get_active (use_align)) sp_transformation_get_align (selection,&p);
      if (gtk_toggle_button_get_active (use_center)) sp_transformation_get_center (selection, &p);
    }
    
    dx -= p.x;
    dy -= p.y;
    sp_selection_move_relative (selection, dx,dy);
    break;
  }
}
#endif

#if 0

#include <math.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkarrow.h>
#include <glade/glade.h>
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop.h"
#include "../desktop-handles.h"
#include "../svg/svg.h"
#include "../selection-chemistry.h"
#include "transformation.h"

typedef enum {
	ABSOLUTE,
	RELATIVE
} SPTransformationType;

typedef enum {
	SELECTION,
	DESKTOP
} SPReferType;

/*
 * handler functions for transformation dialog
 *
 * - maybe we should convert spinbutton values when metrics and metric types change
 * - error messages for extremes like width==0 etc 
 */ 

GladeXML * transformation_xml = NULL;
GladeXML * move_metric_xml = NULL, * scale_metric_xml = NULL, * skew_metric_xml = NULL, * center_metric_xml = NULL;

GtkWidget * transformation_dialog = NULL;

GtkWidget * apply_button;
GtkSpinButton * rotate_angle, * skew_value;
GtkSpinButton * move_hor, * move_ver;
GtkSpinButton * scale_hor, * scale_ver;
GtkSpinButton * center_x, * center_y;
GtkToggleButton * flip_hor, * flip_ver;
GtkToggleButton * use_align, * use_center;
GtkToggleButton * keep_width, * keep_height;
GtkToggleButton * skew_rotate;
GtkFrame * align_frame, * center_frame;
GtkWidget * rotate_left, * rotate_right;
GtkWidget * skew_hor, * skew_ver;
GtkWidget * scale_locked, * scale_unlocked;
GtkLabel * old_x, * old_y, * old_width, * old_height;
GtkButton * rotate_direction, * skew_direction;
GtkButton * move_type, * scale_type, * center_type;
GtkButton * lock_scale, * lock_skew;
GtkButton * angle_0, * angle_90, * angle_180, * angle_270;
GtkNotebook * trans_notebook;
GtkToggleButton * make_copy;
GtkToggleButton * align_tl, * align_tc, * align_tr, * align_cl, * align_cc, * align_cr, * align_bl, * align_bc, * align_br;
GtkButton * expand;
GtkHBox * expansion;
GtkArrow * arrow_expand;
GtkOptionMenu * move_metric_om;
GtkWidget * move_metrics, * move_pt, * move_mm, * move_cm, * move_in;
GtkOptionMenu * scale_metric_om;
GtkWidget * scale_metrics, * scale_pr, * scale_pt, * scale_mm, * scale_cm, * scale_in;
GtkOptionMenu * center_metric_om;
GtkWidget * center_metrics, * center_pr, * center_pt, * center_mm, * center_cm, * center_in;
GtkOptionMenu * skew_metric_om;
GtkWidget * skew_metrics, * skew_deg, * skew_pt, * skew_mm, * skew_cm, * skew_in;


static SPTransformationType tr_move_type=ABSOLUTE, tr_scale_type=ABSOLUTE;
static SPReferType tr_center_type=DESKTOP;
static guint sel_changed_id = 0;

// move, scale, rotate, skew, center
void sp_transformation_apply_move (SPSelection * selection);
void sp_transformation_apply_scale (SPSelection * selection);
void sp_transformation_apply_rotate (SPSelection * selection);
void sp_transformation_apply_skew (SPSelection * selection);
void sp_transformation_move_update (SPSelection * selection);
void sp_transformation_scale_update (SPSelection * selection);
void sp_transformation_rotate_update (SPSelection * selection);
void sp_transformation_skew_update (SPSelection * selection);
void sp_transformation_select_move_metric (GtkWidget * widget);
void sp_transformation_select_scale_metric (GtkWidget * widget);
void sp_transformation_select_center_metric (GtkWidget * widget);
void sp_transformation_select_skew_metric (GtkWidget * widget);
void sp_transformation_set_move_metric (SPMetric metric);
void sp_transformation_set_scale_metric (SPMetric metric);
void sp_transformation_set_skew_metric (SPMetric metric);
void sp_transformation_set_center_metric (SPMetric metric);
SPMetric sp_transformation_get_move_metric (void);
SPMetric sp_transformation_get_scale_metric (void);
SPMetric sp_transformation_get_skew_metric (void);
SPMetric sp_transformation_get_center_metric (void);
//handlers
static void sp_transformation_selection_changed (Sodipodi * sodipodi, SPSelection * selection);
void sp_transformation_dialog_apply (void);
void sp_transformation_display_position (ArtDRect * bbox, SPMetric metric);
void sp_transformation_display_dimension (ArtDRect * bbox, SPMetric metric);
void sp_transformation_dialog_reset (GtkWidget * widget);
void sp_transformation_set_angle (GtkButton * widget);
void sp_transformation_direction_change (GtkButton * widget);
void sp_transformation_scale_changed (GtkWidget * widget);
void sp_transformation_scale_lock (void);
void sp_transformation_metric_type (GtkButton * widget);
void sp_transformation_fixpoint_toggle (GtkWidget * widget);
void sp_transformation_keep_toggle (GtkWidget * widget);
void sp_transformation_expand_dialog (void);
void sp_transformation_dialog_set_flip (GtkToggleButton * button);
void sp_transformation_notebook_switch (GtkNotebook *notebook,
					GtkNotebookPage *page,
					gint page_num,
					gpointer user_data);
gboolean sp_transformation_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data);
//helpers
ArtPoint * sp_transformation_get_align (SPSelection * selection, ArtPoint * p);
ArtPoint * sp_transformation_get_center (SPSelection * selection, ArtPoint * p);
void sp_transformation_apply_button_reset (void);


void
sp_transformation_dialog_close (void) {
  g_assert (transformation_dialog != NULL);

  gtk_widget_hide (transformation_dialog);
  if (sel_changed_id > 0) {
    gtk_signal_disconnect (GTK_OBJECT (sodipodi), sel_changed_id);
    sel_changed_id = 0;
  }
}

// dialog generation
void 
sp_transformation_dialog (void)
{
  transformation_xml = glade_xml_new (SODIPODI_GLADEDIR "/transformation.glade", NULL, PACKAGE);
  glade_xml_signal_autoconnect (transformation_xml);
  transformation_dialog = glade_xml_get_widget (transformation_xml, "transform_dialog_small");
  
  apply_button = glade_xml_get_widget (transformation_xml, "apply_button");
  rotate_angle = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "rotate_angle");
  skew_value = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "skew_value");
  move_hor = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "move_hor");
  move_ver = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "move_ver");
  scale_hor = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "scale_hor");
  scale_ver = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "scale_ver");
  center_x = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "center_x");
  center_y = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "center_y");
  
  flip_hor = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "flip_hor");
  gtk_button_set_relief (GTK_BUTTON(flip_hor), GTK_RELIEF_NONE);
  flip_ver = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "flip_ver");
  gtk_button_set_relief (GTK_BUTTON(flip_ver), GTK_RELIEF_NONE);
  
  rotate_direction = (GtkButton *) glade_xml_get_widget (transformation_xml, "rotate_direction");
  skew_direction = (GtkButton *) glade_xml_get_widget (transformation_xml, "skew_direction");
  gtk_button_set_relief (rotate_direction, GTK_RELIEF_NONE);
  gtk_button_set_relief (skew_direction, GTK_RELIEF_NONE);
  lock_scale = (GtkButton *) glade_xml_get_widget (transformation_xml, "lock_scale");
  gtk_button_set_relief (lock_scale, GTK_RELIEF_NONE);
  
  expansion = (GtkHBox *) glade_xml_get_widget (transformation_xml, "expansion");
  expand = (GtkButton *) glade_xml_get_widget (transformation_xml, "expand");
  arrow_expand = (GtkArrow *) glade_xml_get_widget (transformation_xml, "arrow_expand");
  
  trans_notebook = (GtkNotebook *) glade_xml_get_widget (transformation_xml, "trans_notebook");
  make_copy = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "make_copy");
  align_tl = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_tl");
  align_tc = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_tc");
  align_tr = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_tr");
  align_cl = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_cl");
  align_cc = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_cc");
  align_cr = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_cr");
  align_bl = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_bl");
  align_bc = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_bc");
  align_br = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "align_br");

  use_align = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "use_align");
  use_center = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "use_center");
  keep_width = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "keep_width");
  keep_height = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "keep_height");
  skew_rotate = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "skew_rotate");
  align_frame = (GtkFrame *) glade_xml_get_widget (transformation_xml, "align_frame");
  center_frame = (GtkFrame *) glade_xml_get_widget (transformation_xml, "center_frame");
  
  move_type = (GtkButton *) glade_xml_get_widget (transformation_xml, "move_type");
  gtk_button_set_relief (move_type, GTK_RELIEF_NONE);
  scale_type = (GtkButton *) glade_xml_get_widget (transformation_xml, "scale_type");
  gtk_button_set_relief (scale_type, GTK_RELIEF_NONE);
  center_type = (GtkButton *) glade_xml_get_widget (transformation_xml, "center_type");
  gtk_button_set_relief (center_type, GTK_RELIEF_NONE);
  
  angle_0 = (GtkButton *) glade_xml_get_widget (transformation_xml, "angle_0");
  gtk_button_set_relief (angle_0, GTK_RELIEF_NONE);
  angle_90 = (GtkButton *) glade_xml_get_widget (transformation_xml, "angle_90");
  gtk_button_set_relief (angle_90, GTK_RELIEF_NONE);
  angle_180 = (GtkButton *) glade_xml_get_widget (transformation_xml, "angle_180");
  gtk_button_set_relief (angle_180, GTK_RELIEF_NONE);
  angle_270 = (GtkButton *) glade_xml_get_widget (transformation_xml, "angle_270");
  gtk_button_set_relief (angle_270, GTK_RELIEF_NONE);
  
  rotate_left = glade_xml_get_widget (transformation_xml, "rotate_left");
  rotate_right = glade_xml_get_widget (transformation_xml, "rotate_right");
  scale_locked = glade_xml_get_widget (transformation_xml, "scale_locked");
  scale_unlocked = glade_xml_get_widget (transformation_xml, "scale_unlocked");
  skew_hor = glade_xml_get_widget (transformation_xml, "skew_hor");
  skew_ver = glade_xml_get_widget (transformation_xml, "skew_ver");
  
  old_x = (GtkLabel *) glade_xml_get_widget (transformation_xml, "old_x");
  old_y = (GtkLabel *) glade_xml_get_widget (transformation_xml, "old_y");
  old_width = (GtkLabel *) glade_xml_get_widget (transformation_xml, "old_width");
  old_height = (GtkLabel *) glade_xml_get_widget (transformation_xml, "old_height");
  // move metrics
  move_metric_om = (GtkOptionMenu *) glade_xml_get_widget (transformation_xml, "move_metric_om");
  move_metrics = glade_xml_get_widget (transformation_xml, "move_metrics");
  gtk_option_menu_set_menu (move_metric_om, move_metrics);
  move_pt = glade_xml_get_widget (transformation_xml, "move_pt");
  move_mm = glade_xml_get_widget (transformation_xml, "move_mm");
  move_cm = glade_xml_get_widget (transformation_xml, "move_cm");
  move_in = glade_xml_get_widget (transformation_xml, "move_in");
  // scale metrics
  scale_metric_om = (GtkOptionMenu *) glade_xml_get_widget (transformation_xml, "scale_metric_om");
  scale_metrics = glade_xml_get_widget (transformation_xml, "scale_metrics");
  gtk_option_menu_set_menu (scale_metric_om, scale_metrics);
  scale_pr = glade_xml_get_widget (transformation_xml, "scale_%");
  scale_pt = glade_xml_get_widget (transformation_xml, "scale_pt");
  scale_mm = glade_xml_get_widget (transformation_xml, "scale_mm");
  scale_cm = glade_xml_get_widget (transformation_xml, "scale_cm");
  scale_in = glade_xml_get_widget (transformation_xml, "scale_in");
  // center metrics
  center_metric_om = (GtkOptionMenu *) glade_xml_get_widget (transformation_xml, "center_metric_om");
  center_metrics = glade_xml_get_widget (transformation_xml, "center_metrics");
  gtk_option_menu_set_menu (center_metric_om, center_metrics);
  center_pt = glade_xml_get_widget (transformation_xml, "center_pt");
  center_mm = glade_xml_get_widget (transformation_xml, "center_mm");
  center_cm = glade_xml_get_widget (transformation_xml, "center_cm");
  center_in = glade_xml_get_widget (transformation_xml, "center_in");
  center_pr = glade_xml_get_widget (transformation_xml, "center_%");
  // skew metrics
  skew_metric_om = (GtkOptionMenu *) glade_xml_get_widget (transformation_xml, "skew_metric_om");
  skew_metrics = glade_xml_get_widget (transformation_xml, "skew_metrics");
  gtk_option_menu_set_menu (skew_metric_om, skew_metrics);
  skew_deg = glade_xml_get_widget (transformation_xml, "skew_deg");
  skew_pt = glade_xml_get_widget (transformation_xml, "skew_pt");
  skew_mm = glade_xml_get_widget (transformation_xml, "skew_mm");
  skew_cm = glade_xml_get_widget (transformation_xml, "skew_cm");
  skew_in = glade_xml_get_widget (transformation_xml, "skew_in");
  sp_transformation_dialog_reset (NULL);
}

/*
 * scale
 */

SPMetric 
sp_transformation_get_scale_metric (void){
  GtkWidget * selected;

  selected = gtk_menu_get_active ((GtkMenu *) scale_metrics);

  if (selected == scale_pr) return NONE;
  if (selected == scale_pt) return SP_PT;
  if (selected == scale_mm) return SP_MM;
  if (selected == scale_cm) return SP_CM;
  if (selected == scale_in) return SP_IN;

  return SP_PT;
}

void
sp_transformation_display_dimension (ArtDRect * bbox, SPMetric metric) {
  GString * str;
  
  if (metric == NONE) metric = SP_DEFAULT_METRIC;
  str = SP_PT_TO_METRIC_STRING (bbox->x1 - bbox->x0, metric); 
  gtk_label_set (old_width, str->str);
  g_string_free (str, TRUE);
  str = SP_PT_TO_METRIC_STRING (bbox->y1 - bbox->y0, metric);
  gtk_label_set (old_height, str->str);
  g_string_free (str, TRUE);
}

void
sp_transformation_select_scale_metric (GtkWidget * widget) {
  SPDesktop * desktop;
  SPSelection * selection;
  ArtDRect  bbox;
  SPMetric metric;

  desktop = SP_ACTIVE_DESKTOP;
  if (!SP_IS_DESKTOP (desktop)) return;
  selection = SP_DT_SELECTION (desktop);

  if (!sp_selection_is_empty (selection)) {
    sp_selection_bbox (selection, &bbox);
    metric = sp_transformation_get_scale_metric ();
    sp_transformation_display_dimension (&bbox, metric);
  }
}

void
sp_transformation_set_scale_metric (SPMetric metric){

  if (metric == SP_PT) gtk_option_menu_set_history (scale_metric_om, 0);
  if (metric == SP_MM) gtk_option_menu_set_history (scale_metric_om, 1);
  if (metric == SP_CM) gtk_option_menu_set_history (scale_metric_om, 2);
  if (metric == SP_IN) gtk_option_menu_set_history (scale_metric_om, 3);
  if (metric == NONE) gtk_option_menu_set_history (scale_metric_om, 4);
  sp_transformation_select_scale_metric (NULL);
}

void
sp_transformation_scale_update (SPSelection * selection) {
  ArtDRect  bbox;
  SPMetric metric;

  g_assert (transformation_dialog != NULL);
  
  if (SP_IS_SELECTION (selection)) {
    if (!sp_selection_is_empty (selection)) {
      sp_selection_bbox (selection, &bbox);
      metric = sp_transformation_get_scale_metric ();
      
      sp_transformation_display_dimension (&bbox, metric);
      return;
    }
  }
  gtk_label_set (old_width, "");
  gtk_label_set (old_height, "");
}

void
sp_transformation_apply_scale (SPSelection * selection) {
  double dx, dy, ax,ay;
  ArtPoint p;
  ArtDRect bbox;
  SPMetric metric;

  g_assert (transformation_dialog != NULL);
  g_assert (!sp_selection_is_empty (selection));

  metric = sp_transformation_get_scale_metric ();
  sp_selection_bbox (selection, &bbox);

  if (metric == NONE) {
    dx = gtk_spin_button_get_value_as_float (scale_hor) / 100.0 ;
    dy = gtk_spin_button_get_value_as_float (scale_ver) / 100.0 ;
  } else {
    if (fabs(bbox.x1-bbox.x0)<1e-15 || fabs(bbox.y1-bbox.y0)<1e-15) return;
    ax = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (scale_hor), metric);
    dx = ax / fabs(bbox.x1-bbox.x0);
    ay = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (scale_ver), metric);
    dy = ay / fabs(bbox.y1-bbox.y0);
  }

  if (tr_scale_type == RELATIVE) {
    dx += 1;
    dy += 1;
  }

  p.x = (bbox.x0 + bbox.x1)/2;
  p.y = (bbox.y0 + bbox.y1)/2;

  if (GTK_WIDGET_VISIBLE (expansion)) {
    if (gtk_toggle_button_get_active (use_align)) sp_transformation_get_align (selection,&p);
    if (gtk_toggle_button_get_active (use_center)) sp_transformation_get_center (selection, &p);
  } 

  if (gtk_toggle_button_get_active (flip_hor)) dx = -1;
  if (gtk_toggle_button_get_active (flip_ver)) dy = -1;
 
  if ((dx < 1e-15) || (dy < 1e-15)) return;
  sp_selection_scale_relative (selection, &p, dx, dy);
}


/*
 * rotate
 */

void
sp_transformation_rotate_update (SPSelection * selection) {
  g_assert (transformation_dialog != NULL);
}

void
sp_transformation_apply_rotate (SPSelection * selection) {
  ArtPoint bp, ap, p;
  double angle, dx=0, dy=0;
  ArtDRect bbox, bbox2;

  g_assert (transformation_dialog != NULL);
  g_assert (!sp_selection_is_empty (selection));

  //angle
  angle = gtk_spin_button_get_value_as_float (rotate_angle);
  if (GTK_WIDGET_VISIBLE (rotate_right)) angle = -angle;
  // before + default
  sp_selection_bbox (selection, &bbox);
  sp_transformation_get_align (selection,&bp);
  p.x = (bbox.x0 + bbox.x1)/2;
  p.y = (bbox.y0 + bbox.y1)/2;
  // explicit center
  if (GTK_WIDGET_VISIBLE (expansion) && gtk_toggle_button_get_active (use_center)) sp_transformation_get_center (selection,&p);
  // rotate
  sp_selection_rotate_relative (selection, &p, angle);
  //after
  sp_selection_bbox (selection, &bbox2);
  // align
  if (GTK_WIDGET_VISIBLE (expansion) && gtk_toggle_button_get_active (use_align)) {
    sp_transformation_get_align (selection,&ap);
    p.x = bp.x;
    p.y = bp.y;
    dx = bp.x-ap.x;
    dy = bp.y-ap.y;
    sp_selection_move_relative (selection, dx, dy);
  }
  // keep width/height
  if (GTK_WIDGET_VISIBLE (expansion) && (gtk_toggle_button_get_active (keep_width) || 
					 gtk_toggle_button_get_active (keep_height))) {
    if (fabs(bbox2.x1-bbox2.x0)<1e-15 || fabs(bbox2.y1-bbox2.y0)<1e-15) return;
    if (gtk_toggle_button_get_active (keep_width)) dx = dy =fabs(bbox.x1-bbox.x0) / fabs(bbox2.x1-bbox2.x0);
    if (gtk_toggle_button_get_active (keep_height)) dx = dy = fabs(bbox.y1-bbox.y0) / fabs(bbox2.y1-bbox2.y0);

    if (dx < 1e-15 || dy < 1e-15) return;
    sp_selection_scale_relative (selection, &p, dx, dy);
  }
}

/*
 * skew
 */

void
sp_transformation_select_skew_metric (GtkWidget * widget) {
}

void
sp_transformation_set_skew_metric (SPMetric metric){

  if (metric == SP_PT) gtk_option_menu_set_history (skew_metric_om, 0);
  if (metric == SP_MM) gtk_option_menu_set_history (skew_metric_om, 1);
  if (metric == SP_CM) gtk_option_menu_set_history (skew_metric_om, 2);
  if (metric == SP_IN) gtk_option_menu_set_history (skew_metric_om, 3);
  if (metric == NONE) gtk_option_menu_set_history (skew_metric_om, 4);
  sp_transformation_select_skew_metric (NULL);
}

SPMetric 
sp_transformation_get_skew_metric (void){
  GtkWidget * selected;

  selected = gtk_menu_get_active ((GtkMenu *) skew_metrics);

  if (selected == skew_deg) return NONE;
  if (selected == skew_pt) return SP_PT;
  if (selected == skew_mm) return SP_MM;
  if (selected == skew_cm) return SP_CM;
  if (selected == skew_in) return SP_IN;

  return SP_PT;
}

void
sp_transformation_skew_update (SPSelection * selection) {
  g_assert (transformation_dialog != NULL);

}

void
sp_transformation_apply_skew (SPSelection * selection) {
  ArtDRect bbox, bbox2;
  ArtPoint p, bp, ap;
  double dx=0, dy=0, a, b=0, first, snd=0;
  SPMetric metric;

  g_assert (transformation_dialog != NULL);
  g_assert (!sp_selection_is_empty (selection));

  // before + default
  sp_selection_bbox (selection, &bbox);
  sp_transformation_get_align (selection,&bp);
  p.x = (bbox.x0 + bbox.x1)/2;
  p.y = (bbox.y0 + bbox.y1)/2;
  // explicit center
  if (GTK_WIDGET_VISIBLE (expansion) && gtk_toggle_button_get_active (use_center)) sp_transformation_get_center (selection,&p);

  // skew
  metric = sp_transformation_get_skew_metric ();
  a = gtk_spin_button_get_value_as_float (skew_value);
  snd = 0;
  if (!gtk_toggle_button_get_active (skew_rotate)) { // no aspect
    if (metric == NONE) { 
      if (fabs((fabs( remainder(a,180))-90 ))  < 1e-10) return;
      first = tan (M_PI*a/180);
    } else {
      if (fabs(bbox.x1-bbox.x0)<1e-15 || fabs(bbox.y1-bbox.y0)<1e-15) return;
      a = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (skew_value), metric);
      if (GTK_WIDGET_VISIBLE (skew_hor)) first = a / fabs(bbox.y1-bbox.y0);
      else first = a / fabs(bbox.x1-bbox.x0);
    }
  } else { // keep aspect
    if (metric == NONE) { 
      b = M_PI * a / 180;
    } else {
      if (fabs(bbox.x1-bbox.x0)<1e-15 || fabs(bbox.y1-bbox.y0)<1e-15) return;
      a = SP_METRIC_TO_PT (a, metric);
      if (GTK_WIDGET_VISIBLE (skew_hor)) b = M_PI * (a / fabs(bbox.y1-bbox.y0))/2;
      else b = M_PI * (a / fabs(bbox.x1-bbox.x0))/2;
    }
    first = sin (b);
    if (GTK_WIDGET_VISIBLE (skew_hor));  
  }
 
  if (GTK_WIDGET_VISIBLE (skew_hor)) sp_selection_skew_relative (selection, &p, first, snd);
  else  sp_selection_skew_relative (selection, &p, snd, first);

  if (gtk_toggle_button_get_active (skew_rotate)) { // keep aspect
    first = cos (b);
    snd = 1;
    if (GTK_WIDGET_VISIBLE (skew_hor)) sp_selection_scale_relative (selection, &p, snd, first);
    else  sp_selection_scale_relative (selection, &p, first, snd);
  }

  //after
  sp_selection_bbox (selection, &bbox2);
  // align
  if (GTK_WIDGET_VISIBLE (expansion) && gtk_toggle_button_get_active (use_align)) {
    sp_transformation_get_align (selection,&ap);
    p.x = bp.x;
    p.y = bp.y;
    dx = bp.x-ap.x;
    dy = bp.y-ap.y;
    sp_selection_move_relative (selection, dx, dy);
  }
  // keep width/height
  if (GTK_WIDGET_VISIBLE (expansion) && (gtk_toggle_button_get_active (keep_width) || 
					 gtk_toggle_button_get_active (keep_height))) {
    if (fabs(bbox2.x1-bbox2.x0)<1e-15 || fabs(bbox2.y1-bbox2.y0)<1e-15) return;
    if (gtk_toggle_button_get_active (keep_width)) dx = dy =fabs(bbox.x1-bbox.x0) / fabs(bbox2.x1-bbox2.x0);
    if (gtk_toggle_button_get_active (keep_height)) dx = dy = fabs(bbox.y1-bbox.y0) / fabs(bbox2.y1-bbox2.y0);
    if (dx < 1e-15 || dy < 1e-15) return;
    sp_selection_scale_relative (selection, &p, dx, dy);
  }
}

/*
 * center
 */

void
sp_transformation_select_center_metric (GtkWidget * widget) {
}

void
sp_transformation_set_center_metric (SPMetric metric){

  if (metric == SP_PT) gtk_option_menu_set_history (center_metric_om, 0);
  if (metric == SP_MM) gtk_option_menu_set_history (center_metric_om, 1);
  if (metric == SP_CM) gtk_option_menu_set_history (center_metric_om, 2);
  if (metric == SP_IN) gtk_option_menu_set_history (center_metric_om, 3);
}

SPMetric 
sp_transformation_get_center_metric (void){
  GtkWidget * selected;

  selected = gtk_menu_get_active ((GtkMenu *) center_metrics);

  if (selected == center_pr) return NONE;
  if (selected == center_pt) return SP_PT;
  if (selected == center_mm) return SP_MM;
  if (selected == center_cm) return SP_CM;
  if (selected == center_in) return SP_IN;

  return SP_PT;
}


void
sp_transformation_dialog_apply (void)
{
  SPDesktop * desktop;
  SPSelection * selection;
  gint page;

  g_assert (transformation_dialog != NULL);
  desktop = SP_ACTIVE_DESKTOP;
  g_return_if_fail (desktop != NULL);
  selection = SP_DT_SELECTION(desktop);
  g_return_if_fail (!sp_selection_is_empty (selection));

  // sp_selection_duplicate updates undo too, thus we get 2 undo entries. This should be fixed for sane undo list!
  if (gtk_toggle_button_get_active (make_copy)) sp_selection_duplicate ((GtkWidget *) desktop);

  page = gtk_notebook_get_current_page(trans_notebook);
    switch (page) {
  case 0 : // move
    sp_transformation_apply_move (selection);
    break;
  case 2 : // rotate
    sp_transformation_apply_rotate (selection);
    break;
  case 1 : // scale
    sp_transformation_apply_scale (selection);
    break;
  case 3 : // skew
    sp_transformation_apply_skew (selection);
    break;
  }

    // update handels and undo
  sp_selection_changed (selection);
  sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_transformation_dialog_set_flip (GtkToggleButton * button) {
  if (button == flip_hor) {
    if (gtk_toggle_button_get_active (flip_hor)) 
      gtk_widget_set_sensitive((GtkWidget *)scale_hor,0);
    else gtk_widget_set_sensitive((GtkWidget *)scale_hor,1);
  }
  if (button == flip_ver) {
    if (gtk_toggle_button_get_active (flip_ver)) 
      gtk_widget_set_sensitive((GtkWidget *)scale_ver,0);
    else gtk_widget_set_sensitive((GtkWidget *)scale_ver,1);
  }
}

void
sp_transformation_expand_dialog (void) {

  if (GTK_WIDGET_VISIBLE ((GtkWidget *) expansion)) {
    gtk_widget_hide (GTK_WIDGET (expansion));
    gtk_arrow_set (arrow_expand, GTK_ARROW_RIGHT, GTK_SHADOW_ETCHED_IN);
  } else {
    gtk_widget_show (GTK_WIDGET (expansion));
    gtk_arrow_set (arrow_expand, GTK_ARROW_LEFT, GTK_SHADOW_ETCHED_IN);
  }
}

void
sp_transformation_fixpoint_toggle (GtkWidget * widget) {
  GtkToggleButton * button;

  button = (GtkToggleButton *) widget;
  if (gtk_toggle_button_get_active (button)) {
    if (button == use_align) {
      // use_align was pressed
      gtk_widget_set_sensitive (GTK_WIDGET (align_frame), TRUE);
      if (gtk_toggle_button_get_active (use_center)) {
	gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
	gtk_toggle_button_set_active (use_center,FALSE);
      }
    } else {
      // use center was pressed
      gtk_widget_set_sensitive (GTK_WIDGET (center_frame), TRUE);
      if (gtk_toggle_button_get_active (use_align)) {
	gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
	gtk_toggle_button_set_active (use_align,FALSE);
      }
    };
  } else {
    if (button == use_align) gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
    else gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
  }
}

void
sp_transformation_keep_toggle (GtkWidget * widget) {
  GtkToggleButton * button;

  button = (GtkToggleButton *) widget;
  if (gtk_toggle_button_get_active (button)) {
    if (button == keep_width) gtk_toggle_button_set_active (keep_height,FALSE);
    else gtk_toggle_button_set_active (keep_width,FALSE);
  }
}

void
sp_transformation_notebook_switch (GtkNotebook *notebook,
				   GtkNotebookPage *page,
				   gint page_num,
				   gpointer user_data) {
  SPDesktop * desktop;
  SPSelection * selection;

  desktop = SP_ACTIVE_DESKTOP;
  if (SP_IS_DESKTOP (desktop)) selection = SP_DT_SELECTION (desktop);
  else selection = NULL;
  
  switch (page_num) {
  case 0:
    gtk_widget_set_sensitive (GTK_WIDGET (keep_width), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (keep_height), FALSE);
    if (tr_move_type == RELATIVE) {
      gtk_widget_set_sensitive (GTK_WIDGET (use_center), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (use_align), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
    } else {
      gtk_widget_set_sensitive (GTK_WIDGET (use_align), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (use_center), TRUE);
      if (gtk_toggle_button_get_active (use_center)) gtk_widget_set_sensitive (GTK_WIDGET (center_frame), TRUE);
      else gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
      if (gtk_toggle_button_get_active (use_align)) gtk_widget_set_sensitive (GTK_WIDGET (align_frame), TRUE);
      else gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
    }
    sp_transformation_move_update (selection);
    break;
  case 1:
    gtk_widget_set_sensitive (GTK_WIDGET (keep_width), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (keep_height), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (use_center), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (use_align), TRUE);
    if (gtk_toggle_button_get_active (use_center)) gtk_widget_set_sensitive (GTK_WIDGET (center_frame), TRUE);
    else gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
    if (gtk_toggle_button_get_active (use_align)) gtk_widget_set_sensitive (GTK_WIDGET (align_frame), TRUE);
    else gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);

    sp_transformation_scale_update (selection);
    break;
  case 2:
    gtk_widget_set_sensitive (GTK_WIDGET (keep_width), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (keep_height), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (use_center), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (use_align), TRUE);
    if (gtk_toggle_button_get_active (use_center)) gtk_widget_set_sensitive (GTK_WIDGET (center_frame), TRUE);
    else gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
    if (gtk_toggle_button_get_active (use_align)) gtk_widget_set_sensitive (GTK_WIDGET (align_frame), TRUE);
    else gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);

    sp_transformation_rotate_update (selection);
    break;
  case 3:
    gtk_widget_set_sensitive (GTK_WIDGET (keep_width), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (keep_height), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (use_center), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (use_align), TRUE);
    if (gtk_toggle_button_get_active (use_center)) gtk_widget_set_sensitive (GTK_WIDGET (center_frame), TRUE);
    else gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
    if (gtk_toggle_button_get_active (use_align)) gtk_widget_set_sensitive (GTK_WIDGET (align_frame), TRUE);
    else gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);

    sp_transformation_skew_update (selection);
    break;
  }
}

void
sp_transformation_metric_type (GtkButton * widget) {
  if (widget == move_type) {
    if (tr_move_type == ABSOLUTE) {
      gtk_object_set(GTK_OBJECT(move_type), 
                  "label", "relative",
                  NULL);
      tr_move_type = RELATIVE;
      gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (use_align), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (use_center), FALSE);
    } else {
      gtk_object_set(GTK_OBJECT(move_type), 
                  "label", "absolute",
                  NULL);
      tr_move_type = ABSOLUTE;
      gtk_widget_set_sensitive (GTK_WIDGET (use_align), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (use_center), TRUE);
      if (gtk_toggle_button_get_active (use_center)) gtk_widget_set_sensitive (GTK_WIDGET (center_frame), TRUE);
      else gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
      if (gtk_toggle_button_get_active (use_align)) gtk_widget_set_sensitive (GTK_WIDGET (align_frame), TRUE);
      else gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
    }
  }
  if (widget == scale_type) {
    if (tr_scale_type == ABSOLUTE) {
      gtk_object_set(GTK_OBJECT(scale_type), 
                  "label", "relative",
                  NULL);
      tr_scale_type = RELATIVE;
    } else {
      gtk_object_set(GTK_OBJECT(scale_type), 
                  "label", "absolute",
                  NULL);
      tr_scale_type = ABSOLUTE;
    }
  }
  if (widget == center_type) {
    if (tr_center_type == SELECTION) {
      gtk_object_set(GTK_OBJECT(center_type), 
                  "label", "desktop",
                  NULL);
      tr_center_type = DESKTOP;
      gtk_widget_hide (center_pr);
      if (sp_transformation_get_center_metric () == NONE) sp_transformation_set_center_metric (SP_DEFAULT_METRIC);
    } else {
      gtk_object_set(GTK_OBJECT(center_type), 
                  "label", "selection",
                  NULL);
      tr_center_type = SELECTION;
      gtk_widget_show (center_pr);
    }
  }
}

void 
sp_transformation_scale_lock (void) {
  gdouble value;
  if (GTK_WIDGET_VISIBLE (scale_unlocked)) {
    gtk_widget_hide (scale_unlocked);
    gtk_widget_show (scale_locked);
    value = gtk_spin_button_get_value_as_float (scale_hor);
    gtk_spin_button_set_value (scale_ver, value);
  } else {
    gtk_widget_hide (scale_locked);
    gtk_widget_show (scale_unlocked);
  }
}

void 
sp_transformation_direction_change (GtkButton * widget) {
  if (widget == rotate_direction) {
    if (GTK_WIDGET_VISIBLE (rotate_left)) {
      gtk_widget_hide (rotate_left);
      gtk_widget_show (rotate_right);
    } else {
      gtk_widget_hide (rotate_right);
      gtk_widget_show (rotate_left);
    }
  }
  if (widget == skew_direction) {
    if (GTK_WIDGET_VISIBLE (skew_hor)) {
      gtk_widget_hide (skew_hor);
      gtk_widget_show (skew_ver);
    } else {
      gtk_widget_hide (skew_ver);
      gtk_widget_show (skew_hor);
    }
  }
}

void
sp_transformation_set_angle (GtkButton * widget) {
  if (widget == angle_0) gtk_spin_button_set_value (rotate_angle, 0.0);
  if (widget == angle_90) gtk_spin_button_set_value (rotate_angle, 90.0);
  if (widget == angle_180) gtk_spin_button_set_value (rotate_angle, 180.0);
  if (widget == angle_270) gtk_spin_button_set_value (rotate_angle, 270.0);
}

void
sp_transformation_apply_button_reset (void) {
  SPDesktop * desktop;
  SPSelection * selection;

  desktop = SP_ACTIVE_DESKTOP;
  if (desktop != NULL) {
    selection = SP_DT_SELECTION(desktop);
    if (!sp_selection_is_empty (selection)) { 
      gtk_widget_set_sensitive (GTK_WIDGET (apply_button), TRUE);
      return;
    }
  }
  gtk_widget_set_sensitive (GTK_WIDGET (apply_button), FALSE);
}

void
sp_transformation_dialog_reset (GtkWidget * widget) {
  //  gint page;

  g_assert (transformation_dialog != NULL);

  gtk_toggle_button_set_active (align_cc, TRUE);
  gtk_toggle_button_set_active (use_align, FALSE);
  gtk_toggle_button_set_active (use_center, FALSE);
  gtk_toggle_button_set_active (keep_width, FALSE);
  gtk_toggle_button_set_active (keep_height, FALSE);
  gtk_toggle_button_set_active (make_copy, FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (align_frame), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (center_frame), FALSE);
  gtk_object_set(GTK_OBJECT(center_type), 
		 "label", "selection",
		 NULL);
  gtk_spin_button_set_value (center_x, 0.0);
  gtk_spin_button_set_value (center_y, 0.0);
  //  gtk_widget_hide (center_pr);
  tr_center_type = SELECTION;
  sp_transformation_set_center_metric (SP_DEFAULT_METRIC);
  /*
  page = gtk_notebook_get_current_page(trans_notebook);
  switch (page) {
  case 0:
  */
    gtk_spin_button_set_value (move_hor, 0.0);
    gtk_spin_button_set_value (move_ver, 0.0);
    gtk_object_set(GTK_OBJECT(move_type), 
		   "label", "relative",
		   NULL);
    tr_move_type = RELATIVE;
    sp_transformation_set_move_metric (SP_DEFAULT_METRIC);
    /*
    break;
  case 1:
    */
    gtk_spin_button_set_value (scale_hor, 0.0);
    gtk_spin_button_set_value (scale_ver, 0.0);
    gtk_toggle_button_set_active (flip_hor, FALSE);
    gtk_toggle_button_set_active (flip_hor, FALSE);
    gtk_widget_hide (scale_unlocked);
    gtk_widget_show (scale_locked);
    gtk_object_set(GTK_OBJECT(scale_type), 
		   "label", "relative",
		   NULL);
    tr_scale_type = RELATIVE;
    sp_transformation_set_scale_metric (SP_DEFAULT_METRIC);
    /* 
   break;
  case 2:
    */
    gtk_spin_button_set_value (rotate_angle, 0.0);
    gtk_widget_hide (rotate_left);
    gtk_widget_show (rotate_right);
    /*
    break;
  case 3:
    */
    gtk_spin_button_set_value (skew_value, 0.0);
    gtk_widget_hide (skew_ver);
    gtk_widget_show (skew_hor);
    sp_transformation_set_skew_metric (SP_DEFAULT_METRIC);
    gtk_toggle_button_set_active (skew_rotate, FALSE);
    /*
    break;
  }
    */
}

void
sp_transformation_scale_changed (GtkWidget * widget) {
  gdouble value;

  if (GTK_WIDGET_VISIBLE (GTK_WIDGET (scale_locked))) {
    if (widget == GTK_WIDGET (scale_hor)) {
      value = gtk_spin_button_get_value_as_float (scale_hor);
      gtk_spin_button_set_value (scale_ver, value);
    } 
    if (widget == GTK_WIDGET (scale_ver)) {
      value = gtk_spin_button_get_value_as_float (scale_ver);
      gtk_spin_button_set_value (scale_hor, value);
    } 
  }
}


/*
 * key bindings for transformation dialog
 */

gboolean
sp_transformation_key_press (GtkWidget *widget,
			     GdkEventKey *event,
			     gpointer user_data) {

  g_assert (transformation_dialog != NULL);

  if (event->state & GDK_CONTROL_MASK) switch (event->keyval) {
  case 49:
  case 50:
  case 51:
  case 52:
    gtk_notebook_set_page (trans_notebook, event->keyval-49);
    break;
  case 114:
    sp_transformation_dialog_reset (NULL);
    break;
  case 101:
    sp_transformation_expand_dialog ();
    break;
  case 97:
    sp_transformation_dialog_apply ();
    break;
  case 99:
    sp_transformation_dialog_close ();
    break;
  case 109:
    if (gtk_toggle_button_get_active (make_copy)) gtk_toggle_button_set_active (make_copy, FALSE);
    else gtk_toggle_button_set_active (make_copy, TRUE);
    break;
  }

  return TRUE;
};

/*
 * helpers
 */

ArtPoint *
sp_transformation_get_align (SPSelection * selection, ArtPoint * p) {
  ArtDRect  bbox;

  g_assert (SP_IS_SELECTION (selection));
  sp_selection_bbox (selection, &bbox);

  if (gtk_toggle_button_get_active (align_tl)) { p->x = bbox.x0; p->y = bbox.y1; }
  if (gtk_toggle_button_get_active (align_tc)) { p->x = (bbox.x0+bbox.x1)/2; p->y = bbox.y1; }
  if (gtk_toggle_button_get_active (align_tr)) { p->x = bbox.x1; p->y = bbox.y1; }
  if (gtk_toggle_button_get_active (align_cl)) { p->x = bbox.x0; p->y = (bbox.y0+bbox.y1)/2; }
  if (gtk_toggle_button_get_active (align_cc)) { p->x = (bbox.x0+bbox.x1)/2; p->y = (bbox.y0+bbox.y1)/2; }
  if (gtk_toggle_button_get_active (align_cr)) { p->x = bbox.x1; p->y = (bbox.y0+bbox.y1)/2; }
  if (gtk_toggle_button_get_active (align_bl)) { p->x = bbox.x0; p->y = bbox.y0; }
  if (gtk_toggle_button_get_active (align_bc)) { p->x = (bbox.x0+bbox.x1)/2; p->y = bbox.y0; }
  if (gtk_toggle_button_get_active (align_br)) { p->x = bbox.x1; p->y = bbox.y0; }

  return p;
}

ArtPoint *
sp_transformation_get_center (SPSelection * selection, ArtPoint * p) {
  ArtDRect  bbox;
  SPMetric metric;

  g_assert (SP_IS_SELECTION (selection));
  sp_selection_bbox (selection, &bbox);

  metric = sp_transformation_get_center_metric ();

  if (tr_center_type == DESKTOP) {
    p->x = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (center_x), metric);
    p->y = SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (center_y), metric);
  } else {
    // center is relative to selection
    if (metric == NONE) {
      p->x = bbox.x0 + fabs(bbox.x1 - bbox.x0) * gtk_spin_button_get_value_as_float (center_x)/100;
      p->y = bbox.y0 + fabs(bbox.y1 - bbox.y0) * gtk_spin_button_get_value_as_float (center_y)/100;
    } else {
      p->x = bbox.x0 + SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (center_x), metric);
      p->y = bbox.y0 + SP_METRIC_TO_PT (gtk_spin_button_get_value_as_float (center_y), metric);
    }
  }
  return p;
}


#endif
