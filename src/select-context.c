#define SP_SELECT_CONTEXT_C

#include <math.h>
#include "rubberband.h"
#include "sodipodi-private.h"
#include "document.h"
#include "selection.h"
#include "desktop-affine.h"
#include "seltrans-handles.h"
#include "pixmaps/cursor-select.xpm"
#include "select-context.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "desktop.h"
#include "dialogs/object-properties.h"
#include "sp-metrics.h"

static void sp_select_context_class_init (SPSelectContextClass * klass);
static void sp_select_context_init (SPSelectContext * select_context);
static void sp_select_context_destroy (GtkObject * object);

static void sp_select_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_select_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_select_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_selection_moveto (SPSelTrans * seltrans, double x, double y, guint state);

static SPEventContextClass * parent_class;

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
	event_context_class->root_handler = sp_select_context_root_handler;
	event_context_class->item_handler = sp_select_context_item_handler;
}

static void
sp_select_context_init (SPSelectContext * select_context)
{
}

static void
sp_select_context_destroy (GtkObject * object)
{
	SPSelectContext * select_context;

	select_context = SP_SELECT_CONTEXT (object);

	sp_sel_trans_shutdown (&select_context->seltrans);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_select_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPSelectContext * select_context;

	select_context = SP_SELECT_CONTEXT (event_context);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	sp_sel_trans_init (&select_context->seltrans, event_context->desktop);
}

static gint
sp_select_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	SPDesktop * desktop;
	SPSelectContext * select_context;
	SPSelTrans * seltrans;
	SPSelection * selection;
	GdkCursor * cursor;
	ArtPoint p;
	static int dragging;
	gint ret = FALSE;

	desktop = event_context->desktop;
	select_context = SP_SELECT_CONTEXT (event_context);
	seltrans = &select_context->seltrans;
	selection = SP_DT_SELECTION (desktop);

	switch (event->type) {
	case GDK_2BUTTON_PRESS:
	  ret = TRUE;
	  break;
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			if (!sp_selection_is_empty (selection)) {
			  if (event->button.state & GDK_SHIFT_MASK) {
			    sp_sel_trans_reset_state (seltrans);
			    if (sp_selection_item_selected (selection, item)) {
			      sp_selection_remove_item (selection, item);
			    } else {
			      sp_selection_add_item (selection, item);
			    }
			  } else {
			    if (sp_selection_item_selected (selection, item)) {
			      sp_sel_trans_increase_state (seltrans);
			    } else {
			      sp_sel_trans_reset_state (seltrans);
			      sp_selection_set_item (selection, item);
			    }
			  }
			} else {
			        sp_sel_trans_reset_state (seltrans);
				sp_selection_set_item (selection, item);
			}
			if (!sp_selection_is_empty (selection)) {
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				sp_sel_trans_grab (seltrans, NULL, p.x, p.y, FALSE);
				dragging = TRUE;
			}
			gnome_canvas_item_grab (sp_item_canvas_item (item, desktop), 
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_selection_moveto (seltrans, p.x, p.y, event->button.state);
			// status message

			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			sp_sel_trans_ungrab (seltrans);
			dragging = FALSE;
			sp_selection_changed (SP_DT_SELECTION (seltrans->desktop));
			gnome_canvas_item_ungrab (sp_item_canvas_item (item, desktop), event->button.time);
			sp_desktop_clear_status (desktop);
			ret = TRUE;
			break;
		default:
			break;
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
sp_select_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPDesktop * desktop;
	SPSelectContext * select_context;
	SPSelTrans * seltrans;
	SPSelection * selection;
	SPItem * item;
	gint ret = FALSE;
	ArtPoint p;
	ArtDRect b;
	GSList * l;

	desktop = event_context->desktop;
	select_context = SP_SELECT_CONTEXT (event_context);
	seltrans = &select_context->seltrans;
	selection = SP_DT_SELECTION (desktop);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
			case 1:
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				sp_sel_trans_grab (seltrans, NULL, p.x, p.y, FALSE);
				sp_rubberband_start (desktop, p.x, p.y);
				gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
						  GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
							NULL, event->button.time);
				ret = TRUE;
				break;
			default:
				break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_rubberband_move (p.x, p.y);
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			if (sp_rubberband_rect (&b)) {
				sp_rubberband_stop ();
				sp_sel_trans_reset_state (seltrans);
				l = sp_document_items_in_box (SP_DT_DOCUMENT (desktop), &b);
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
			sp_sel_trans_ungrab (seltrans);
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate), event->button.time);
			ret = TRUE;
			break;
		default:
			break;
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
	    sp_selection_delete (GTK_WIDGET(desktop));
	    ret = TRUE;
	    break;
	  case GDK_d: // Ctrl d - duplicate selection
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_selection_duplicate (GTK_WIDGET(desktop));
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

