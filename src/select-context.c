#define __SP_SELECT_CONTEXT_C__

/*
 * Selection and transformation context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktogglebutton.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "helper/sp-canvas-util.h"
#include "rubberband.h"
#include "sodipodi-private.h"
#include "document.h"
#include "selection.h"
#include "desktop-affine.h"
#include "seltrans-handles.h"
#include "sp-cursor.h"
#include "pixmaps/cursor-select-m.xpm"
#include "pixmaps/cursor-select-d.xpm"
#include "pixmaps/handles.xpm"
#if 0
/* This is not needed, so kill warning (Lauris) */
#include "pixmaps/cursor-select.xpm"
#endif
#include "select-context.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "desktop.h"
#include "dialogs/object-properties.h"
#include "sp-metrics.h"
#include "desktop-snap.h"

static void sp_select_context_class_init (SPSelectContextClass * klass);
static void sp_select_context_init (SPSelectContext * select_context);
static void sp_select_context_destroy (GtkObject * object);

static void sp_select_context_setup (SPEventContext *ec);
static void sp_select_context_set (SPEventContext *ec, const guchar *key, const guchar *val);
static gint sp_select_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_select_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static GtkWidget *sp_select_context_config_widget (SPEventContext *ec);

static void sp_selection_moveto (SPSelTrans * seltrans, double x, double y, guint state);

static SPEventContextClass * parent_class;

GdkCursor * CursorSelectMouseover = NULL, * CursorSelectDragging = NULL;
GdkPixbuf * handles[13];

GtkType
sp_select_context_get_type (void)
{
	static GtkType select_context_type = 0;

	if (!select_context_type) {

		static const GtkTypeInfo select_context_info = {
			"SPSelectContext",
			sizeof (SPSelectContext),
			sizeof (SPSelectContextClass),
			(GtkClassInitFunc) sp_select_context_class_init,
			(GtkObjectInitFunc) sp_select_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		select_context_type = gtk_type_unique (sp_event_context_get_type (), &select_context_info);
	}

	return select_context_type;
}

static void
sp_select_context_class_init (SPSelectContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_select_context_destroy;

	event_context_class->setup = sp_select_context_setup;
	event_context_class->set = sp_select_context_set;
	event_context_class->root_handler = sp_select_context_root_handler;
	event_context_class->item_handler = sp_select_context_item_handler;
	event_context_class->config_widget = sp_select_context_config_widget;

	// cursors in select context
	CursorSelectMouseover = sp_cursor_new_from_xpm (cursor_select_m_xpm , 1, 1); 
	CursorSelectDragging = sp_cursor_new_from_xpm (cursor_select_d_xpm , 1, 1); 
	// selection handles
      	handles[0]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_scale_nw_xpm);
	handles[1]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_scale_ne_xpm);
	handles[2]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_scale_h_xpm);
	handles[3]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_scale_v_xpm);
	handles[4]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_nw_xpm);
	handles[5]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_n_xpm);
	handles[6]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_ne_xpm);
	handles[7]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_e_xpm);
	handles[8]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_se_xpm);
	handles[9]  = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_s_xpm);
	handles[10] = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_sw_xpm);
	handles[11] = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_rotate_w_xpm);
	handles[12] = gdk_pixbuf_new_from_xpm_data ((const gchar **)handle_center_xpm);

}

static void
sp_select_context_init (SPSelectContext * sc)
{
	sc->dragging = FALSE;
	sc->moved = FALSE;
	sc->button_press_shift = FALSE;
}

