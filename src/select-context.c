#define SP_SELECT_CONTEXT_C

#include <math.h>
#include "rubberband.h"
#include "document.h"
#include "selection.h"
#include "desktop-affine.h"
#include "seltrans-handles.h"
#include "pixmaps/cursor-select.xpm"
#include "select-context.h"

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
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			sp_sel_trans_ungrab (seltrans);
			dragging = FALSE;
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
			break;
		default:
			break;
		}
		break;
	case GDK_KEY_PRESS:
		g_print ("key %d pressed\n", event->key.keyval);
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
}

void
sp_handle_scale (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state) {
	ArtPoint p;
	double scale[6], n2p[6], p2n[6];

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
}

void
sp_handle_skew (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
	ArtPoint p;
	double skew[6];

	sp_sel_trans_d2n_xy_point (seltrans, &p, x, y);

	skew[0] = 1.0;
	skew[1] = 0.0;
	skew[2] = p.x;
	skew[3] = 1.0;
	skew[4] = 0.0;
	skew[5] = 0.0;

	sp_sel_trans_transform (seltrans, skew);
}

void
sp_handle_rotate (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
	ArtPoint p;
	double s[6], rotate[6];
	double d;

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
}

void
sp_handle_center (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state)
{
	sp_sel_trans_set_center (seltrans, x, y);
}

static void
sp_selection_moveto (SPSelTrans * seltrans, double x, double y, guint state)
{
	double dx, dy;
	double move[6];
	ArtPoint p;

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
	sp_selection_changed (SP_DT_SELECTION (seltrans->desktop));
}

