#define SP_TRANSFORMATION_C

#include "align.h"
#include <math.h>
#include <glade/glade.h>
#include "transformation.h"
#include "../sodipodi.h"
#include "../desktop-handles.h"
#include "../svg/svg.h"
#include "../selection-chemistry.h"
#include "../sp-item-transform.h"

/*
 * handler functions for transformation dialog
 *
 * todo: 
 * - add aligns to rotate, scale, skew
 * - more transformations
 */ 

GladeXML  * transformation_xml = NULL;
GtkWidget * transformation_dialog = NULL;

GtkSpinButton * rotate_angle;
GtkSpinButton * move_hor;
GtkSpinButton * move_ver;
GtkSpinButton * scale_hor;
GtkSpinButton * scale_ver;
GtkSpinButton * skew_hor;
GtkSpinButton * skew_ver;
GtkToggleButton * flip_hor;
GtkToggleButton * flip_ver;
GtkToggleButton * rotate_any;
GtkToggleButton * rotate_90;
GtkToggleButton * rotate_180;
GtkToggleButton * rotate_270;
GtkToggleButton * rotate_right;
GtkToggleButton * rotate_left;
GtkNotebook * trans_notebook;
GtkToggleButton * make_copy;


void
sp_transformation_dialog (void)
{
	if (transformation_dialog == NULL) {
		transformation_xml = glade_xml_new (SODIPODI_GLADEDIR "/transformation.glade", "transform_dialog_small");
		glade_xml_signal_autoconnect (transformation_xml);
		transformation_dialog = glade_xml_get_widget (transformation_xml, "transform_dialog_small");
		
		rotate_angle = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "rotate_angle");
		move_hor = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "move_hor");
		move_ver = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "move_ver");
		scale_hor = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "scale_hor");
		scale_ver = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "scale_ver");
		skew_hor = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "skew_hor");
		skew_ver = (GtkSpinButton *) glade_xml_get_widget (transformation_xml, "skew_ver");
		flip_hor = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "flip_hor");
		flip_ver = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "flip_ver");
		rotate_any = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "rotate_any");
		rotate_90 = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "rotate_90");
		rotate_180 = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "rotate_180");
		rotate_270 = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "rotate_270");
		rotate_left = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "rotate_left");
		rotate_right = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "rotate_right");
		trans_notebook = (GtkNotebook *) glade_xml_get_widget (transformation_xml, "trans_notebook");
		make_copy = (GtkToggleButton *) glade_xml_get_widget (transformation_xml, "make_copy");

	} else {
		if (!GTK_WIDGET_VISIBLE (transformation_dialog))
			gtk_widget_show (transformation_dialog);
	}
}

void
sp_transformation_dialog_close (void)
{
	g_assert (transformation_dialog != NULL);

	if (GTK_WIDGET_VISIBLE (transformation_dialog))
		gtk_widget_hide (transformation_dialog);
}


void
sp_transformation_dialog_apply (void)
{
  SPDesktop * desktop;
  SPSelection * selection;
  SPItem * item;
  GSList * l, * l2;
  double dx, dy, angle = 0;
  gint page;

  g_assert (transformation_dialog != NULL);

  desktop = SP_ACTIVE_DESKTOP;
  g_return_if_fail (desktop != NULL);

  // hm, sp_selection_duplicate updates undo itself, this should be fixed
  if (gtk_toggle_button_get_active (make_copy)) sp_selection_duplicate ((GtkWidget *) desktop);

  desktop = SP_ACTIVE_DESKTOP;
  g_return_if_fail (desktop != NULL);
  selection = SP_DT_SELECTION(desktop);
  if sp_selection_is_empty(selection) return;
  l = selection->items;  

  page = gtk_notebook_get_current_page(trans_notebook);
    switch (page) {
  case 0 : // move
    dx = gtk_spin_button_get_value_as_float (move_hor);
    dy = gtk_spin_button_get_value_as_float (move_ver);

    for (l2 = l; l2 != NULL; l2 = l2-> next) {
      item = SP_ITEM (l2->data);
      sp_item_move_rel (item,dx,dy);
    }
    break;
  case 1 : // rotate
    if (gtk_toggle_button_get_active (rotate_any)) angle = gtk_spin_button_get_value_as_float (rotate_angle);
    else if (gtk_toggle_button_get_active (rotate_90)) angle = 90;
    else if (gtk_toggle_button_get_active (rotate_180)) angle = 180;
    else if (gtk_toggle_button_get_active (rotate_270)) angle = 270;					      
    if (gtk_toggle_button_get_active (rotate_right)) angle = -angle;

    for (l2 = l; l2 != NULL; l2 = l2-> next) {
      item = SP_ITEM (l2->data);
      sp_item_rotate_rel (item,angle);
    }
    break;
  case 2 : // scale
    if (gtk_toggle_button_get_active (flip_hor)) dx = -1;
    else dx = gtk_spin_button_get_value_as_float (scale_hor)/100 +1;
    if (gtk_toggle_button_get_active (flip_ver)) dy = -1;
    else dy = gtk_spin_button_get_value_as_float (scale_ver)/100 +1;

    for (l2 = l; l2 != NULL; l2 = l2-> next) {
      item = SP_ITEM (l2->data);
      sp_item_scale_rel (item,dx,dy);
    }
    break;
  case 3 : // skew
    dx = gtk_spin_button_get_value_as_float (skew_hor)/100;
    dy = gtk_spin_button_get_value_as_float (skew_ver)/100;

    for (l2 = l; l2 != NULL; l2 = l2-> next) {
      item = SP_ITEM (l2->data);
      sp_item_skew_rel (item,dx,dy);
    }
    break;
  }

    // update handels and undo
  sp_selection_changed (selection);
  sp_document_done (SP_DT_DOCUMENT (desktop));


}

void
sp_transformation_dialog_ok (void)
{
	g_assert (transformation_dialog != NULL);

	sp_transformation_dialog_apply ();
	sp_transformation_dialog_close ();
}

void
set_hor_flip (void)
{
  if (gtk_toggle_button_get_active (flip_hor)) 
    gtk_widget_set_sensitive((GtkWidget *)scale_hor,0);
  else gtk_widget_set_sensitive((GtkWidget *)scale_hor,1);
}

void
set_ver_flip (void)
{
  if (gtk_toggle_button_get_active (flip_ver)) 
    gtk_widget_set_sensitive((GtkWidget *)scale_ver,0);
  else gtk_widget_set_sensitive((GtkWidget *)scale_ver,1);
}

void
set_any_angle (void)
{
  if (gtk_toggle_button_get_active (rotate_any)) 
    gtk_widget_set_sensitive((GtkWidget *)rotate_angle,1);
  else gtk_widget_set_sensitive((GtkWidget *)rotate_angle,0);
}