static void
sp_select_context_destroy (GtkObject * object)
{
	SPSelectContext *sc;

	sc = SP_SELECT_CONTEXT (object);

	if (sc->grabbed) {
		sp_canvas_item_ungrab (sc->grabbed, gdk_time_get ());
		sc->grabbed = NULL;
	}

	sp_sel_trans_shutdown (&sc->seltrans);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_select_context_setup (SPEventContext *ec)
{
	SPSelectContext * select_context;

	select_context = SP_SELECT_CONTEXT (ec);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (ec);

	sp_sel_trans_init (&select_context->seltrans, ec->desktop);

	sp_event_context_read (ec, "show");
	sp_event_context_read (ec, "transform");
}

static void
sp_select_context_set (SPEventContext *ec, const guchar *key, const guchar *val)
{
	SPSelectContext *sc;

	sc = SP_SELECT_CONTEXT (ec);

	if (!strcmp (key, "show")) {
		if (val && !strcmp (val, "outline")) {
			sc->seltrans.show = SP_SELTRANS_SHOW_OUTLINE;
		} else {
			sc->seltrans.show = SP_SELTRANS_SHOW_CONTENT;
		}
	} else if (!strcmp (key, "transform")) {
		if (val && !strcmp (val, "keep")) {
			sc->seltrans.transform = SP_SELTRANS_TRANSFORM_KEEP;
		} else {
			sc->seltrans.transform = SP_SELTRANS_TRANSFORM_OPTIMIZE;
		}
	}
}

static gint
sp_select_context_item_handler (SPEventContext *event_context, SPItem *item, GdkEvent *event)
{
	SPDesktop *desktop;
	SPSelectContext *sc;
	SPSelTrans *seltrans;
	SPSelection *selection;
	GdkCursor *cursor;
	NRPointF p;
	gint ret = FALSE;

	desktop = event_context->desktop;
	sc = SP_SELECT_CONTEXT (event_context);
	seltrans = &sc->seltrans;
	selection = SP_DT_SELECTION (desktop);

	switch (event->type) {
	case GDK_2BUTTON_PRESS:
		ret = TRUE;
		break;
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			/* Left mousebutton */
			sc->dragging = TRUE;
			sc->moved = FALSE;
			sc->item = item;
			sp_canvas_item_grab (SP_CANVAS_ITEM (desktop->drawing),
					     GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
					     NULL, event->button.time);
			sc->grabbed = SP_CANVAS_ITEM (desktop->drawing);
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			/* Left mousebutton */
			if (sc->dragging) {
				sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
				if (!sc->moved) {
					if (!sp_selection_item_selected (selection, sc->item)) {
						// have to select here since selecting is done when releasing
						sp_sel_trans_reset_state (seltrans);
						sp_selection_set_item (selection, sc->item);
					}
					sp_sel_trans_grab (seltrans, &p, -1, -1, FALSE);
					sc->moved = TRUE;
				}
				sp_selection_moveto (seltrans, p.x, p.y, event->button.state);
				ret = TRUE;
			}
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			if (sc->moved) {
				// item has been moved
				sp_sel_trans_ungrab (seltrans);
				sc->moved = FALSE;
				sp_view_set_status (SP_VIEW (desktop), NULL, FALSE);
			} else {
				// item has not been moved -> do selecting
				if (!sp_selection_is_empty (selection)) {
					if (event->button.state & GDK_SHIFT_MASK) {
						sp_sel_trans_reset_state (seltrans);
						if (sp_selection_item_selected (selection, sc->item)) {
							sp_selection_remove_item (selection, sc->item);
						} else {
							sp_selection_add_item (selection, sc->item);
						}
					} else {
						if (sp_selection_item_selected (selection, sc->item)) {
							sp_sel_trans_increase_state (seltrans);
						} else {
							sp_sel_trans_reset_state (seltrans);
							sp_selection_set_item (selection, sc->item);
						}
					}
				} else {
					sp_sel_trans_reset_state (seltrans);
					sp_selection_set_item (selection, sc->item);
				}
			}
			sc->dragging = FALSE;
			sc->item = NULL;
			if (sc->grabbed) {
				sp_canvas_item_ungrab (sc->grabbed, event->button.time);
				sc->grabbed = NULL;
			}
			ret = TRUE;
		} 
		break;
	case GDK_ENTER_NOTIFY:
		cursor = gdk_cursor_new (GDK_FLEUR);
		gdk_window_set_cursor (GTK_WIDGET (SP_DT_CANVAS (desktop))->window, cursor);
		gdk_cursor_destroy (cursor);
		break;
	case GDK_LEAVE_NOTIFY:
		gdk_window_set_cursor (GTK_WIDGET (SP_DT_CANVAS (desktop))->window, event_context->cursor);
		break;
	case GDK_KEY_PRESS:
		if (event->key.keyval == GDK_space) {
			/* stamping mode: show content mode moving */
			sp_sel_trans_stamp(seltrans);
			ret = TRUE;
		}
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);
	}

	return ret;
}