void
sp_handle_stretch (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
	ArtPoint p;
	double stretch[6], n2p[6], p2n[6];
	gchar status[80];

	sp_sel_trans_d2n_xy_point (seltrans, &p, x, y);
	p.x = 1.0;

	if (state & GDK_SHIFT_MASK) {
		n2p[0] = 1.0;
		n2p[1] = 0.0;
		n2p[2] = 0.0;
		n2p[3] = 2.0;
		n2p[4] = 0.0;
		n2p[5] = -1.0;
		art_affine_invert (p2n, n2p);
		art_affine_point (&p, &p, n2p);
	} else {
		art_affine_identity (n2p);
		art_affine_identity (p2n);
	}

	if (state & GDK_CONTROL_MASK) {
		if (fabs (p.y) > fabs (p.x)) p.y = p.y / fabs (p.y);
		if (fabs (p.y) < fabs (p.x)) p.x = fabs (p.y) * p.x / fabs (p.x);
	}

	art_affine_scale (stretch, p.x, p.y);
	art_affine_multiply (stretch, n2p, stretch);
	art_affine_multiply (stretch, stretch, p2n);

	sp_sel_trans_transform (seltrans, stretch);

	// status text
	sprintf (status, "Scale  %0.2f%c, %0.2f%c", 100 * stretch[0], '%', 100 * stretch[3], '%');
	sp_desktop_set_status (seltrans->desktop, status);
}

void
sp_handle_scale (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state) {
	ArtPoint p;
	double scale[6], n2p[6], p2n[6];
	gchar status[80];

	sp_sel_trans_d2n_xy_point (seltrans, &p, x, y);

	if (state & GDK_SHIFT_MASK) {
		n2p[0] = 2.0;
		n2p[1] = 0;
		n2p[2] = 0;
		n2p[3] = 2.0;
		n2p[4] = -1.0;
		n2p[5] = -1.0;
		art_affine_invert (p2n, n2p);
		art_affine_point (&p, &p, n2p);
	} else {
		art_affine_identity (n2p);
		art_affine_identity (p2n);
	}

	if (state & GDK_CONTROL_MASK) {
		if (fabs (p.y) > fabs (p.x)) p.y = fabs (p.x) * p.y / fabs (p.y);
		if (fabs (p.x) > fabs (p.y)) p.x = fabs (p.y) * p.x / fabs (p.x);
	}
	art_affine_scale (scale, p.x, p.y);
	art_affine_multiply (scale, n2p, scale);
	art_affine_multiply (scale, scale, p2n);

	sp_sel_trans_transform (seltrans, scale);

	// status text
	sprintf (status, "Scale  %0.2f%c, %0.2f%c", 100 * scale[0], '%', 100 * scale[0], '%');
	sp_desktop_set_status (seltrans->desktop, status);
}

void
sp_handle_skew (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
  ArtPoint p;
  double skew[6], n2p[6], p2n[6];
  gchar status[80];

  sp_sel_trans_d2n_xy_point (seltrans, &p, x, y);

  skew[0] = 1.0;
  if (state & GDK_CONTROL_MASK) {
    if (fabs(p.y) < 1e-15) return;
    skew[3] = p.y;
    skew[2] = p.x;
  } else {
    if (fabs(p.y) < 1e-15) return;
    if (fabs(p.y) < 1) p.y = fabs(p.y)/p.y;
    skew[3] = (double)((int)(p.y + 0.5*(fabs(p.y)/p.y)));
    if (fabs(skew[3]) < 1e-15) return;
    skew[2] = p.x;
  }
  skew[1] = 0.0;
  skew[4] = 0.0;
  skew[5] = 0.0;
  
  if (state & GDK_SHIFT_MASK) {
    n2p[0] = 2.0;
    n2p[1] = 0;
    n2p[2] = 0;
    n2p[3] = 2.0;
    n2p[4] = -1.0;
    n2p[5] = -1.0;
    art_affine_invert (p2n, n2p);
    skew[3] = -1 + 2*skew[3];
    skew[2] = 2*skew[2];
  } else {
    art_affine_identity (n2p);
    art_affine_identity (p2n);
  }
  
  art_affine_multiply (skew, n2p, skew);
  art_affine_multiply (skew, skew, p2n);
  
  sp_sel_trans_transform (seltrans, skew);

  // status text
  sprintf (status, "Skew  %0.2f%c", 100 * fabs(skew[2]), '%');
  sp_desktop_set_status (seltrans->desktop, status);
}

