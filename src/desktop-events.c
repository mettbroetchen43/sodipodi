#define SP_DESKTOP_EVENTS_C

#include "helper/sp-guide.h"
#include "helper/unit-menu.h"
#include "sodipodi-private.h"
#include "desktop.h"
#include "document.h"
#include "sp-guide.h"
#include "sp-namedview.h"
#include "desktop-affine.h"
#include "desktop-handles.h"
#include "event-context.h"
#include "sp-metrics.h"
#include "sp-item.h"
#include "desktop-events.h"
#include "gnome.h"

static void sp_dt_simple_guide_dialog (SPGuide * guide, SPDesktop * desktop);


/* Root item handler */


void
sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, SPDesktop * desktop)
{
	if (event->type == GDK_ENTER_NOTIFY) {
		/* fixme: should it go here? 
		   moved it to sp_desktop_set_focus (Frank)
	  gnome_canvas_item_grab_focus ((GnomeCanvasItem *) desktop->main); 
		*/
	}
	sp_event_context_root_handler (desktop->event_context, event);
}

/*
 * fixme: this conatins a hack, to deal with deleting a view, which is
 * completely on another view, in which case active_desktop will not be updated
 *
 */

void
sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	gpointer ddata;
	SPDesktop * desktop;

	ddata = gtk_object_get_data (GTK_OBJECT (item->canvas), "SPDesktop");
	g_return_if_fail (ddata != NULL);

	desktop = SP_DESKTOP (ddata);

	sp_event_context_item_handler (desktop->event_context, SP_ITEM (data), event);
}


// not needed 
gint
sp_desktop_enter_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  	sodipodi_activate_desktop (SP_DESKTOP (widget));
	return FALSE;
}

static gint
sp_dt_ruler_event (GtkWidget * widget, GdkEvent * event, gpointer data, gboolean horiz)
{
	static gboolean dragging = FALSE;
	static GnomeCanvasItem * guide = NULL;
	static ArtPoint p;
	SPDesktopWidget *dtw;
	SPDesktop *desktop;

	dtw = SP_DESKTOP_WIDGET (data);
	desktop = dtw->desktop;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			guide = gnome_canvas_item_new (desktop->guides, SP_TYPE_GUIDELINE,
						       "orientation",
						       horiz ? SP_GUIDELINE_ORIENTATION_HORIZONTAL : SP_GUIDELINE_ORIENTATION_VERTICAL,
						       "color", desktop->namedview->guidehicolor,
						       NULL);
			gdk_pointer_grab (widget->window, FALSE,
					  GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
					  NULL, NULL,
					  event->button.time);
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging) {
		    /* we have to substract (x|y)thickness to the position 
		     * since there is a frame between ruler and canvas */
			gnome_canvas_window_to_world (desktop->owner->canvas,
						      event->motion.x - 
						      (horiz ? 0//-widget->style->klass->xthickness
						             : widget->allocation.width + 
						               widget->style->klass->xthickness),
						      event->motion.y - 
						      (horiz ? widget->allocation.height +
						               widget->style->klass->ythickness 
						             : 0),//-widget->style->klass->ythickness),
						      &p.x, &p.y);
			sp_desktop_w2d_xy_point (desktop, &p, p.x, p.y);
			sp_guideline_moveto ((SPGuideLine *) guide, p.x, p.y);
			switch (SP_GUIDELINE(guide)->orientation){
			case SP_GUIDELINE_ORIENTATION_HORIZONTAL:
			  sp_desktop_coordinate_status (desktop, p.x, p.y, 2);
			  break;
			case SP_GUIDELINE_ORIENTATION_VERTICAL:
			  sp_desktop_coordinate_status (desktop, p.x, p.y, 1);
			  break;
			}
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging && event->button.button == 1) {
		        gdk_pointer_ungrab (event->button.time);
			dragging = FALSE;
			gtk_object_destroy (GTK_OBJECT (guide));
			guide = NULL;
			if ((horiz && (event->button.y > widget->allocation.height)) ||
				(!horiz && (event->button.x > widget->allocation.width))) {
				SPRepr * repr;
				repr = sp_repr_new ("sodipodi:guide");
				sp_repr_set_attr (repr, "orientation", horiz ? "horizontal" : "vertical");
				sp_repr_set_double_attribute (repr, "position", horiz ? p.y : p.x);
				sp_repr_append_child (SP_OBJECT_REPR (desktop->namedview), repr);
				sp_repr_unref (repr);
				sp_document_done (SP_DT_DOCUMENT (desktop));
			}
			sp_desktop_coordinate_status (desktop, p.x, p.y, 4);
		}
	default:
		break;
	}
	return FALSE;
}