static gint
sp_select_context_root_handler (SPEventContext *event_context, GdkEvent * event)
{
	SPDesktop *desktop;
	SPSelectContext *sc;
	SPSelTrans *seltrans;
	SPSelection *selection;
	SPItem *item;
	gint ret = FALSE;
	NRPointF p;
	ArtDRect b;
	GSList *l;

	desktop = event_context->desktop;
	sc = SP_SELECT_CONTEXT (event_context);
	seltrans = &sc->seltrans;
	selection = SP_DT_SELECTION (desktop);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_rubberband_start (desktop, p.x, p.y);
			sp_canvas_item_grab (SP_CANVAS_ITEM (desktop->acetate),
					     GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
					     NULL, event->button.time);
			sc->grabbed = SP_CANVAS_ITEM (desktop->acetate);
#if 0
			/* We cannot assign shift for partial selects, as it is used for add mode */
			sc->button_press_shift = (event->button.state & GDK_SHIFT_MASK) ? TRUE : FALSE;
#endif
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			if (sc->dragging) {
				/* User has dragged fast, so we get events on root */
				if (!sc->moved) {
					if (!sp_selection_item_selected (selection, sc->item)) {
						// have to select here since selecting is done when releasing
						sp_sel_trans_reset_state (seltrans);
						sp_selection_set_item (selection, sc->item);
					}
					sp_sel_trans_grab (seltrans, &p, -1, -1, FALSE);
					sc->moved = TRUE;
				} 
				sp_selection_moveto (seltrans, p.x, p.y, event->button.state);
				ret = TRUE;
			} else {
				sp_rubberband_move (p.x, p.y);
			}
		}
		break;
	case GDK_BUTTON_RELEASE:
		if ((event->button.button == 1) && (sc->grabbed)) {
			if (sc->dragging) {
				if (sc->moved) {
					// item has been moved
					sp_sel_trans_ungrab (seltrans);
					sc->moved = FALSE;
					sp_view_set_status (SP_VIEW (desktop), NULL, FALSE);
				} else {
					// item has not been moved -> do selecting
					if (!sp_selection_is_empty (selection)) {
						if (event->button.state & GDK_SHIFT_MASK) {
							sp_sel_trans_reset_state (seltrans);
							if (sp_selection_item_selected (selection, sc->item)) {
								sp_selection_remove_item (selection, sc->item);
							} else {
								sp_selection_add_item (selection, sc->item);
							}
						} else {
							if (sp_selection_item_selected (selection, sc->item)) {
								sp_sel_trans_increase_state (seltrans);
							} else {
								sp_sel_trans_reset_state (seltrans);
								sp_selection_set_item (selection, sc->item);
							}
						}
					} else {
						sp_sel_trans_reset_state (seltrans);
						sp_selection_set_item (selection, sc->item);
					}
				}
				sc->dragging = FALSE;
				sc->item = NULL;
			} else {
				if (sp_rubberband_rect (&b)) {
					sp_rubberband_stop ();
					sp_sel_trans_reset_state (seltrans);
					if (sc->button_press_shift) {
						l = sp_document_partial_items_in_box (SP_DT_DOCUMENT (desktop), &b);
					} else {
						l = sp_document_items_in_box (SP_DT_DOCUMENT (desktop), &b);
					}
					if (event->button.state & GDK_SHIFT_MASK) {
						while (l) {
							item = SP_ITEM (l->data);
							if (sp_selection_item_selected (selection, item)) {
								sp_selection_remove_item (selection, item);
							} else {
								sp_selection_add_item (selection, item);
							}
							l = g_slist_remove (l, item);
						}
					} else {
						sp_selection_set_item_list (selection, l);
					}
				} else {
					if (!sp_selection_is_empty (selection)) {
						sp_selection_empty (selection);
						ret = TRUE;
					}
				}
				ret = TRUE;
			}
			if (sc->grabbed) {
				sp_canvas_item_ungrab (sc->grabbed, event->button.time);
				sc->grabbed = NULL;
			}
			sc->button_press_shift = FALSE;
		} 
		break;
	case GDK_KEY_PRESS: // keybindings for select context
          switch (event->key.keyval) {  
          case GDK_x: // Ctrl x - cut
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_cut(NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_c:  // Ctrl c - copy
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_copy(NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_v: // Ctrl v - paste
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_paste(NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_g: // Ctrl g - group
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_group(NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_G: // Ctrl G - ungroup
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_ungroup(NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_k: // Ctrl k - combine (shift - break appart)
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selected_path_combine();
	      ret = TRUE;
	    }
	    break;
	  case GDK_K: // Ctrl K - break appart
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selected_path_break_apart();
	      ret = TRUE;
	    }
	    break;
	  case GDK_Home: // Home - raise selection to top
	    sp_selection_raise_to_top (NULL);
            ret = TRUE;
            break;
	  case GDK_End: // End - lower selection to bottom
	    sp_selection_lower_to_bottom (NULL);
            ret = TRUE;
            break;
	  case GDK_Page_Up: // PageUp - raise selection one layer
	    sp_selection_raise (NULL);
            ret = TRUE;
            break;
	  case GDK_Page_Down: // PageDown - lower selection one layer
	    sp_selection_lower (NULL);
            ret = TRUE;
            break;
	  case GDK_Delete: // Del - delete selection
	    sp_selection_delete (NULL);
	    ret = TRUE;
	    break;
	  case GDK_d: // Ctrl d - duplicate selection
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_duplicate (NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Left: // Left - move selection left
	    if (event->key.state != GDK_CONTROL_MASK) {
	      if (event->key.state == GDK_SHIFT_MASK) sp_selection_move_screen (-1,0);
	      else sp_selection_move_screen (-10,0);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Up: // Up - move selection up
	    if (event->key.state != GDK_CONTROL_MASK) {
	      if (event->key.state == GDK_SHIFT_MASK) sp_selection_move_screen (0,1);
	      else sp_selection_move_screen (0,10);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Right: // Right - move selection right
	    if (event->key.state != GDK_CONTROL_MASK) {
	      if (event->key.state == GDK_SHIFT_MASK) sp_selection_move_screen (1,0);
	      else sp_selection_move_screen (10,0);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Down: // Down - move selection down
	    if (event->key.state != GDK_CONTROL_MASK) {
	      if (event->key.state == GDK_SHIFT_MASK) sp_selection_move_screen (0,-1);
	      else sp_selection_move_screen (0,-10);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Tab: // Tab - cycle selection forward
	    sp_selection_item_next ();
	    ret = TRUE;
	    break;
	  case GDK_ISO_Left_Tab: // Tab - cycle selection backward
	    sp_selection_item_prev ();
	    ret = TRUE;
	    break;
	  case GDK_space:
	    /* stamping mode: show outline mode moving */
	    /* FIXME: Is next condition ok? */
	    if (sc->dragging && sc->grabbed) 
		    sp_sel_trans_stamp(seltrans);
	    ret = TRUE;
	    break;
          }
	  break;
	default:
	  break;
	}
	
	if (!ret) {
	  if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
	    ret = SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler (event_context, event);
	}
	
	return ret;
}

static void
sp_selection_moveto (SPSelTrans * seltrans, double x, double y, guint state)
{
	double dx, dy;
	double move[6];
	ArtPoint p, norm = {0,0};
	GString * xs, * ys;
	gchar status[80];
	SPDesktop * desktop;

	desktop = seltrans->desktop;

	sp_sel_trans_point_desktop (seltrans, &p);
	dx = x - p.x;
	dy = y - p.y;

	if (state & GDK_MOD1_MASK) {
	  dx /= 10;
	  dy /= 10;
	}

	dx = sp_desktop_horizontal_snap_list (desktop, seltrans->snappoints, dx);
	dy = sp_desktop_vertical_snap_list (desktop, seltrans->snappoints, dy);

	if (state & GDK_CONTROL_MASK) {
		if (fabs (dx) > fabs (dy)) 
		  dy = 0.0;
		else 
		  dx = 0.0;
	}

	art_affine_translate (move, dx, dy);
	sp_sel_trans_transform (seltrans, move, &norm);

	// status text
	xs = SP_PT_TO_METRIC_STRING (dx, SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (dy, SP_DEFAULT_METRIC);
	sprintf (status, "Move  %s, %s", xs->str, ys->str);
	sp_view_set_status (SP_VIEW (seltrans->desktop), status, FALSE);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

/* Gtk styff */

static void
sp_select_context_show_toggled (GtkToggleButton *button, SPSelectContext *sc)
{
	if (gtk_toggle_button_get_active (button)) {
		const gchar *val;
		val = gtk_object_get_data (GTK_OBJECT (button), "value");
		sp_repr_set_attr (SP_EVENT_CONTEXT_REPR (sc), "show", val);
	}
}

static void
sp_select_context_transform_toggled (GtkToggleButton *button, SPSelectContext *sc)
{
	if (gtk_toggle_button_get_active (button)) {
		const gchar *val;
		val = gtk_object_get_data (GTK_OBJECT (button), "value");
		sp_repr_set_attr (SP_EVENT_CONTEXT_REPR (sc), "transform", val);
	}
}

static GtkWidget *
sp_select_context_config_widget (SPEventContext *ec)
{
	SPSelectContext *sc;
	GtkWidget *vb, *f, *fb, *b;

	sc = SP_SELECT_CONTEXT (ec);

	vb = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);

	f = gtk_frame_new (_("Visual transformation"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, FALSE, FALSE, 0);

	fb = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (fb);
	gtk_container_add (GTK_CONTAINER (f), fb);

	b = gtk_radio_button_new_with_label (NULL, _("Show content"));
	gtk_widget_show (b);
	gtk_object_set_data (GTK_OBJECT (b), "value", "content");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), sc->seltrans.show == SP_SELTRANS_SHOW_CONTENT);
	gtk_box_pack_start (GTK_BOX (fb), b, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_select_context_show_toggled), sc);

	b = gtk_radio_button_new_with_label (gtk_radio_button_group (GTK_RADIO_BUTTON (b)), _("Show outline"));
	gtk_widget_show (b);
	gtk_object_set_data (GTK_OBJECT (b), "value", "outline");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), sc->seltrans.show == SP_SELTRANS_SHOW_OUTLINE);
	gtk_box_pack_start (GTK_BOX (fb), b, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_select_context_show_toggled), sc);

	f = gtk_frame_new (_("Object transformation"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, FALSE, FALSE, 0);

	fb = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (fb);
	gtk_container_add (GTK_CONTAINER (f), fb);

	b = gtk_radio_button_new_with_label (NULL, _("Optimize"));
	gtk_widget_show (b);
	gtk_object_set_data (GTK_OBJECT (b), "value", "optimize");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), sc->seltrans.transform == SP_SELTRANS_TRANSFORM_OPTIMIZE);
	gtk_box_pack_start (GTK_BOX (fb), b, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_select_context_transform_toggled), sc);

	b = gtk_radio_button_new_with_label (gtk_radio_button_group (GTK_RADIO_BUTTON (b)), _("Preserve"));
	gtk_widget_show (b);
	gtk_object_set_data (GTK_OBJECT (b), "value", "keep");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), sc->seltrans.transform == SP_SELTRANS_TRANSFORM_KEEP);
	gtk_box_pack_start (GTK_BOX (fb), b, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_select_context_transform_toggled), sc);

	return vb;
}