void
sp_handle_rotate (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
  ArtPoint p, q;
  double s[6], rotate[6];
  double d, angle, dx1, dy1, dx2, dy2, hyp1, hyp2, a1, a2;
  gchar status[80];
 
  sp_sel_trans_point_normal (seltrans, &p);
  d = sqrt (p.x * p.x + p.y * p.y);
  if (d < 1e-15) return;
  p.x /= d;
  p.y /= d;
  s[0] = p.x;
  s[1] = -p.y;
  s[2] = p.y;
  s[3] = p.x;
  s[4] = 0.0;
  s[5] = 0.0;
  
  sp_sel_trans_d2n_xy_point (seltrans, &p, x, y);
  art_affine_point (&p, &p, s);
  d = sqrt (p.x * p.x + p.y * p.y);
  if (d < 1e-15) return;
  p.x /= d;
  p.y /= d;
  rotate[0] = p.x;
  rotate[1] = p.y;
  rotate[2] = -p.y;
  rotate[3] = p.x;
  rotate[4] = 0.0;
  rotate[5] = 0.0;

  sp_sel_trans_transform (seltrans, rotate);

  // status text
  // uh, this is ugly find a more elegant way to compute the angle  
  sp_sel_trans_point_desktop (seltrans, &q);
  dx1 = q.x - seltrans->center.x;
  dy1 = q.y - seltrans->center.y;
  dx2 = x   - seltrans->center.x;
  dy2 = y   - seltrans->center.y;
  hyp1 = hypot (dx1, dy1);
  hyp2 = hypot (dx2, dy2);
  a1 = (dx1>=0) ? acos (dy1 / hyp1) : 2*M_PI - acos (dy1 / hyp1);
  a2 = (dx2>=0) ? acos (dy2 / hyp2) : 2*M_PI - acos (dy2 / hyp2);
  angle = fabs ((a2-a1) * 180 / M_PI);
  if (angle >180) angle = fabs (360 - angle);

  sprintf (status, "Rotate by %0.2f deg", angle);
  sp_desktop_set_status (seltrans->desktop, status);
}

void
sp_handle_center (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
	gchar status[80];
	GString * xs, * ys;
	sp_sel_trans_set_center (seltrans, x, y);
	// status text
	xs = SP_PT_TO_METRIC_STRING (x, SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (y, SP_DEFAULT_METRIC);
	sprintf (status, "Center  %s, %s", xs->str, ys->str);
	sp_desktop_set_status (seltrans->desktop, status);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

static void
sp_selection_moveto (SPSelTrans * seltrans, double x, double y, guint state)
{
	double dx, dy;
	double move[6];
	ArtPoint p;
	GString * xs, * ys;
	gchar status[80];

	sp_sel_trans_point_desktop (seltrans, &p);
	dx = x - p.x;
	dy = y - p.y;

	if (state & GDK_CONTROL_MASK) {
		if (fabs (dx) > fabs (dy)) dy = 0.0;
		else dx = 0.0;
	}

	art_affine_translate (move, dx, dy);
	sp_sel_trans_transform (seltrans, move);
	sp_sel_trans_reset_state (seltrans);
	// status text
	xs = SP_PT_TO_METRIC_STRING (dx, SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (dy, SP_DEFAULT_METRIC);
	sprintf (status, "Move  %s, %s", xs->str, ys->str);
	sp_desktop_set_status (seltrans->desktop, status);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