gint
sp_dt_hruler_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	return sp_dt_ruler_event (widget, event, data, TRUE);
}

gint
sp_dt_vruler_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	return sp_dt_ruler_event (widget, event, data, FALSE);
}

/* Guides */

gint
sp_dt_guide_event (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	static gboolean dragging = FALSE, moved = FALSE;
	SPGuide * guide;
	SPDesktop * desktop;
	ArtPoint p;
	gint ret = FALSE;
	GString * msg;

	guide = SP_GUIDE (data);
	desktop = SP_DESKTOP (gtk_object_get_data (GTK_OBJECT (item->canvas), "SPDesktop"));

	switch (event->type) {
	case GDK_2BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = FALSE;
			gnome_canvas_item_ungrab (item, event->button.time);
			sp_dt_simple_guide_dialog (guide, desktop);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			gnome_canvas_item_grab (item, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_guide_moveto (guide, p.x, p.y);
			moved = TRUE;
			switch (guide->orientation){
			case SP_GUIDE_HORIZONTAL:
				sp_desktop_coordinate_status (desktop, p.x, p.y, 2);
				break;
			case SP_GUIDE_VERTICAL:
				sp_desktop_coordinate_status (desktop, p.x, p.y, 1);
				break;
			}
			ret=TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging && event->button.button == 1) {
			if (moved) {
				GtkWidget * w;
				double winx, winy;
				w = GTK_WIDGET (item->canvas);
				gnome_canvas_world_to_window (item->canvas, event->button.x, event->button.y, &winx, &winy);
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				if ((winx >= 0) && (winy >= 0) &&
				    (winx < w->allocation.width) &&
				    (winy < w->allocation.height)) {
					sp_guide_set (guide, p.x, p.y);
				} else {
					sp_guide_remove(guide);
				}
				moved = FALSE;
				sp_document_done (SP_DT_DOCUMENT (desktop));
				sp_desktop_coordinate_status (desktop, p.x, p.y, 4);
			}
			dragging = FALSE;
			gnome_canvas_item_ungrab (item, event->button.time);
			ret=TRUE;
		}
	case GDK_ENTER_NOTIFY:
		gnome_canvas_item_set (item, "color", guide->hicolor, NULL);
		msg = SP_PT_TO_METRIC_STRING (guide->position, SP_DEFAULT_METRIC);
		switch (guide->orientation) {
		case SP_GUIDE_HORIZONTAL:
			g_string_prepend(msg, "horizontal guideline at ");
			break;
		case SP_GUIDE_VERTICAL:
			g_string_prepend(msg, "vertical guideline at ");
			break;
		}
		sp_desktop_set_status (desktop, msg->str);
		g_string_free (msg, FALSE);
       		break;
	case GDK_LEAVE_NOTIFY:
		gnome_canvas_item_set (item, "color", guide->color, NULL);
		sp_desktop_clear_status (desktop);
		break;
	default:
		break;
	}

	return ret;
}



/*
 * simple guideline dialog
 */

static GtkWidget *d = NULL, *l1, *l2, * e , *u, * m;
static gdouble oldpos;
static gboolean mode;
static gpointer g;

#if 0
static gdouble
get_document_len (SPGuideOrientation orientation)
{
  SPDocument * document = NULL;
  gdouble len =0;

  document = SP_ACTIVE_DOCUMENT;
  g_return_val_if_fail (SP_IS_DOCUMENT (document),0);
  switch (orientation) {
  case SP_GUIDE_VERTICAL:
    len = sp_document_width (document);
    break;
  case SP_GUIDE_HORIZONTAL:
    len = sp_document_height (document);
    break;
  }    
  return len;
}
#endif

#if 0
static void
guide_dialog_unit_changed (SPUnitMenu * u, SPSVGUnit system, SPMetric metric, SPGuide * guide)
{
  gdouble len = 0, pos = 0;
  GString * st = NULL;

  switch (system) {
  case SP_SVG_UNIT_ABSOLUTE:
    st = SP_PT_TO_METRIC_STRING (oldpos, metric);
    g_string_prepend (st,"from ");
    break;
  case SP_SVG_UNIT_PERCENT:
    len = get_document_len (guide->orientation);
    pos = 100 * oldpos / len;
    st = g_string_new ("");
    g_string_sprintf (st, "from %0.2f %s", pos, "%");    
    break;
  default:
    g_print("unit not allowed (should not happen)\n");
    break;
  }
  gtk_label_set (GTK_LABEL (l2), st->str);
  g_string_free (st, TRUE);
}
#endif

static void
guide_dialog_mode_changed (GtkWidget * widget)
{
  if (mode) {
    gtk_label_set_text (GTK_LABEL (m), " relative by ");
    mode = FALSE;
  } else {
    gtk_label_set_text (GTK_LABEL (m), " absolute to ");
    mode = TRUE;
  }
}

static void
guide_dialog_close (GtkWidget * widget, GnomeDialog * d)
{
  gnome_dialog_close (d);
}

static void
guide_dialog_apply (GtkWidget * widget, SPGuide ** g)
{
	gdouble distance;
	const SPUnit *unit;
	gdouble newpos;
	SPGuide * guide;
  
	guide = *g;

	distance = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (e));
	unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (u));
	sp_convert_distance_full (&distance, unit, sp_unit_get_identity (SP_UNIT_ABSOLUTE), 1.0, 1.0);
	if (mode) {
		newpos = distance;
	} else {
		newpos = guide->position + distance;
	}
	sp_guide_set (guide, newpos, newpos);
	sp_document_done (SP_OBJECT_DOCUMENT (guide));
}

static void
guide_dialog_ok (GtkWidget * widget, gpointer g)
{
  guide_dialog_apply (NULL, g);
  guide_dialog_close (NULL, GNOME_DIALOG(d));
}

static void
guide_dialog_delete (GtkWidget *widget, SPGuide **guide)
{
	SPDocument *doc;
  
	doc = SP_OBJECT_DOCUMENT (*guide);
	sp_guide_remove (*guide);
	sp_document_done (doc);
	guide_dialog_close (NULL, GNOME_DIALOG (d));
}

static void
sp_dt_simple_guide_dialog (SPGuide *guide, SPDesktop *desktop)
{
	gdouble val = 0;
	GtkWidget * pix, * b1, * b2, * b3, * b4,* but;
	const SPUnit *unit;

	if (!GTK_IS_WIDGET (d)) {
		GtkObject *a;
		// create dialog
		d = gnome_dialog_new ("Guideline", 
				      GNOME_STOCK_BUTTON_OK, 
				      "Delete",
				      GNOME_STOCK_BUTTON_CLOSE,
				      NULL);
		gtk_window_set_modal (GTK_WINDOW (d), TRUE);
		gnome_dialog_close_hides (GNOME_DIALOG (d), TRUE);
    
		b1 = gtk_hbox_new (FALSE,4);
		gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (d)->vbox), b1, FALSE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (b1), 4);
		gtk_widget_show (b1);

		b2 = gtk_vbox_new (FALSE,4);
		gtk_box_pack_end (GTK_BOX (b1), b2, TRUE, TRUE, 0);
		gtk_widget_show (b2);
    
		//pixmap
		pix = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/guide_dialog.png");
		gtk_box_pack_start (GTK_BOX (b1), pix, TRUE, TRUE, 0);
		gtk_widget_show (pix);
		//labels
		b3 = gtk_hbox_new (FALSE,4);
		gtk_box_pack_start (GTK_BOX (b2), b3, TRUE, TRUE, 0);
		gtk_widget_show (b3);

		l1 = gtk_label_new ("foo1");
		gtk_box_pack_start (GTK_BOX (b3), l1, TRUE, TRUE, 0);
		gtk_misc_set_alignment (GTK_MISC (l1), 1.0, 0.5);
		gtk_widget_show (l1);

		l2 = gtk_label_new ("foo2");
		gtk_box_pack_start (GTK_BOX (b3), l2, TRUE, TRUE, 0);
		gtk_misc_set_alignment (GTK_MISC (l2), 0.0, 0.5);
		gtk_widget_show (l2);
    
		b4 = gtk_hbox_new (FALSE,4);
		gtk_box_pack_start (GTK_BOX (b2), b4, FALSE, FALSE, 0);
		gtk_widget_show (b4);
		// mode button
		but = gtk_button_new ();
		gtk_button_set_relief (GTK_BUTTON (but), GTK_RELIEF_NONE);
		gtk_box_pack_start (GTK_BOX (b4), but, FALSE, TRUE, 0);
		gtk_signal_connect_while_alive (GTK_OBJECT (but), "clicked", GTK_SIGNAL_FUNC (guide_dialog_mode_changed), 
						NULL , GTK_OBJECT(but));
		gtk_widget_show (but);
		m = gtk_label_new (" absolute to ");
		mode = TRUE;
		gtk_container_add (GTK_CONTAINER (but), m);
		gtk_widget_show (m);
    
		// spinbutton
		a = gtk_adjustment_new (0.0, -SP_DESKTOP_SCROLL_LIMIT, SP_DESKTOP_SCROLL_LIMIT, 1.0, 10.0, 10.0);
		e = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0 , 2);
		gtk_widget_show (e);
		gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (e), TRUE);
		gtk_box_pack_start (GTK_BOX (b4), e, TRUE, TRUE, 0);
		gnome_dialog_editable_enters (GNOME_DIALOG (d), GTK_EDITABLE (e)); 
		// unitmenu
		/* fixme: We should allow percents here too */
		u = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
		gtk_widget_show (u);
		gtk_box_pack_start (GTK_BOX (b4), u, FALSE, FALSE, 0);
		sp_unit_selector_set_unit (SP_UNIT_SELECTOR (u), sp_unit_get_identity (SP_UNIT_ABSOLUTE));
		sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (u), GTK_ADJUSTMENT (a));
#if 0
		gtk_signal_connect_while_alive (GTK_OBJECT (u), "set_unit", GTK_SIGNAL_FUNC (guide_dialog_unit_changed), 
						guide , GTK_OBJECT(u));
#endif
		// dialog
		gnome_dialog_set_default (GNOME_DIALOG (d), 0);
		gnome_dialog_button_connect (GNOME_DIALOG (d), 0, GTK_SIGNAL_FUNC (guide_dialog_ok), &g);
		gnome_dialog_button_connect (GNOME_DIALOG (d), 1, GTK_SIGNAL_FUNC (guide_dialog_delete), &g);
		//gnome_dialog_button_connect (GNOME_DIALOG (d), 1, GTK_SIGNAL_FUNC (guide_dialog_apply), &g);
		gnome_dialog_button_connect (GNOME_DIALOG (d), 2, GTK_SIGNAL_FUNC (guide_dialog_close), d);
	}

	// initialize dialog
	g = guide;
	oldpos = guide->position;
	switch (guide->orientation) {
	case SP_GUIDE_VERTICAL:
		gtk_label_set (GTK_LABEL (l1), "Move vertical guideline");
		break;
	case SP_GUIDE_HORIZONTAL:
		gtk_label_set (GTK_LABEL (l1), "Move horizontal guideline");
		break;
	}

	val = oldpos;
	unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (u));
	sp_convert_distance_full (&val, sp_unit_get_identity (SP_UNIT_ABSOLUTE), unit, 1.0, 1.0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (e), val);
#if 0
	switch (system) {
	case SP_SVG_UNIT_ABSOLUTE:
		val = SP_PT_TO_METRIC (oldpos, metric);
		break;
	case SP_SVG_UNIT_PERCENT:
		len = get_document_len (guide->orientation);
		val = 100 * guide->position /len;
		break;
	default:
		g_print("unit not allowed (should not happen\n");
		break;
	}
#endif
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (e), val);
	gtk_widget_grab_focus (e);
	gtk_editable_select_region (GTK_EDITABLE (e), 0, 20);
	gtk_window_set_position (GTK_WINDOW (d), GTK_WIN_POS_MOUSE);

	gtk_widget_show (d);
}


